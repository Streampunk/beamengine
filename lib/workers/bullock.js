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
const mediaSpec = require('../mediaSpec.js');
const fs = require('fs');

async function makeJPEG(job) {
  console.log('Running makeJPEG', job.id);
  let start = Date.now();
  try {
    const pp = job.data.path.split('/');
    const url = decodeURIComponent(pp[2]);
    const vstr = await redisio.retrieveStream(url, pp[3]);
    const pts = pp[4].slice(pp[4].indexOf('_')+1, pp[4].indexOf('.'));
    const pkt = await redisio.retrievePacket(url, pp[3], pts);

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

const adjustTS = (pkt, srcTB, dstTB) => {
  const adj = (srcTB[0] * dstTB[1]) / (srcTB[1] * dstTB[0]);
  pkt.pts = Math.round(pkt.pts * adj);
  pkt.dts = Math.round(pkt.dts * adj);
  pkt.duration > 0 ? Math.round(pkt.duration * adj) : Math.round(adj);
};

async function makeMP4(job) {
  console.log('Running makeMP4', job.id);
  let start = Date.now();
  try {
    const pp = job.data.path.split('/');
    const url = decodeURIComponent(pp[2]);
    const fmt = await redisio.retrieveFormat(url);
    const ms = mediaSpec.parseMediaSpec(pp[3].slice(0, pp[3].indexOf('.')));
    const streamIndex = 0;
    const vstr = fmt.streams[streamIndex];

    const width = 1280;
    const height = 720;
    const dec = beamcoder.decoder({ codec_id: vstr.codecpar.codec_id }); // Create a decoder

    const filt = await beamcoder.filterer({ // Create a filterer for video
      filterType: 'video',
      inputParams: [{
        width: vstr.codecpar.width,
        height: vstr.codecpar.height,
        pixelFormat: vstr.codecpar.format,
        timeBase: vstr.time_base,
        pixelAspect: vstr.sample_aspect_ratio }],
      outputParams: [{ pixelFormat: 'yuv422p' }],
      filterSpec: `scale=${width}:${height}, colorspace=all=bt709` });

    const enc = beamcoder.encoder({
      name : 'libx264',
      width : width,
      height: height,
      pix_fmt: 'yuv422p',
      sample_aspect_ratio: vstr.sample_aspect_ratio,
      time_base: vstr.time_base,
      framerate: [vstr.time_base[1], vstr.time_base[0]],
      // bit_rate: 2000000,
      // gop_size: 10,
      // max_b_frames: 1,
      // priv_data: { preset: 'slow' }
      priv_data: { crf: 23 } }); // ... more required ...

    const mux = beamcoder.muxer({ format_name: 'mp4' });
    let ovstr = mux.newStream({
      name: 'h264',
      time_base: [1, 90000],
      sample_aspect_ratio: vstr.sample_aspect_ratio,
      interleaved: true }); // Set to false for manual interleaving, true for automatic
    Object.assign(ovstr.codecpar, {
      width: width,
      height: height,
      format: 'yuv422p',
      sample_aspect_ratio: vstr.sample_aspect_ratio,
      field_order: vstr.codecpar.field_order,
      color_space: 'bt709' }); // ... how much is required ?
  
    await mux.openIO({
      url: 'file:temp.mp4'
    });
    await mux.writeHeader();

    for (let s = ms.start; s != ms.end; ++s) {
      const media = await redisio.retrieveMedia(url, streamIndex, s, s+1, 0, 25, ms.flags, false);
      for (const pkt of media) {
        const decResult = await dec.decode(pkt); // Decode the frame
        const filtResult = await filt.filter([{ frames: decResult }]); // Filter the decoded frame
        const encResult = await enc.encode(filtResult[0].frames[0]); // Encode the filtered frame
        for (const encPkt of encResult.packets) {
          adjustTS(encPkt, vstr.time_base, ovstr.time_base);
          await mux.writeFrame(encPkt);
        }
      }
    }
    const encResult = await enc.flush(); // Tidy the encoder
    for (const encPkt of encResult.packets) {
      adjustTS(encPkt, vstr.time_base, ovstr.time_base);
      await mux.writeFrame(encPkt);
    }
    await mux.writeTrailer();
    const mp4File = fs.readFileSync('temp.mp4');
    fs.unlinkSync('temp.mp4');

    job.data.body = {
      blobKey: await redisio.storeBlob(mp4File),
      blobType: 'video/mp4'
    };
    job.data.success = true;
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

process.on('SIGINT', async () => {
  await redisio.close()
    .catch(console.error);
  process.exit();
});
