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
const mappings = require('../lib/mappings.js');
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
  t.ok(await beforeTest(), 'test database flushed OK.');
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

  let rpkt = await redisio.retrievePacket('test_url', 3, 42);
  t.ok(rpkt, 'roundtrip packet is truthy.');
  pkt.buf_size = pkt.size;
  t.deepEqual(rpkt, pkt, 'roundtrip packet is the same.');

  t.equal(await redis.del(`${config.redis.prepend}:test_url:stream_3:packet_42:data`),
    1, 'deleted the data.');
  rpkt = await redisio.retrievePacket('test_url', 3, 42);
  t.ok(rpkt, 'roundtrip packet without data is truthy.');
  t.equal(rpkt.size, 0, 'data size reset to zero.');
  t.notOk(rpkt.data, 'data is now null.');
  pkt.data = null;
  t.deepEqual(rpkt.toJSON(), pkt.toJSON(),
    'data-stripped packet and original have the same base.');

  redisio.redisPool.recycle(redis);
  await redisio.close();
  t.equal(redisio.redisPool.size(), 0, 'redis pool is reset.');
  t.end();
});

const stripAlloc = ({ alloc, ...other }) => ({ ...other }); // eslint-disable-line no-unused-vars
const stripBufSizes = ({ buf_sizes, ...other }) => ({ ...other });  // eslint-disable-line no-unused-vars

test('Frame store and retrieve', async t => {
  t.ok(await beforeTest(), 'test database flushed OK.');
  let frm = beamcoder.frame({
    pts: 42,
    width: 1920,
    height: 1080,
    format: 'yuv422p'
  }).alloc();
  t.deepEqual(await redisio.storeMedia('test_url', frm, 2), ['OK','OK','OK','OK'],
    'redis reports store of frame and data OK.');
  // console.log(redisio.redisPool.pool[0].options);
  let redis = await redisio.redisPool.use();
  t.ok(await redis.exists(`${config.redis.prepend}:test_url:stream_2:frame_42`),
    'frame key created.');
  t.ok(await redis.exists(`${config.redis.prepend}:test_url:stream_2:frame_42:data_0`),
    'data_0 key created.');
  t.ok(await redis.exists(`${config.redis.prepend}:test_url:stream_2:frame_42:data_1`),
    'data_1 key created.');
  t.ok(await redis.exists(`${config.redis.prepend}:test_url:stream_2:frame_42:data_2`),
    'data_2 key created.');
  t.ok(await redis.ttl(`${config.redis.prepend}:test_url:stream_2:frame_42:data_0`) > 0,
    'data_0 TTL is set.');
  t.ok(await redis.ttl(`${config.redis.prepend}:test_url:stream_2:frame_42:data_1`) > 0,
    'data_1 TTL is set.');
  t.ok(await redis.ttl(`${config.redis.prepend}:test_url:stream_2:frame_42:data_2`) > 0,
    'data_2 TTL is set.');
  t.deepEqual(
    await redis.zrange(`${config.redis.prepend}:test_url:stream_2:index`,
      0, -1, 'WITHSCORES'), // finds all packets
    [ `${config.redis.prepend}:test_url:stream_2:frame_42`, '42' ],
    'stores expected score and key into index.');

  let rfrm = await redisio.retrieveFrame('test_url', 2, 42);
  t.ok(rfrm, 'roundtrip frame is truthy.');
  frm.buf_sizes = frm.data.map(x => x.length);
  t.deepEqual(stripAlloc(rfrm), stripAlloc(frm), 'roundtrip frame is the same.');

  t.equal(await redis.del(`${config.redis.prepend}:test_url:stream_2:frame_42:data_0`),
    1, 'deleted data_0.');
  t.equal(await redis.del(`${config.redis.prepend}:test_url:stream_2:frame_42:data_1`),
    1, 'deleted data_1.');
  t.equal(await redis.del(`${config.redis.prepend}:test_url:stream_2:frame_42:data_2`),
    1, 'deleted data_2.');
  rfrm = await redisio.retrieveFrame('test_url', 2, 42);
  t.ok(rfrm, 'roundtrip frame without data is truthy.');
  t.ok(Array.isArray(frm.data), 'data is now an empty array ...');
  t.equal(rfrm.data.length, 0, '... of length zero.');
  frm.data = null;
  t.deepEqual(stripBufSizes(rfrm.toJSON()), frm.toJSON(),
    'data-stripped frame and original have the same base.');

  await redisio.redisPool.recycle(redis);
  await redisio.close();
  t.equal(redisio.redisPool.size(), 0, 'redis pool is reset.');
  t.end();
});

test('Yet another test', t => { t.end(); });
