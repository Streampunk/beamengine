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
const { Readable } = require('stream');
const fs = require('fs');

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

async function makeMP4(job) {
  console.log('Running makeMP4', job.id);
  let start = Date.now();
  try {
    const pp = job.data.path.split('/');
    const url = decodeURIComponent(pp[2]);
    const fmt = await redisio.retrieveFormat(url);
    const ms = mediaSpec.parseMediaSpec(pp[3].slice(0, pp[3].indexOf('.')));

    const vidIndex = 0;
    const audIndex = (0 == fmt.streams[1].codecpar.codec_id) ? 2 : 1;

    const params = {
      video: [
        {
          sources: [
            { url: url, ms: ms, streamIndex: vidIndex }
          ],
          filterSpec: '[in0:v] scale=1280:720, colorspace=all=bt709 [out0:v]',
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
            { url: url, ms: ms, streamIndex: audIndex }
          ],
          filterSpec: '[in0:a] aformat=sample_fmts=fltp:channel_layouts=mono [out0:a]',
          streams: [
            { name: 'aac', time_base: [1, 90000], encoderName: 'aac',
              codecpar: {
                sample_rate: 48000, format: 'fltp', frame_size: 1024,
                channels: 1, channel_layout: 'mono'
              }
            }
          ]
        },
      ],
      out: {
        formatName: 'mp4',
        url: 'file:temp.mp4'
      }
    };
  
    params.video.forEach(p => p.sources.forEach(src => src.format = fmt));
    params.audio.forEach(p => p.sources.forEach(src => src.format = fmt));
  
    params.video.forEach(p => p.sources.forEach(src =>
      src.stream = genToStream({ highWaterMark : 2 }, srcPktsGen(src.url, src.ms, src.streamIndex))));
    params.audio.forEach(p => p.sources.forEach(src =>
      src.stream = genToStream({ highWaterMark : 2 }, srcPktsGen(src.url, src.ms, src.streamIndex))));
  
    await beamcoder.makeStreams(params);

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