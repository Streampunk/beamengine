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
const { codecParToRedis, codecParFromRedis } = require('../lib/mappings.js');

test('Roundtrip codec parameters', async t => {
  let cps = beamcoder.codecParameters({
    name: 'h264',
    width: 1920,
    height: 1080,
    format: 'yuv422p'
  });
  console.log(codecParToRedis(cps));
  let redis = new Redis();
  await redis.del('beam:codecpar:test');
  let start = process.hrtime();
  let cpr = codecParToRedis(cps);
  console.log('codecParToRedis', process.hrtime(start));
  start = process.hrtime();
  await redis.hmset('beam:codecpar:test', cpr);
  console.log('redis set', process.hrtime(start));

  start = process.hrtime();
  let rcpb = await redis.hgetallBuffer('beam:codecpar:test');
  console.log('redis get', process.hrtime(start));

  start = process.hrtime();
  let rcpo = codecParFromRedis(rcpb);
  console.log('codecParFromRedis', process.hrtime(start));

  let rcp = beamcoder.codecParameters(rcpo);
  console.log(rcp);
  t.ok(rcp, 'roundtrip codec parameters is truthy.');
  await redis.quit();
  t.end();
});
