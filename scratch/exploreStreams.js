/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2019 Streampunk Media Ltd.

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
const { redisio } = require('../index.js');
const beamcoder = require('beamcoder');
const mediaSpec = require('../lib/mediaSpec.js');
const { Readable, Writable, Transform } = require('stream');

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

  this.getLast = () => {
    let result = [];
    if (lastFrm.data[0].length > 0) {
      const resFrm = beamcoder.frame(lastFrm.toJSON());
      resFrm.data = [ lastFrm.data[0].slice(0) ];
      resFrm.nb_samples = lastFrm.data[0].length / sampleBytes;
      resFrm.pkt_duration = resFrm.nb_samples;
      lastFrm = beamcoder.frame({ pts: 0, data: [ Buffer.alloc(0) ]});
      result.push(resFrm);
    }
    return result;
  };
}

function diceFrames(frameDicer, frames, srcStrm, flush = false) {
  if ('audio' == srcStrm.codecpar.codec_type) {
    let result = frames.reduce((muxFrms, frm) => {
      frameDicer.addFrame(frm).map(f => muxFrms.push(f));
      return muxFrms;
    }, []);

    if (flush)
      frameDicer.getLast().map(f => result.push(f));

    return result;
  }

  return frames;
}

const serialBalancer = numStreams => {
  let pending = [];
  // initialise with negative ts and no pkt
  // - there should be no output until each stream has sent its first packet
  for (let s = 0; s < numStreams; ++s)
    pending.push({ ts: -Number.MAX_VALUE, streamIndex: s });

  return (pkt, streamIndex, ts) => {
    return new Promise(resolve => {
      Object.assign(pending[streamIndex], { pkt: pkt, ts: ts, resolve: resolve });
      const minTS = pending.reduce((acc, pend) => Math.min(acc, pend.ts), Number.MAX_VALUE);
      // console.log(streamIndex, pending.map(p => p.ts), minTS);
      const nextPend = pending.find(pend => pend.pkt && (pend.ts === minTS));
      if (nextPend) nextPend.resolve(nextPend.pkt);
      if (!pkt) resolve();
    });
  };
};

const adjustTS = (pkt, srcTB, dstTB) => {
  const adj = (srcTB[0] * dstTB[1]) / (srcTB[1] * dstTB[0]);
  pkt.pts = Math.round(pkt.pts * adj);
  pkt.dts = Math.round(pkt.dts * adj);
  pkt.duration > 0 ? Math.round(pkt.duration * adj) : Math.round(adj);
};

async function writeMux(mux, packets, muxIndex, srcStream, muxStream, muxBalancer, final) {
  if (packets.length) {
    return packets.reduce(async (promise, pkt) => {
      await promise;
      pkt.stream_index = muxIndex;
      adjustTS(pkt, srcStream.time_base, muxStream.time_base);
      const pktTS = pkt.pts * muxStream.time_base[0] / muxStream.time_base[1];
      return mux.writeFrame(await muxBalancer(pkt, muxIndex, pktTS));
    }, Promise.resolve());
  } else if (final)
    return muxBalancer(null, muxIndex, Number.MAX_VALUE);
}

const srcPktsGen = async function*(url, mediaSpec, index) {
  for (let pos = mediaSpec.start; pos != mediaSpec.end; ++pos) {
    yield await redisio.retrieveMedia(url, index, pos, pos+0.98, 0, Number.MAX_SAFE_INTEGER, mediaSpec.flags, false);
  }
};

function genToStream(params, gen) {
  return new Readable({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    read() {
      (async () => {
        try {
          const result = await gen.next();
          if (result.done)
            this.push(null);
          else
            this.push(result.value);
        } catch(err) {
          console.log(err);
          this.push(null); // end the stream
        }
      })();
    }
  });
}

function createTransform(params, processFn, flushFn) {
  return new Transform({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    transform(val, encoding, cb) {
      (async () => {
        try { cb(null, await processFn(val)); } 
        catch (err) { cb(err); }
      })();
    },
    flush(cb) {
      (async () => {
        try { cb(null, flushFn ? await flushFn() : null); } 
        catch (err) { cb(err); }
      })();
    }
  });
}

function createMuxStream(params, mux, muxIndex, srcStream, muxStream, muxBalancer) {
  return new Writable({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    write(pkts, encoding, cb) {
      (async () => {
        try { 
          await writeMux(mux, pkts.packets, muxIndex, srcStream, muxStream, muxBalancer, false);
          cb();
        } catch (err) { cb(err); }
      })();
    },
    final(cb) {
      // unlock any other streams in balancer that are waiting
      (async () => {
        try {
          await writeMux(mux, [], muxIndex, srcStream, muxStream, muxBalancer, true);
          cb();
        } catch (err) { cb(err); }
      })();
    }
  });
}

async function makeStreams(url, fmt, ms, indexes, decoders, filterers, encoders, mux) {
  const muxBalancer = serialBalancer(mux.streams.length);
  const frameDicers = indexes.map((srcIndex, muxIndex) => new frameDicer(mux.streams[muxIndex]));
  await Promise.all(
    indexes.map(async (srcIndex, muxIndex) => {
      return new Promise(resolve => {
        const srcStream = genToStream({ highWaterMark : 2 }, srcPktsGen(url, ms, srcIndex));
        const decStream = createTransform({ highWaterMark : 2 }, pkts => decoders[muxIndex].decode(pkts), () => decoders[muxIndex].flush());
        const filtStream = createTransform({ highWaterMark : 2 }, frms => filterers[muxIndex].filter(frms.frames));
        const diceStream = createTransform({ highWaterMark : 2 },
          frms => diceFrames(frameDicers[muxIndex], frms[0].frames, fmt.streams[srcIndex]), 
          () => diceFrames(frameDicers[muxIndex], [], fmt.streams[srcIndex], true));
        const encStream = createTransform({ highWaterMark : 2 }, frms => encoders[muxIndex].encode(frms), () => encoders[muxIndex].flush());
        const muxStream = createMuxStream({ highWaterMark : 2 }, mux, muxIndex, fmt.streams[srcIndex], mux.streams[muxIndex], muxBalancer);

        muxStream.on('error', console.error);
        muxStream.on('finish', resolve);

        srcStream.pipe(decStream).pipe(filtStream).pipe(diceStream).pipe(encStream).pipe(muxStream);
      });
    })
  );
}

async function testStreams() {
  // const url = 'file:../../Media/big_buck_bunny_1080p_h264.mov';
  const url = 'file:../../Media/dpp/AS11_DPP_HD_EXAMPLE_1.mxf';

  const spec = '70s-80s';

  console.log('Running testStreams');
  let start = Date.now();
  const fmt = await redisio.retrieveFormat(url);
  const ms = mediaSpec.parseMediaSpec(spec);

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
  console.log(`Finished ${Date.now() - start}ms`);
}

testStreams();