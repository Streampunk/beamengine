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
const config = require('../config.json');

const beforeTest = async () => {
  redisio.redisPool.testing = true;
  config.redis.pool = 10;
  let redis = await redisio.redisPool.use();
  let result = await redis.flushdb();
  redisio.redisPool.recycle(redis);
  return result === 'OK';
};

test.onFinish(() => {
  console.log('Disabling database test mode.');
  redisio.redisPool.testing = false;
  redisio.close();
});

const checkCloseResult = x => Array.isArray(x) && x.every(x => x === 'OK');

test('Directly close a redis pool', async t => {
  t.ok(checkCloseResult(await redisio.close()), 'directly closing gives OK result.');
  t.ok(checkCloseResult(await redisio.close()), 'second closing gives OK result.');
  t.end();
});

test('Single connection and then close before recycle', async t => {
  t.ok(await beforeTest(), 'test setup OK.');
  let redis = await redisio.redisPool.use();
  t.equal(redisio.redisPool.size(), 5, 'redis pool filled to size 5.');
  t.ok(redis, 'connection is truthy.');
  redis.set('flip', 'flop');
  // redisio.redisPool.recycle(redis);
  console.log(redis.status);
  t.ok(checkCloseResult(await redisio.close()), 'interupting close gives OK result.');
  console.log(redis);
  t.equal(redisio.redisPool.size(), 0, 'redis pool reset to empty.');
  t.equal(Array.isArray(redisio.redisPool.pool) && redisio.redisPool.pool.length, 0,
    'pool is an empty array.');
  t.equal(Array.isArray(redisio.redisPool.queue) && redisio.redisPool.queue.length, 0,
    'queue is an empty array.');
  t.equal(redisio.redisPool.nextFree, null, 'next free reset to null.');
  t.equal(Array.isArray(redisio.redisPool.allConnections) && redisio.redisPool.allConnections.length, 0,
    'all connections is an empty array.');
  await redis.get('flip').then(
    () => t.fail('Connection should be closed.'),
    e => t.ok(e.message.indexOf('Connection is closed') >= 0, 'connection is closed.'));
  console.log(redis.status);
  redis = await redisio.redisPool.use();
  t.equal(await redis.get('flip'), 'flop', 'interuppted store operation successful.');
  redisio.redisPool.recycle(redis);
  console.log(redisio.redisPool.queue, redisio.redisPool.pool.map(x => x !== null));
  t.end();
});

test('Cause a queue by exceeding the pool size', async t => {
  t.ok(await beforeTest(), 'test setup OK.');
  let checkedOut = [];
  for ( let x = 0 ; x < config.redis.pool ; x++ ) {
    checkedOut.push(await redisio.redisPool.use());
  }
  t.equal(redisio.redisPool.size(), 10, 'redis pool filled to size 10.');
  t.equal(redisio.redisPool.nextFree, config.redis.pool, 'next free is size of pool.');
  let nextProm = redisio.redisPool.use();
  let done = false;
  nextProm.then(() => { done = true; });
  redisio.redisPool.recycle(checkedOut[0]);
  let nextOut = await nextProm;
  t.equal(done, true, 'promise completes after recycling.');
  t.ok(nextOut, 'next out is truthy.');

  redisio.redisPool.recycle(checkedOut[1]);
  done = false;
  nextProm = redisio.redisPool.use().then(x => { done = true; return x; });
  t.ok(await nextProm, 'next out is truthy.');
  t.equal(done, true, 'promise completes after recycling.');

  checkedOut.forEach(x => redisio.redisPool.recycle(x));
  t.end();
});

process.on('SIGINT', () => {
  console.log('Received SIGINT.');
  redisio.redisPool.close().then(() => { console.log('Closed by sigint.'); },
    err => { console.error(err); });
});

test('Test sending a SIGINT', async t => {
  t.ok(await beforeTest(), 'test setup OK.');
  let wait = new Promise(f => { setTimeout(f, 1000); });
  process.emit('SIGINT');
  await wait;
  t.equal(redisio.redisPool.size(), 0, 'after SIGINT, pool is empty.');
  t.end();
});
