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

const { beamcoder, createBeamStream } = require('../index.js');
const fs = require('fs');

async function run() {
  const srcStream = fs.createReadStream('../../media/dpp/AS11_DPP_HD_EXAMPLE_1.mxf');
  const beamStream = createBeamStream({});
  srcStream.pipe(beamStream);

  let format = await beamcoder.format(beamStream.governor);
  let decoder = await beamcoder.decoder({ format: format, stream : 0 });
  let filterer = await beamcoder.filterer({ format: format, decoder: decoder, description: 'scale=78:24,transpose=cclock' });
  console.log(decoder);
  console.log(filterer);
  for ( let x = 0 ; x < 10 ; x++ ) {
    let packet = await format.readFrame();
    if (packet.stream_index == 0) {
      // console.log(packet);
      let frames = await decoder.decode(packet);
      console.log(frames.frames[0]);
      let filtFrames = await filterer.filter(frames);
      console.log(filtFrames.frames[0]);
    }
  }
}

run();
