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
const redisio = require('../lib/redisio.js');
const beamcoder = require('beamcoder');
const config = require('../config.json');

const beforeTest = async () => {
  redisio.redisPool.testing = true;
  let redis = await redisio.redisPool.use();
  let result = await redis.flushdb();
  redisio.redisPool.recycle(redis);
  return result === 'OK';
};

test.onFinish(() => {
  console.log('Disabling database test mode.');
  redisio.redisPool.testing = false;
});

test('Packet store and retrieve', async t => {
  t.ok(beforeTest(), 'test database flushed OK.');
  let pkt = beamcoder.packet({
    pts: 42,
    dts: 43,
    data: Buffer.alloc(16383),
    stream_index: 3,
    flags: { KEY: true, TRUSTED: true},
    side_data: { replaygain: Buffer.from('Zen time?') },
    duration: 44,
    pos: 45
  });
  t.deepEqual(await redisio.storeMedia('test_url', pkt), [ 'OK', 'OK' ],
    'redis reports store of packet and data OK.');
  t.equal(redisio.redisPool.size(), 5, 'redis pool is populated.');
  let redis = await redisio.redisPool.use();
  t.ok(await redis.exists(`${config.redis.prepend}:test_url:stream_3:packet_42`),
    'packet key created.');
  t.ok(await redis.exists(`${config.redis.prepend}:test_url:stream_3:packet_42:data`),
    'data key created.');
  t.ok(await redis.ttl(`${config.redis.prepend}:test_url:stream_3:packet_42:data`) > 0,
    'data TTL is set.');
  t.deepEqual(
    await redis.zrange(`${config.redis.prepend}:test_url:stream_3:index`,
      0, -1, 'WITHSCORES'), // finds all packets
    [ `${config.redis.prepend}:test_url:stream_3:packet_42`, '42' ],
    'stores expected score and key into index.');
  redisio.redisPool.recycle(redis);
  await redisio.close();
  t.equal(redisio.redisPool.size(), 0, 'redis pool is reset.');
  t.end();
});

test('Yet another test', t => { t.end(); });
