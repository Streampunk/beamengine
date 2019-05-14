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
      dicer.addFrame(frm).forEach(f => muxFrms.push(f));
      return muxFrms;
    }, []);

    if (flush)
      dicer.getLast().forEach(f => result.push(f));

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

  const makeSet = resolve => {
    if (resolve) {
      // console.log('makeSet', pending.map(p => p.ts));
      const nextPends = pending.every(pend => pend.pkt) ? pending : null;
      const final = pending.filter(pend => true === pend.final);
      if (nextPends) {
        nextPends.forEach(pend => pend.resolve());
        resolve({
          value: nextPends.map(pend => { 
            return { name: `in${pend.streamIndex}:${tag}`, frames: [ pend.pkt ] }; }), 
          done: false });
        resolveGet = null;
        pending.forEach(pend => Object.assign(pend, { pkt: null, ts: Number.MAX_VALUE }));
      } else if (final.length > 0) {
        final.forEach(f => f.resolve());
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
        const result = await pullSet();
        if (result.done)
          this.push(null);
        else
          this.push(result.value);
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

function teeBalancer(params, numStreams) {
  let resolvePush = null;
  const pending = [];
  for (let s = 0; s < numStreams; ++s)
    pending.push({ frames: null, resolve: null, final: false });
  
  const pullFrame = async index => {
    return new Promise(resolve => {
      if (pending[index].frames) {
        resolve({ value: pending[index].frames, done: false });
        Object.assign(pending[index], { frames: null, resolve: null });
      } else if (pending[index].final)
        resolve({ done: true });
      else
        pending[index].resolve = resolve;

      if (resolvePush && pending.every(p => null === p.frames)) {
        resolvePush();
        resolvePush = null;
      }
    });
  };

  const readStreams = [];
  for (let s = 0; s < numStreams; ++s)
    readStreams.push(new Readable({
      objectMode: true,
      highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
      read() {
        (async () => {
          const result = await pullFrame(s);
          if (result.done)
            this.push(null);
          else
            this.push(result.value);
        })();
      },
    }));

  readStreams.pushFrames = async frames => {
    return new Promise(resolve => {
      pending.forEach((p, index) => {
        if (frames.length)
          p.frames = frames[index].frames;
        else
          p.final = true;
      });

      pending.forEach(p => {
        if (p.resolve) {
          if (p.frames)
            p.resolve({ value: p.frames, done: false });
          else if (p.final)
            p.resolve({ done: true });
        }
        Object.assign(p, { frames: null, resolve: null });
      });
      resolvePush = resolve;
    });
  };

  return readStreams;
}

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

function genToStream(params, gen, reject) {
  return new Readable({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    read() {
      (async () => {
        const result = await gen.next();
        if (result.done)
          this.push(null);
        else
          this.push(result.value);
      })().catch(err => reject(err));
    }
  });
}

function createTransform(params, processFn, flushFn, reject) {
  return new Transform({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    transform(val, encoding, cb) {
      (async () => cb(null, await processFn(val)))().catch(cb);
    },
    flush(cb) {
      (async () => cb(null, flushFn ? await flushFn() : null))().catch(cb);
    }
  }).on('error', err => reject(err));
}

function createWriteStream(params, processFn, finalFn, reject) {
  return new Writable({
    objectMode: true,
    highWaterMark: params.highWaterMark ? params.highWaterMark || 4 : 4,
    write(val, encoding, cb) {
      (async () => cb(null, await processFn(val)))().catch(cb);
    },
    final(cb) {
      (async () => cb(null, finalFn ? await finalFn() : null))().catch(cb);
    }
  }).on('error', err => reject(err));
}

function runStreams(streamType, sources, filterer, streams, mux, muxBalancer) {
  if (!sources.length) 
    return Promise.resolve();

  return new Promise((resolve, reject) => {
    const timeBaseStream = sources[0].format.streams[sources[0].streamIndex];
    const filterBalancer = parallelBalancer({ highWaterMark : 2 }, streamType, sources.length);

    sources.forEach((src, srcIndex) => {
      const srcStream = genToStream({ highWaterMark : 2 }, srcPktsGen(src.format.url, src.ms, src.streamIndex), reject);
      const decStream = createTransform({ highWaterMark : 2 }, async pkts => src.decoder.decode(pkts), () => src.decoder.flush(), reject);
      const filterSource = createWriteStream({ highWaterMark : 2 },
        pkts => filterBalancer.pushPkts(pkts.frames, src.format.streams[src.streamIndex], srcIndex),
        () => filterBalancer.pushPkts([], src.format.streams[src.streamIndex], srcIndex, true), reject);

      srcStream.pipe(decStream).pipe(filterSource);
    });

    const streamTee = teeBalancer({ highWaterMark : 2 }, streams.length);
    const filtStream = createTransform({ highWaterMark : 2 }, async frms => filterer.filter(frms), () => {}, reject);
    const streamSource = createWriteStream({ highWaterMark : 2 },
      frms => streamTee.pushFrames(frms), () => streamTee.pushFrames([], true), reject);

    filterBalancer.pipe(filtStream).pipe(streamSource);

    streams.forEach((str, i) => {
      const dicer = new frameDicer(str.stream);
      const diceStream = createTransform({ highWaterMark : 2 },
        async frms => diceFrames(dicer, frms), () => diceFrames(dicer, [], true), reject);
      const encStream = createTransform({ highWaterMark : 2 }, async frms => str.encoder.encode(frms), () => str.encoder.flush(), reject);
      const muxStream = createWriteStream({ highWaterMark : 2 }, 
        pkts => writeMux(mux, pkts.packets, str.stream.index, timeBaseStream, str.stream, muxBalancer),
        () => writeMux(mux, [], str.stream.index, timeBaseStream, str.stream, muxBalancer, true), reject);
      muxStream.on('finish', resolve);

      streamTee[i].pipe(diceStream).pipe(encStream).pipe(muxStream);
    });
  });
}

function retrieveFormats(sources, srcFmts) {
  return sources.reduce((fmts, src) => {
    if (!fmts.find(f => f.url === src.url))
      fmts.push({ url: src.url, fmt: redisio.retrieveFormat(src.url) });
    return fmts;
  }, srcFmts);
}

async function makeStreams(params) {
  if (!params.video) params.video = [];
  if (!params.audio) params.audio = [];

  let srcFmts = [];
  params.video.forEach(p => srcFmts = retrieveFormats(p.sources, srcFmts));
  params.audio.forEach(p => srcFmts = retrieveFormats(p.sources, srcFmts));
  const formats = await Promise.all(srcFmts.map(f => f.fmt));

  params.video.forEach(p => p.sources.forEach(src => src.format = formats.find(f => f.url === src.url)));
  params.audio.forEach(p => p.sources.forEach(src => src.format = formats.find(f => f.url === src.url)));

  params.video.forEach(p => {
    p.sources.forEach(src =>
      src.decoder = beamcoder.decoder({ demuxer: src.format, stream_index: src.streamIndex }));
  });
  params.audio.forEach(p => {
    p.sources.forEach(src =>
      src.decoder = beamcoder.decoder({ demuxer: src.format, stream_index: src.streamIndex }));
  });

  params.video.forEach(async p => {
    p.filter = await beamcoder.filterer({ // Create a filterer for video
      filterType: 'video',
      inputParams: p.sources.map((src, i) => {
        const stream = src.format.streams[src.streamIndex];
        return {
          name: `in${i}:v`,
          width: stream.codecpar.width,
          height: stream.codecpar.height,
          pixelFormat: stream.codecpar.format,
          timeBase: stream.time_base,
          pixelAspect: stream.sample_aspect_ratio };
      }),
      outputParams: p.streams.map((str, i) => { return { name: `out${i}:v`, pixelFormat: 'yuv422p' }; }),
      filterSpec: p.filterSpec });
    // console.log(p.filter.graph.dump());
  });

  params.video.forEach(async p => {
    // use first video stream for output time base, sample aspect ratio
    const vidStr = p.sources[0].format.streams[p.sources[0].streamIndex];

    p.streams.forEach(str => {
      str.encoder = beamcoder.encoder({
        name: 'libx264',
        width: str.width,
        height: str.height,
        pix_fmt: 'yuv422p',
        sample_aspect_ratio: vidStr.sample_aspect_ratio,
        time_base: vidStr.time_base,
        // framerate: [vidStr.time_base[1], vidStr.time_base[0]],
        // bit_rate: 2000000,
        // gop_size: 10,
        // max_b_frames: 1,
        // priv_data: { preset: 'slow' }
        priv_data: { crf: 23 } }); // ... more required ...
    });
  });

  params.audio.forEach(async p => {
    p.filter = await beamcoder.filterer({ // Create a filterer for audio
      filterType: 'audio',
      inputParams: p.sources.map((src, i) => {
        const stream = src.format.streams[src.streamIndex];
        return {
          name: `in${i}:a`,
          sampleRate: src.decoder.sample_rate,
          sampleFormat: src.decoder.sample_fmt,
          channelLayout: src.decoder.channel_layout,
          timeBase: stream.time_base };
      }),
      outputParams: p.streams.map((str, i) => { 
        return { 
          name: `out${i}:a`,
          sampleRate: str.sampleRate,
          sampleFormat: 'fltp',
          channelLayout: 'mono' }; }), //audDec.channel_layout }],
      filterSpec: p.filterSpec });
    // console.log(p.filter.graph.dump());
  });

  params.audio.forEach(async p => {
    p.streams.forEach(str => {
      str.encoder = beamcoder.encoder({
        name: 'aac',
        sample_fmt: 'fltp',
        sample_rate: str.sampleRate,
        channels: 1, //audDec.channels,
        channel_layout: 'mono', //audDec.channel_layout,
        flags: { GLOBAL_HEADER: true } });
    });
  });

  const mux = beamcoder.muxer({ format_name: 'mp4' });

  params.video.forEach(p => {
    // use first video stream for time base, sample aspect ratio
    const vidStr = p.sources[0].format.streams[p.sources[0].streamIndex];

    p.streams.forEach(s => {
      let oVidStr = mux.newStream({
        name: 'h264',
        time_base: [1, 90000],
        sample_aspect_ratio: vidStr.sample_aspect_ratio,
        interleaved: true }); // Set to false for manual interleaving, true for automatic
      Object.assign(oVidStr.codecpar, {
        width: s.width,
        height: s.height,
        format: 'yuv422p',
        sample_aspect_ratio: vidStr.sample_aspect_ratio,
        field_order: vidStr.codecpar.field_order,
        color_space: 'bt709' }); // ... how much is required ?
      s.stream = oVidStr;
    });
  });

  params.audio.forEach(p => {
    p.streams.forEach(s => {
      let oAudStr = mux.newStream({
        name: 'aac',
        time_base: [1, 90000],
        interleaved: true }); // Set to false for manual interleaving, true for automatic
      Object.assign(oAudStr.codecpar, {
        sample_rate: s.sampleRate,
        format: 's32',
        frame_size: 1024,
        channels: 1,
        channel_layout: 'mono' });
      s.stream = oAudStr;
    });
  });

  await mux.openIO({
    url: 'file:temp.mp4'
  });
  await mux.writeHeader();

  const muxBalancer = serialBalancer(mux.streams.length);
  const muxStreams = [];
  params.video.forEach(p => muxStreams.push(runStreams('video', p.sources, p.filter, p.streams, mux, muxBalancer)));
  params.audio.forEach(p => muxStreams.push(runStreams('audio', p.sources, p.filter, p.streams, mux, muxBalancer)));
  await Promise.all(muxStreams);

  await mux.writeTrailer();
}

async function testStreams() {
  const spec = mediaSpec.parseMediaSpec('50s-52s');
  const urls = [ 'file:../../Media/dpp/AS11_DPP_HD_EXAMPLE_1.mxf', 'file:../../Media/big_buck_bunny_1080p_h264.mov' ];

  const params = {
    video: [
      {
        sources: [
          { ms: spec, url: urls[0], streamIndex: 0 },
          // { ms: spec, url: urls[1], streamIndex: 0 },
        ],
        // filterSpec: '[in0:v] scale=1280:720 [left]; [in1:v] scale=640:360 [right]; [left][right] overlay=format=auto:x=640 [out0:v]',
        filterSpec: '[in0:v] scale=1280:720, colorspace=all=bt709 [out0:v]',
        streams: [
          { width: 1280, height: 720 }
        ]
      }
    ],
    audio: [
      {
        sources: [
          // { ms: spec, url: urls[1], streamIndex: 2 },
          { ms: spec, url: urls[0], streamIndex: 2 },
        ],
        filterSpec: '[in0:a] aformat=sample_fmts=fltp:channel_layouts=mono [out0:a]',
        // filterSpec: '[in0:a][in1:a] join=inputs=2:channel_layout=stereo [out0:a]',
        streams: [
          { sampleRate: 48000 }
        ]
      },
      {
        sources: [
          { ms: spec, url: urls[0], streamIndex: 3 },
          { ms: spec, url: urls[0], streamIndex: 4 },
        ],
        // filterSpec: '[in0:a] aformat=sample_fmts=fltp:channel_layouts=mono [out0:a]',
        filterSpec: '[in0:a][in1:a] join=inputs=2:channel_layout=stereo [out0:a]',
        streams: [
          { sampleRate: 48000 }
        ]
      }
    ]
  };

  await makeStreams(params);
}

console.log('Running testStreams');
let start = Date.now();
testStreams()
  .then(() => { console.log(`Finished ${Date.now() - start}ms`); process.exit(0); })
  .catch(err => { console.log(err), process.exit(1); });
