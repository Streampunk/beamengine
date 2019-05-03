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

  this.isAudio = () => 'audio' === dstStrm.codecpar.codec_type;

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

function diceFrames(dicer, frames, flush = false) {
  if (dicer.isAudio()) {
    let result = frames.reduce((muxFrms, frm) => {
      dicer.addFrame(frm).map(f => muxFrms.push(f));
      return muxFrms;
    }, []);

    if (flush)
      dicer.getLast().map(f => result.push(f));

    return result;
  }

  return frames;
}

function parallelBalancer(params, streamType, numStreams) {
  let resolveGet = null;
  const tag = 'video' === streamType ? 'v' : 'a';
  const pending = [];
  // initialise with negative ts and no pkt
  // - there should be no output until each stream has sent its first packet
  for (let s = 0; s < numStreams; ++s)
    pending.push({ ts: -Number.MAX_VALUE, streamIndex: s });

  const makeSet = (resolve) => {
    if (resolve) {
      // console.log('makeSet', pending.map(p => p.ts));
      const nextPends = pending.every(pend => pend.pkt) ? pending : null;
      const final = pending.filter(pend => true === pend.final);
      if (nextPends) {
        nextPends.map(pend => pend.resolve());
        resolve({
          value: nextPends.map(pend => { 
            return { name: `in${pend.streamIndex}:${tag}`, frames: [ pend.pkt ] }; }), 
          done: false });
        resolveGet = null;
        pending.map(pend => Object.assign(pend, { pkt: null, ts: Number.MAX_VALUE }));
      } else if (final.length > 0) {
        final.map(f => f.resolve());
        resolve({ done: true });
      } else {
        resolveGet = resolve;
      }
    }
  };

  const pushPkt = async (pkt, streamIndex, ts) =>
    new Promise(resolve => {
      Object.assign(pending[streamIndex], { pkt: pkt, ts: ts, final: pkt ? false : true, resolve: resolve });
      makeSet(resolveGet);
    });

  const pullSet = async () => new Promise(resolve => makeSet(resolve));

  const readStream = new Readable({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    read() {
      (async () => {
        try {
          const result = await pullSet();
          if (result.done)
            this.push(null);
          else
            this.push(result.value);
        } catch(err) {
          console.log(err);
          this.push(null); // end the stream
        }
      })();
    },
  });

  readStream.pushPkts = (packets, stream, streamIndex, final = false) => {
    if (packets.length) {
      return packets.reduce(async (promise, pkt) => {
        await promise;
        const ts = pkt.pts * stream.time_base[0] / stream.time_base[1];
        return pushPkt(pkt, streamIndex, ts);
      }, Promise.resolve());
    } else if (final) {
      return pushPkt(null, streamIndex, Number.MAX_VALUE);
    }
  };

  return readStream;
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

async function writeMux(mux, packets, muxIndex, srcStream, muxStream, muxBalancer, final = false) {
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

function createFilterStream(params, stream, streamIndex, balancer) {
  return new Writable({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    write(pkts, encoding, cb) {
      (async () => {
        try {
          await balancer.pushPkts(pkts.frames, stream, streamIndex);
          cb();
        } catch (err) { cb(err); }
      })();
    },
    final(cb) {
      // unlock any other streams in balancer that are waiting
      (async () => {
        try {
          await balancer.pushPkts([], stream, streamIndex, true);
          cb();
        } catch (err) { cb(err); }
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
          await writeMux(mux, pkts.packets, muxIndex, srcStream, muxStream, muxBalancer);
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

function makeStreams(streamType, sources, decoders, filterer, encoder, mux, muxIndex, muxBalancer) {
  if (!sources.length) 
    return Promise.resolve();

  const timeBaseStream = sources[0].fmt.streams[sources[0].streamIndex];
  const filterBalancer = parallelBalancer({ highWaterMark : 2 }, streamType, decoders.length);

  sources.map((src, srcIndex) => {
    const srcStream = genToStream({ highWaterMark : 2 }, srcPktsGen(src.url, src.ms, src.streamIndex));
    const decStream = createTransform({ highWaterMark : 2 }, pkts => decoders[srcIndex].decode(pkts), () => decoders[srcIndex].flush());
    const filterSource = createFilterStream({ highWaterMark : 2 }, src.fmt.streams[src.streamIndex], srcIndex, filterBalancer);
    srcStream.pipe(decStream).pipe(filterSource);
  });

  return new Promise(resolve => {
    const filtStream = createTransform({ highWaterMark : 2 }, frms => filterer.filter(frms));
    const dicer = new frameDicer(mux.streams[muxIndex]);
    const diceStream = createTransform({ highWaterMark : 2 },
      frms => diceFrames(dicer, frms[0].frames), () => diceFrames(dicer, [], true));
    const encStream = createTransform({ highWaterMark : 2 }, frms => encoder.encode(frms), () => encoder.flush());
    const muxStream = createMuxStream({ highWaterMark : 2 }, mux, muxIndex, timeBaseStream, mux.streams[muxIndex], muxBalancer);

    muxStream.on('error', console.error);
    muxStream.on('finish', resolve);

    filterBalancer.pipe(filtStream).pipe(diceStream).pipe(encStream).pipe(muxStream);
  });
}

async function testStreams() {
  console.log('Running testStreams');
  let start = Date.now();

  const spec = mediaSpec.parseMediaSpec('70s-72s');
  const urls = [ 'file:../../Media/dpp/AS11_DPP_HD_EXAMPLE_1.mxf', 'file:../../Media/big_buck_bunny_1080p_h264.mov' ];
  const fmts = await Promise.all(urls.map(async url => await redisio.retrieveFormat(url)));
  const sources = {
    video: [
      { ms: spec, url: urls[0], fmt: fmts[0], streamIndex: 0 },
      { ms: spec, url: urls[1], fmt: fmts[1], streamIndex: 0 },
    ],
    audio: [
      { ms: spec, url: urls[0], fmt: fmts[0], streamIndex: 1 },
      // { ms: spec, url: urls[1], fmt: fmts[1], streamIndex: 2 }
    ]
  };

  const vidDecs = sources.video.map(src => beamcoder.decoder({ demuxer: src.fmt, stream_index: src.streamIndex }));
  const audDecs = sources.audio.map(src => beamcoder.decoder({ demuxer: src.fmt, stream_index: src.streamIndex }));

  const outWidth = 1280;
  const outHeight = 720;

  let vidFilt = null;
  let vidEnc = null; 
  if (sources.video.length) {
    vidFilt = await beamcoder.filterer({ // Create a filterer for video
      filterType: 'video',
      inputParams: sources.video.map((src, i) => {
        const stream = src.fmt.streams[src.streamIndex];
        return {
          name: `in${i}:v`,
          width: stream.codecpar.width,
          height: stream.codecpar.height,
          pixelFormat: stream.codecpar.format,
          timeBase: stream.time_base,
          pixelAspect: stream.sample_aspect_ratio };
      }),
      outputParams: [{ name: 'out0:v', pixelFormat: 'yuv422p' }],
      // filterSpec: `[in0:v] scale=${outWidth}:${outHeight}, colorspace=all=bt709 [out0:v]` });
      filterSpec: '[in0:v] scale=1280:720 [left]; [in1:v] scale=640:360 [right]; [left][right] overlay=format=auto:x=640 [out0:v]' });
    // console.log(vidFilt.graph.dump());

    // use first video stream for output time base, sample aspect ratio
    const vidStr = sources.video[0].fmt.streams[sources.video[0].streamIndex];

    vidEnc = beamcoder.encoder({
      name: 'libx264',
      width: outWidth,
      height: outHeight,
      pix_fmt: 'yuv422p',
      sample_aspect_ratio: vidStr.sample_aspect_ratio,
      time_base: vidStr.time_base,
      // framerate: [vidStr.time_base[1], vidStr.time_base[0]],
      // bit_rate: 2000000,
      // gop_size: 10,
      // max_b_frames: 1,
      // priv_data: { preset: 'slow' }
      priv_data: { crf: 23 } }); // ... more required ...
  }

  let audFilt = null; 
  let audEnc = null; 
  if (sources.audio.length) {
    const outSampleRate = 48000;

    audFilt = await beamcoder.filterer({ // Create a filterer for audio
      filterType: 'audio',
      inputParams: sources.audio.map((src, i) => {
        const stream = src.fmt.streams[src.streamIndex];
        return {
          name: `in${i}:a`,
          sampleRate: audDecs[i].sample_rate,
          sampleFormat: audDecs[i].sample_fmt,
          channelLayout: audDecs[i].channel_layout,
          timeBase: stream.time_base };
      }),
      outputParams: [{
        name: 'out0:a',
        sampleRate: outSampleRate,
        sampleFormat: 'fltp',
        channelLayout: 'mono' }], //audDec.channel_layout }],
      filterSpec: '[in0:a] aformat=sample_fmts=fltp:channel_layouts=mono [out0:a]' });
    // console.log(audFilt.graph.dump());

    audEnc = beamcoder.encoder({
      name: 'aac',
      sample_fmt: 'fltp',
      sample_rate: audDecs[0].sample_rate,
      channels: 1, //audDec.channels,
      channel_layout: 'mono', //audDec.channel_layout,
      flags: { GLOBAL_HEADER: true } });
  }

  const mux = beamcoder.muxer({ format_name: 'mp4' });

  let muxVidIndex = 0;
  let muxAudIndex = 0;

  if (sources.video.length) {
    // use first video stream for time base, sample aspect ratio
    const vidStr = sources.video[0].fmt.streams[sources.video[0].streamIndex];

    muxVidIndex = mux.streams.length;
    let oVidStr = mux.newStream({
      name: 'h264',
      time_base: [1, 90000],
      sample_aspect_ratio: vidStr.sample_aspect_ratio,
      interleaved: true }); // Set to false for manual interleaving, true for automatic
    Object.assign(oVidStr.codecpar, {
      width: outWidth,
      height: outHeight,
      format: 'yuv422p',
      sample_aspect_ratio: vidStr.sample_aspect_ratio,
      field_order: vidStr.codecpar.field_order,
      color_space: 'bt709' }); // ... how much is required ?
  }
  if (sources.audio.length) {
    muxAudIndex = mux.streams.length;
    let oAudStr = mux.newStream({
      name: 'aac',
      time_base: [1, 90000],
      interleaved: true }); // Set to false for manual interleaving, true for automatic
    Object.assign(oAudStr.codecpar, {
      sample_rate: audDecs[0].sample_rate,
      frame_size: 1024,
      channels: 1,
      channel_layout: 'mono' });
  }

  await mux.openIO({
    url: 'file:temp.mp4'
  });
  await mux.writeHeader();

  const muxBalancer = serialBalancer(mux.streams.length);
  await Promise.all([
    makeStreams('video', sources.video, vidDecs, vidFilt, vidEnc, mux, muxVidIndex, muxBalancer),
    makeStreams('audio', sources.audio, audDecs, audFilt, audEnc, mux, muxAudIndex, muxBalancer)
  ]).catch(console.error);

  await mux.writeTrailer();
  console.log(`Finished ${Date.now() - start}ms`);
}

testStreams();