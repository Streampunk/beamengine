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
const { formatToRedis, formatFromRedis } = require('../lib/mappings.js');

test('Roundtrip format', async t => {
  let fmt = beamcoder.format({
    oformat: 'wav',
    url: 'this_is_my_name',
    priv_data: { peak_block_size: 192 },
    key: Buffer.from('Wibble wobble'),
    metadata: { jelly: 'plate' }
  });
  let redis = new Redis();
  await redis.del('beam:testing:format');

  let start = process.hrtime();
  let fmtr = formatToRedis(fmt.toJSON());
  console.log('formatToRedis', process.hrtime(start));

  start = process.hrtime();
  t.equal(await redis.hmset('beam:testing:format', fmtr), 'OK', 'redis set reports OK.');
  console.log('redis set', console.log(process.hrtime(start)));

  start = process.hrtime();
  let rfmtb = await redis.hgetallBuffer('beam:testing:format');
  console.log('redis get', process.hrtime(start));

  start = process.hrtime();
  let rfmto = formatFromRedis(rfmtb);
  console.log('formatFromRedis', process.hrtime(start));

  let rfmt = beamcoder.format(rfmto);
  t.ok(rfmt, 'roundtrip format is truthy.');

  t.equal(rfmt.oformat.name, 'wav', 'has WAV output format.');
  t.equal(rfmt.priv_data.peak_block_size, 192, 'has expected priv_data.peak_block_size.');
  t.equal(Buffer.compare(rfmt.key, Buffer.from('Wibble wobble')), 0, 'has expected key.');
  t.deepEqual(rfmt.metadata, { jelly: 'plate' }, 'has expected metadata.');

  await redis.quit();
  t.end();
});
