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

const { beamcoder, createDemuxer } = require('../index.js');
const fs = require('fs');

async function run() {
  const srcStream = fs.createReadStream('../../media/dpp/AS11_DPP_HD_EXAMPLE_1.mxf');
  let demuxer = await createDemuxer(srcStream);

  let decoder = await beamcoder.decoder({ name: 'h264' });

  const vidStream = demuxer.streams[0];
  let filterer = await beamcoder.filterer({
    inputParams: [
      {
        name: '0:v',
        width: vidStream.codecpar.width,
        height: vidStream.codecpar.height,
        pixFmt: vidStream.codecpar.format,
        timeBase: vidStream.time_base,
        pixelAspect: vidStream.sample_aspect_ratio,
      },
      {
        name: '1:v',
        width: vidStream.codecpar.width,
        height: vidStream.codecpar.height,
        pixFmt: vidStream.codecpar.format,
        timeBase: vidStream.time_base,
        pixelAspect: vidStream.sample_aspect_ratio,
      }
    ],
    filterSpec: '[0:v] scale=1280:720 [left]; [1:v] scale=640:360 [right]; [left][right] overlay=format=auto:x=640 [out]'
  });

  console.log(decoder);
  console.log(filterer);

  for ( let x = 0 ; x < 10 ; x++ ) {
    let packet = await demuxer.readFrame();
    if (packet.stream_index == 0) {
      // console.log(packet);
      let frames = await decoder.decode(packet);
      console.log(frames);
      let filtFrames = await filterer.filter([
        { name: '0:v', frames: frames },
        { name: '1:v', frames: frames },
      ]);
      console.log(filtFrames);
    }
  }
  let frames = await decoder.flush();
  console.log('flush', frames.totalTime, frames.frames.length);
}

run().catch(console.error);
