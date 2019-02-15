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

const test = require('tape');
const beamcoder = require('beamcoder');
const Redis = require('ioredis');
const { frameToRedis, frameFromRedis } = require('../lib/mappings.js');

test('Roundtrip a frame', async t => {
  let frm = beamcoder.frame({
    pts: 1234567890,
    width: 1920,
    height: 1080,
    format: 'yuv422p',
    metadata: { wibble: 'wobble' },
    side_data: { replaygain : Buffer.from('wombles') }
  }).alloc();
  let redis = new Redis();
  await redis.del(`beam:frame:${frm.pts}`);
  // for ( let x = 0 ; x < 100 ; x++ ) {
  let start = process.hrtime();
  let frmr = frameToRedis(frm.toJSON());
  console.log('frameToRedis', process.hrtime(start));

  start = process.hrtime();

  t.equal(await redis.hmset(`beam:frame:${frm.pts}`, frmr), 'OK',
    'redis reports set OK.');
  console.log('redis set', process.hrtime(start));

  start = process.hrtime();
  let rfrmb = await redis.hgetallBuffer(`beam:frame:${frm.pts}`);
  console.log('redis get', process.hrtime(start));
  start = process.hrtime();
  let rfrm = frameFromRedis(rfrmb);
  console.log('frameFromRedis', process.hrtime(start));
  //console.log(rfrm);

  //}
  let rf = beamcoder.frame(rfrm);
  t.ok(rf, 'roundtrip frame is truthy.');
  await redis.quit();
  t.end();
});

/* test('Frame JSON performance', t => {
  let f = beamcoder.frame();
  for ( let x = 0 ; x < 100 ; x++ ) {
    let start = process.hrtime();
    let j = f.toJSON();
    console.log(process.hrtime(start));
    console.log(j);
  }
  t.end();
}); */

/* test('Buffer roundtrip performance', t => {
  let start = process.hrtime();
  let b = Buffer.from('9876543210');
  for ( let x = 0 ; x < 10000000 ; x++ ) {
    let n = JSON.parse(b);
    if (n != 9876543210) console.error('Blah!');
  }
  console.log('JSON.parse', process.hrtime(start));

  start = process.hrtime();
  for ( let x = 0 ; x < 10000000 ; x++ ) {
    let n = +b.toString();
    if (n != 9876543210) console.error('Blah!');
  }
  console.log('plus toString', process.hrtime(start));

  start = process.hrtime();
  for ( let x = 0 ; x < 10000000 ; x++ ) {
    let n = parseInt(b);
    if (n != 9876543210) console.error('Blah!');
  }
  console.log('parseInt', process.hrtime(start));

  t.end();
}); */
