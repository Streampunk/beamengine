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

const test = require('tape');
const { beamcoder } = require('../index.js');

test('Creating an encoder', async t => {
  let enc = await beamcoder.encoder({ name: 'h264' });
  t.ok(enc, 'is truthy.');
  t.equal(enc.name, 'libx264', 'has the expected name.');
  t.equal(enc.codec_id, 27, 'has the expected codec_id.');
  t.ok(typeof enc._CodecContext == 'object', 'external value present.');
  t.equal(enc.type, 'encoder', 'has expected type name.');
  t.end();
});
