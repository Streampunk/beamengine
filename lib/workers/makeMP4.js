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
const mediaSpec = require('../mediaSpec.js');
const fs = require('fs');

const adjustTS = (pkt, srcTB, dstTB) => {
  const adj = (srcTB[0] * dstTB[1]) / (srcTB[1] * dstTB[0]);
  pkt.pts = Math.round(pkt.pts * adj);
  pkt.dts = Math.round(pkt.dts * adj);
  pkt.duration > 0 ? Math.round(pkt.duration * adj) : Math.round(adj);
};

function frameDicer(dstStrm) {
  const sampleBytes = 4; // Assume floating point 4 byte samples for now...
  let lastFrm = beamcoder.frame({ pts: 0, data: [ Buffer.alloc(0) ]});
  const dstNumSamples = dstStrm.codecpar.frame_size;
  const dstFrmBytes = dstNumSamples * dstStrm.codecpar.channels * sampleBytes;

  this.addFrame = srcFrm => {
    let result = [];
    let dstFrm;
    let curStart = 0;
    if (lastFrm.data[0].length > 0)
      dstFrm = beamcoder.frame(lastFrm.toJSON());
    else
      dstFrm = beamcoder.frame(srcFrm.toJSON());
    dstFrm.nb_samples = dstNumSamples;
    dstFrm.pkt_duration = dstNumSamples;

    while (curStart + dstFrmBytes - lastFrm.data[0].length < srcFrm.nb_samples * sampleBytes) {
      const resFrm = beamcoder.frame(dstFrm.toJSON());
      resFrm.data = [
        Buffer.concat([
          lastFrm.data[0],
          srcFrm.data[0].slice(curStart, curStart + dstFrmBytes - lastFrm.data[0].length)],
        dstFrmBytes)];
      result.push(resFrm);

      dstFrm.pts += dstNumSamples;
      dstFrm.pkt_dts += dstNumSamples;
      curStart += dstFrmBytes - lastFrm.data[0].length;
      lastFrm = beamcoder.frame({ pts: 0, data: [ Buffer.alloc(0) ]});
    }

    lastFrm.data = [ srcFrm.data[0].slice(curStart, srcFrm.nb_samples * sampleBytes)];
    lastFrm.pts = dstFrm.pts;
    lastFrm.pkt_dts = dstFrm.pts;

    return result;
  };
}

function adjustFrames(frameDicer, frames, srcStrm) {
  if ('audio' == srcStrm.codecpar.codec_type) {
    let result = frames.reduce((muxFrms, frm) => {
      frameDicer.addFrame(frm).map(f => muxFrms.push(f));
      return muxFrms;
    }, []);
    return result;
  }

  return frames;
}

async function writeMux(mux, packets, muxIndex, srcStrm, dstStrm) {
  return packets.reduce(async (promise, pkt) => {
    pkt.stream_index = muxIndex;
    adjustTS(pkt, srcStrm.time_base, dstStrm.time_base);
    await promise;
    return mux.writeFrame(pkt);
  }, Promise.resolve());
}

async function writeMuxes(mux, indexes, packets, srcStrms, dstStrms) {
  return indexes.reduce(async (promise, srcIndex, muxIndex) => {
    await promise;
    return writeMux(mux, packets[muxIndex].packets, muxIndex, srcStrms[srcIndex], dstStrms[muxIndex]);
  }, Promise.resolve());
}

async function makeStreams(url, fmt, ms, indexes, decoders, filterers, encoders, mux) {
  let frameDicers = indexes.map((srcIndex, muxIndex) => new frameDicer(mux.streams[muxIndex]));
  for (let s = ms.start; s != ms.end; ++s) {
    const encPkts = await Promise.all(
      indexes.map(async (srcIndex, muxIndex) => {
        const media = await redisio.retrieveMedia(url, srcIndex, s, s+0.98, 0, Number.MAX_SAFE_INTEGER, ms.flags, false);
        const decFrms = await decoders[muxIndex].decode(media);
        const filtFrms = await filterers[muxIndex].filter(decFrms.frames);
        const fifoFrms = adjustFrames(frameDicers[muxIndex], filtFrms[0].frames, fmt.streams[srcIndex]);
        return encoders[muxIndex].encode(fifoFrms);
      })
    );
    await writeMuxes(mux, indexes, encPkts, fmt.streams, mux.streams);
  }

  // flush the decoders
  let encPkts = await Promise.all(
    indexes.map(async (srcIndex, muxIndex) => {
      const decFrms = await decoders[muxIndex].flush();
      const filtFrms = await filterers[muxIndex].filter(decFrms.frames);
      const fifoFrms = adjustFrames(frameDicers[muxIndex], filtFrms[0].frames, fmt.streams[srcIndex]);
      return encoders[muxIndex].encode(fifoFrms);
    })
  );
  await writeMuxes(mux, indexes, encPkts, fmt.streams, mux.streams);

  // flush the encoders
  encPkts = await Promise.all(
    indexes.map(async (srcIndex, muxIndex) => await encoders[muxIndex].flush()));
  await writeMuxes(mux, indexes, encPkts, fmt.streams, mux.streams);
}

async function makeMP4(job) {
  console.log('Running makeMP4', job.id);
  let start = Date.now();
  try {
    const pp = job.data.path.split('/');
    const url = decodeURIComponent(pp[2]);
    const fmt = await redisio.retrieveFormat(url);
    const ms = mediaSpec.parseMediaSpec(pp[3].slice(0, pp[3].indexOf('.')));
    const vidIndex = 0;
    const vidStr = fmt.streams[vidIndex];
    let audIndex = 1;
    if (0 == fmt.streams[audIndex].codecpar.codec_id)
      audIndex += 1;
    const audStr = fmt.streams[audIndex];

    const width = 1280;
    const height = 720;
    const vidDec = beamcoder.decoder({ demuxer: fmt, stream_index: vidIndex }); // Create a video decoder
    const audDec = beamcoder.decoder({ demuxer: fmt, stream_index: audIndex }); // Create an audio decoder

    const vidFilt = await beamcoder.filterer({ // Create a filterer for video
      filterType: 'video',
      inputParams: [{
        width: vidStr.codecpar.width,
        height: vidStr.codecpar.height,
        pixelFormat: vidStr.codecpar.format,
        timeBase: vidStr.time_base,
        pixelAspect: vidStr.sample_aspect_ratio }],
      outputParams: [{ pixelFormat: 'yuv422p' }],
      filterSpec: `scale=${width}:${height}, colorspace=all=bt709` });

    const audFilt = await beamcoder.filterer({ // Create a filterer for audio
      filterType: 'audio',
      inputParams: [{
        sampleRate: audDec.sample_rate,
        sampleFormat: audDec.sample_fmt,
        channelLayout: audDec.channel_layout,
        timeBase: audStr.time_base }],
      outputParams: [{
        sampleRate: audDec.sample_rate,
        sampleFormat: 'fltp',
        channelLayout: 'mono' }], //audDec.channel_layout }],
      filterSpec: 'aformat=sample_fmts=fltp:channel_layouts=mono' });
  
    const vidEnc = beamcoder.encoder({
      name: 'libx264',
      width: width,
      height: height,
      pix_fmt: 'yuv422p',
      sample_aspect_ratio: vidStr.sample_aspect_ratio,
      time_base: vidStr.time_base,
      framerate: [vidStr.time_base[1], vidStr.time_base[0]],
      // bit_rate: 2000000,
      // gop_size: 10,
      // max_b_frames: 1,
      // priv_data: { preset: 'slow' }
      priv_data: { crf: 23 } }); // ... more required ...
    const audEnc = beamcoder.encoder({
      name: 'aac',
      sample_fmt: 'fltp',
      sample_rate: audDec.sample_rate,
      channels: 1, //audDec.channels,
      channel_layout: 'mono', //audDec.channel_layout,
      flags: { GLOBAL_HEADER: true } });
      
    const mux = beamcoder.muxer({ format_name: 'mp4' });

    let oVidStr = mux.newStream({
      name: 'h264',
      time_base: [1, 90000],
      sample_aspect_ratio: vidStr.sample_aspect_ratio,
      interleaved: true }); // Set to false for manual interleaving, true for automatic
    Object.assign(oVidStr.codecpar, {
      width: width,
      height: height,
      format: 'yuv422p',
      sample_aspect_ratio: vidStr.sample_aspect_ratio,
      field_order: vidStr.codecpar.field_order,
      color_space: 'bt709' }); // ... how much is required ?

    let oAudStr = mux.newStream({
      name: 'aac',
      time_base: [1, 90000],
      interleaved: true }); // Set to false for manual interleaving, true for automatic
    Object.assign(oAudStr.codecpar, {
      sample_rate: audDec.sample_rate,
      frame_size: 1024,
      channels: 1,
      channel_layout: 'mono' });

    await mux.openIO({
      url: 'file:temp.mp4'
    });
    await mux.writeHeader();

    await makeStreams(url, fmt, ms, [vidIndex, audIndex], [vidDec, audDec], [vidFilt, audFilt], [vidEnc, audEnc], mux);

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

module.exports = makeMP4;