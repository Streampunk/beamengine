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

const beamcoder = require('beamcoder');
const got = require('got');
const p2re = require('path-to-regexp');

const converter = p2re.compile(':name');
const convertToPathPart = n => converter({ name: n });

async function run() {
  let location = 'file:../../Media/dpp/AS11_DPP_HD_EXAMPLE_1.mxf';
  let mxfFmt = await beamcoder.demuxer(location);
  let locPathPart = convertToPathPart(location);
  try {
    let response = await got.post('http://localhost:3000/beams', { // eslint-disable-line no-unused-vars
      body: mxfFmt.toJSON(),
      json: true,
      headers : { 'Content-Type': 'application/json' }
    }).catch(err => {
      if (err.statusCode === 409) {
        console.log('Got conflict: assuming OK.');
      } else {
        throw err;
      }
    });

    let numFrames = 50;
    await mxfFmt.seek({ time: 30 });
    let pkt = await mxfFmt.read();
    for ( ; (pkt != null) && (numFrames != 0) ; pkt = await mxfFmt.read()) {
      if (pkt.stream_index === 0) {
        console.log(pkt);
        numFrames--;
        await got.put(`http://localhost:3000/beams/${locPathPart}/stream_0/packet_${pkt.pts}`, {
          body: pkt.toJSON(),
          json: true
        });
        await got.put(`http://localhost:3000/beams/${locPathPart}/stream_0/packet_${pkt.pts}/data`, {
          body: pkt.data,
          headers: { 'Content-Type': 'application/octet-stream' }
        });
      }
    }
  } catch (err) {
    console.error(err.stack);
  }

}

try {
  run();
} catch (err) {
  console.error(err.stack);
}
