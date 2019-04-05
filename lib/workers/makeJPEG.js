/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2019  Streampunk Media Ltd.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.


  https://www.streampunk.media/ mailto:furnace@streampunk.media
  14 Ormiscaig, Aultbea, Achnasheen, IV22 2JJ  U.K.
*/

const { redisio } = require('../../index.js');
const beamcoder = require('beamcoder');

async function makeJPEG(job) {
  console.log('Running makeJPEG', job.id);
  let start = Date.now();
  try {
    const pp = job.data.path.split('/');
    const url = decodeURIComponent(pp[2]);
    const fmt = await redisio.retrieveFormat(url);
    const vstr = fmt.streams[0];
    const pts = pp[4].slice(pp[4].indexOf('_')+1, pp[4].indexOf('.'));
    const pkt = await redisio.retrieveMedia(url, pp[3], pts, pts);
    let dec = beamcoder.decoder({ demuxer: fmt, stream_index: 0 }); // Create a decoder
    let decResult = await dec.decode(pkt); // Decode the frame
    if (decResult.frames.length === 0) // Frame may be buffered, so flush it out
      decResult = await dec.flush();

    let filt = await beamcoder.filterer({ // Create a filterer for video
      filterType: 'video',
      inputParams: [{
        width: vstr.codecpar.width,
        height: vstr.codecpar.height,
        pixelFormat: vstr.codecpar.format,
        timeBase: vstr.time_base,
        pixelAspect: vstr.sample_aspect_ratio }],
      outputParams: [{ pixelFormat: 'yuv422p' }],
      filterSpec: 'scale=1280:720, colorspace=range=jpeg:all=bt709' });
    let filtResult = await filt.filter(decResult.frames); // Filter the frame
    let filtFrame = filtResult[0].frames[0];

    let enc = beamcoder.encoder({ // Create an encoder for JPEG data
      name : 'mjpeg', // FFmpeg does not have an encoder called 'jpeg'
      width : filtFrame.width,
      height: filtFrame.height,
      pix_fmt: 'yuvj422p',
      time_base: [1, 1] });
    let jpegResult = await enc.encode(filtFrame); // Encode the filtered frame
    await enc.flush(); // Tidy the encoder

    job.data.body = {
      blobKey: await redisio.storeBlob(jpegResult.packets[0].data),
      blobType: 'image/jpeg'
    };
    job.data.success = true;

    console.log(`Finished ${job.id} - ${Date.now() - start}ms`);
    return job.data;
  } catch (err) {
    console.error(err.stack);
    throw err;
  }
}

module.exports = makeJPEG;