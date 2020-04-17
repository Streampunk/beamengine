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
const { Readable } = require('stream');

function filterControl(stream, graph, tag, adjFn) {
  const filt = graph.filters.find(f => f.name.includes(tag));
  return pts => {
    const ts = pts * stream.time_base[0] / stream.time_base[1];
    filt.priv = adjFn(ts);
  };
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
        } catch (err) {
          console.log(err);
          this.push(null);
        }
      })();
    }
  });
}

function retrieveFormats(sources, srcFmts) {
  return sources.reduce((fmts, src) => {
    if (!fmts.find(f => f.url === src.url))
      fmts.push({ url: src.url, fmt: redisio.retrieveFormat(src.url) });
    return fmts;
  }, srcFmts);
}

async function testStreams() {
  const spec = mediaSpec.parseMediaSpec('50s-52s');
  const urls = [ 'file:../../Media/dpp/AS11_DPP_HD_EXAMPLE_1.mxf', 'file:../../Media/big_buck_bunny_1080p_h264.mov' ];

  const params = {
    video: [
      {
        sources: [
          { url: urls[0], ms: spec, streamIndex: 0 },
          { url: urls[1], ms: spec, streamIndex: 0 },
        ],
        filterSpec: '[in0:v] scale=1280:720 [left]; [in1:v] scale@tag1=640:360 [right]; [left][right] overlay=format=auto:x=640 [out0:v]',
        // filterSpec: '[in0:v] scale=1280:720, colorspace=all=bt709 [out0:v]',
        streams: [
          { name: 'h264', time_base: [1, 90000], encoderName: 'libx264',
            codecpar: {
              width: 1280, height: 720, format: 'yuv422p', color_space: 'bt709',
              sample_aspect_ratio: [1, 1]
            }
          }
        ]
      }
    ],
    audio: [
      {
        sources: [
          // { url: urls[1], ms: spec, streamIndex: 2 },
          { url: urls[0], ms: spec, streamIndex: 2 },
        ],
        filterSpec: '[in0:a] aformat=sample_fmts=fltp:channel_layouts=mono [out0:a]',
        // filterSpec: '[in0:a][in1:a] join=inputs=2:channel_layout=stereo [out0:a]',
        streams: [
          { name: 'aac', time_base: [1, 90000], encoderName: 'aac',
            codecpar: {
              sample_rate: 48000, format: 'fltp', frame_size: 1024,
              channels: 1, channel_layout: 'mono'
            }
          }
        ]
      },
      {
        sources: [
          { url: urls[0], ms: spec, streamIndex: 3 },
          { url: urls[0], ms: spec, streamIndex: 4 },
        ],
        // filterSpec: '[in0:a] aformat=sample_fmts=fltp:channel_layouts=mono [out0:a]',
        filterSpec: '[in0:a][in1:a] join=inputs=2:channel_layout=stereo [out0:a]',
        streams: [
          { name: 'aac', time_base: [1, 90000], encoderName: 'aac',
            codecpar: {
              sample_rate: 48000, format: 'fltp', frame_size: 1024,
              channels: 1, channel_layout: 'mono'
            }
          }
        ]
      }
    ],
    out: {
      formatName: 'mp4',
      url: 'file:temp.mp4'
    }
  };

  // Setup source streams and formats
  if (!params.video) params.video = [];
  if (!params.audio) params.audio = [];

  let srcFmts = [];
  params.video.forEach(p => srcFmts = retrieveFormats(p.sources, srcFmts));
  params.audio.forEach(p => srcFmts = retrieveFormats(p.sources, srcFmts));
  params.formats = await Promise.all(srcFmts.map(f => f.fmt));

  params.video.forEach(p => p.sources.forEach(src => src.format = params.formats.find(f => f.url === src.url)));
  params.audio.forEach(p => p.sources.forEach(src => src.format = params.formats.find(f => f.url === src.url)));

  params.video.forEach(p => p.sources.forEach(src =>
    src.stream = genToStream({ highWaterMark : 2 }, srcPktsGen(src.url, src.ms, src.streamIndex))));

  params.audio.forEach(p => p.sources.forEach(src =>
    src.stream = genToStream({ highWaterMark : 2 }, srcPktsGen(src.url, src.ms, src.streamIndex))));

  const beamStreams = await beamcoder.makeStreams(params);

  params.video.forEach(p => {
    const stream = p.sources[0].format.streams[p.sources[0].streamIndex];
    p.filter.cb = filterControl(stream, p.filter.graph, 'tag1', ts => {
      return { width: (ts * 16 - 400).toString() };
    });
  });

  await beamStreams.run();
}

console.log('Running testStreams');
let start = Date.now();
testStreams()
  .then(() => { console.log(`Finished ${Date.now() - start}ms`); process.exit(0); })
  .catch(err => { console.log(err), process.exit(1); });
