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
const { streamToRedis, streamFromRedis } = require('../lib/mappings.js');

test('Roundtrip a stream', async t => {
  let fmt = beamcoder.format();
  let s = fmt.newStream({
    name:'h264',
    side_data: { replaygain: Buffer.from('Magic Roundabout') },
    metadata: { jelly: 'plate' }
  });
  const redis = new Redis();
  await redis.del('beam:stream:test');

  //for ( let x = 0 ; x < 100 ; x++ ) {
  let start = process.hrtime();
  let sr = streamToRedis(s.toJSON());
  let end = process.hrtime(start);
  console.log('streamToRedis', end);
  start = process.hrtime();

  t.equal(await redis.hmset('beam:stream:test', sr), 'OK', 'reports set OK.');
  console.log('redis set', process.hrtime(start));

  start = process.hrtime();
  let srb = await redis.hgetallBuffer('beam:stream:test');
  console.log('redis get', process.hrtime(start));
  start = process.hrtime();
  let rso = streamFromRedis(srb);
  console.log('streamFromRedis', process.hrtime(start));

  start = process.hrtime();
  let rs = fmt.newStream(rso);
  console.log('Creating stream', process.hrtime(start));
  t.ok(rs, 'roundtrip stream is truthy.');
  //}

  await redis.quit();
  t.end();
});
