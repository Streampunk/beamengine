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
const { packetToRedis, packetFromRedis } = require('../lib/mappings.js');

test('Roundtrip a packet', async t => {
  let data = Buffer.alloc(65536);
  for ( let x = 0 ; x < data.length ; x++ ) data[x] = x % 128;
  let pkt = beamcoder.packet({
    pts: 9876543210,
    dts: null,
    data: data,
    flags: { KEY: true },
    side_data: { replaygain: Buffer.from('wibble') }
  });
  let redis = new Redis();
  await redis.del(`beam:packet:${pkt.pts}`);
  let rdpr = null;
  // for ( let x = 0 ; x < 100 ; x++ ) {
  let start = process.hrtime();
  let pktj = pkt.toJSON();
  console.log('toJSON', process.hrtime(start));
  start = process.hrtime();
  let pktr = packetToRedis(pktj);
  console.log('packetToRedis', process.hrtime(start));
  console.log(pktr);
  start = process.hrtime();
  t.equal(await redis.hmset(`beam:packet:${pkt.pts}`, pktr), 'OK',
    'redis says set OK.');
  console.log('Set took', process.hrtime(start));
  start = process.hrtime();
  let rdp = await redis.hgetallBuffer(`beam:packet:${pkt.pts}`);
  console.log('Get took', process.hrtime(start));
  start = process.hrtime();
  rdpr = packetFromRedis(rdp);
  console.log('packetFromRedis', process.hrtime(start));
  // }
  // t.equal(Buffer.compare(rdp.data, data), 0, 'data has roundtripped OK.');
  let rp = beamcoder.packet(rdpr);
  t.ok(rp, 'roundtrip packet is truthy.');
  console.log(rp);
  await redis.quit();
  t.end();
});
