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

const beamengine = require('../app.js');
const beamcoder = require('beamcoder');
const test = require('tape');
const config = require('../config.json');
const redisio = require('../lib/redisio.js');
const request = require('supertest');

const server = beamengine.listen(+config.testing.port);

test.onFinish(() => {
  console.log('Closing test server and clearing redis connection pool.');
  redisio.redisPool.testing = false;
  redisio.close();
  server.close();
});

const flushdb = async () => {
  let redis = await redisio.redisPool.use();
  let result = await redis.flushdb();
  redisio.redisPool.recycle(redis);
  return result === 'OK';
};

test('Checking that server is listening', async t => {
  redisio.redisPool.testing = true;
  t.ok(await flushdb(), 'test database flushed OK.');
  if (server.listening) {
    t.pass('Server is already listening.');
    return t.end();
  }
  server.on('listening', () => {
    t.pass('After a while, server is now listening.');
    t.end();
  });
});

test('List contents', async t => {
  try {
    let response = await request(server).get('/content')
      .expect(200)
      .expect([]);
    t.ok(response.ok, 'empty response is OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');

    let fmt = beamcoder.format({ url: 'test_url' });
    t.deepEqual(await redisio.storeFormat(fmt), ['OK'], 'stored test format.');
    response = await request(server).get('/content')
      .expect(200)
      .expect([ 'test_url' ]);
    t.ok(response.ok, 'content now has the expected item.');

    t.comment('### Checking range requests.');
    t.ok(await flushdb(), 'database flushed OK.');
    for ( let x = 0 ; x < 100 ; x++ ) {
      fmt = beamcoder.format({ url: `test_url_${x}` });
      await redisio.storeFormat(fmt);
    }
    response = await request(server).get('/content')
      .expect(200);
    console.log(response.body);
    t.ok(Array.isArray(response.body), 'response body is an array ...');
    t.equal(response.body.length, 10, '... of the default limit length of 10.');

  } catch (err) {
    t.fail(err);
  }
  t.end();
});
