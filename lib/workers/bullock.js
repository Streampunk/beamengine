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

const { redisio, queue } = require('../../index.js');
const consumer = queue('media_workers');
const beamcoder = require('beamcoder');
const got = require('got');

async function getObj(url) {
  let response = await got(`http://localhost:3000${url}`, { json: true })
    .catch(err => {
      if (err.statusCode === 409) {
        console.log('Got conflict: assuming OK.');
      } else {
        throw err;
      }
    });
  return response.body;
}

async function getPacket(url) {
  const pkt = beamcoder.packet(await getObj(url));
  const response = await got(`http://localhost:3000${url}/data`, { responseType: 'buffer', encoding: null })
    .catch(err => {
      if (err.statusCode === 409) {
        console.log('Got conflict: assuming OK.');
      } else {
        throw err;
      }
    });
  pkt.data = response.body;
  return pkt;
}

async function makeJPEG(job) {
  let start = Date.now();
  console.log('Running makeJPEG', job.id);
  try {
    const streamUrl = job.data.path.slice(0, job.data.path.lastIndexOf('/'));
    const vstr = await getObj(streamUrl);
    const packetUrl = job.data.path.slice(0, job.data.path.lastIndexOf('.'));
    const pkt = await getPacket(packetUrl);

    let dec = beamcoder.decoder({ codec_id: vstr.codecpar.codec_id }); // Create a decoder
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
    let filtResult = await filt.filter([{ frames: decResult }]); // Filter the frame
    let filtFrame = filtResult[0].frames[0];

    let enc = beamcoder.encoder({ // Create an encoder for JPEG data
      name : 'mjpeg', // FFmpeg does not have an encoder called 'jpeg'
      width : filtFrame.width,
      height: filtFrame.height,
      pix_fmt: 'yuvj422p',
      time_base: [1, 1] });
    let jpegResult = await enc.encode(filtFrame); // Encode the filtered frame
    await enc.flush(); // Tidy the encoder

    job.data.body = await redisio.storeBlob(jpegResult.packets[0].data);
    job.data.success = true;

    console.log(`Finished ${job.id} - ${Date.now() - start}ms`);
    return job.data;
  } catch (err) {
    console.error(err.stack);
    throw err;
  }
}

async function makeMP4(job) {
  let start = Date.now();
  console.log('Running makeMP4', job.id);
  try {
    const streamUrl = job.data.path.slice(0, job.data.path.lastIndexOf('/'));
    const vstr = await getObj(streamUrl);
    const packetUrl = job.data.path.slice(0, job.data.path.lastIndexOf('.'));
    const pkt = await getPacket(packetUrl);
    console.log(vstr, pkt);
    // TODO...

    job.data.success = true;
    job.data.body = 'Hello MP4';
    console.log(`Finished ${job.id} - ${Date.now() - start}ms`);
    return job.data;
  } catch (err) {
    console.error(err.stack);
    throw err;
  }
}

const mediaWorkers = {
  makeJPEG: makeJPEG,
  makeMP4: makeMP4
};

consumer.process(async job => {
  const worker = mediaWorkers[job.data.rule.function];
  if (worker)
    return await worker(job);
  else
    throw new Error('media worker job failed!');
});
