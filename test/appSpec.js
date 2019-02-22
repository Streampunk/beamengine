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
const testUtil = require('./testUtil.js');

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

/* test('List contents', async t => {
  try {
    let response = await request(server).get('/beams')
      .expect(200)
      .expect([]);
    t.ok(response.ok, 'empty response is OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');

    let fmt = beamcoder.format({ url: 'test_url' });
    t.deepEqual(await redisio.storeFormat(fmt), ['OK'], 'stored test format.');
    response = await request(server).get('/beams')
      .expect(200)
      .expect([ 'test_url' ]);
    t.ok(response.ok, 'content now has the expected item.');

    t.comment('### Checking default range request.');
    t.ok(await flushdb(), 'database flushed OK.');
    for ( let x = 0 ; x < 100 ; x++ ) {
      fmt = beamcoder.format({ url: `test_url_${x}` });
      await redisio.storeFormat(fmt);
    }
    response = await request(server).get('/beams')
      .expect(200);
    t.ok(Array.isArray(response.body), 'response body is an array ...');
    t.equal(response.body.length, 10, '... of the default limit length of 10.');
    t.equal(response.body[0], 'test_url_0', 'first element as expected.');
    t.equal(response.body[9], 'test_url_9', 'last element as expected.');

    t.comment('### Limit and offset.');
    response = await request(server).get('/beams?offset=10&limit=42')
      .expect(200);
    t.ok(Array.isArray(response.body), 'response body is an array ...');
    t.equal(response.body.length, 42, '... of the set limit length of 42.');
    t.equal(response.body[0], 'test_url_10', 'first element as expected.');
    t.equal(response.body[41], 'test_url_51', 'last element as expected.');

    t.comment('### Overshoot limit.');
    response = await request(server).get('/beams?limit=256')
      .expect(200);
    t.ok(Array.isArray(response.body), 'response body is an array ...');
    t.equal(response.body.length, 100, '... of the set limit length of 100.');
    t.equal(response.body[0], 'test_url_0', 'first element as expected.');
    t.equal(response.body[99], 'test_url_99', 'last element as expected.');

    t.comment('### Offset based on the end of the list');
    response = await request(server).get('/beams?offset=-5')
      .expect(200);
    t.ok(Array.isArray(response.body), 'response body is an array ...');
    t.equal(response.body.length, 5, '... of the expected length of 5 < limit.');
    t.equal(response.body[0], 'test_url_95', 'first element as expected.');
    t.equal(response.body[4], 'test_url_99', 'last element as expected.');
  } catch (err) {
    console.log(err.stack);
    t.fail(err);
  }
  t.end();
});

const stripNewStream = ({ newStream , ...other }) => ({ ...other });  // eslint-disable-line no-unused-vars

test('GET a format', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Retrieve a format');
    t.deepEqual(await redisio.storeFormat(testUtil.fmt), ['OK','OK','OK'],
      'test format stored.');
    let response = await request(server).get('/beams/test_url').expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    let fmt = beamcoder.format(response.body);
    t.deepEqual(stripNewStream(fmt), stripNewStream(testUtil.fmt), 'roundtrip equal.');

    t.comment('### Retrive a format that does not exist.');
    response = await request(server).get('/beams/wibble').expect(404);
    t.notOk(response.ok, 'response is not OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.deepEqual(response.body, { statusCode: 404,
      error: 'Not Found',
      message: `Format with name 'wibble' was not found: Unable to retrieve a format with key 'beamengine:wibble'.` },  // eslint-disable-line
    'error message structure as expected.');

  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET a stream', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Retrieve a stream');
    t.deepEqual(await redisio.storeFormat(testUtil.fmt), ['OK','OK','OK'],
      'test format stored.');
    let response = await request(server).get('/beams/test_url/stream_1').expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    let fmt = beamcoder.format();
    let str = fmt.newStream(response.body);
    str.__index(response.body.index);
    t.deepEqual(str.toJSON(), testUtil.stream1, 'roundtrip equal.');

    t.comment('### Retrive a stream that does not exist.');
    response = await request(server).get('/beams/test_url/stream_42').expect(404);
    t.notOk(response.ok, 'response is not OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.deepEqual(response.body, { statusCode: 404,
      error: 'Not Found',
      message: `Stream with name 'test_url:stream_42' was not found: Unable to retrieve a stream with key 'beamengine:test_url:stream_42'.` },  // eslint-disable-line
    'error message structure as expected.');

    t.comment('### Retrieve a stream by type - video');
    response = await request(server).get('/beams/test_url/video').expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    fmt = beamcoder.format();
    str = fmt.newStream(response.body);
    str.__index(response.body.index);
    t.deepEqual(str.toJSON(), testUtil.stream0, 'roundtrip equal.');

    t.comment('### Retrieve a stream by type and index - audio_0');
    response = await request(server).get('/beams/test_url/audio_0').expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    fmt = beamcoder.format();
    str = fmt.newStream(response.body);
    str.__index(response.body.index);
    t.deepEqual(str.toJSON(), testUtil.stream1, 'roundtrip equal.');

  } catch (err) {
    t.fail(err);
  }
  t.end();
}); */

const stripSize = ({ size, ...other }) => ({ ...other, buf_size: size }); // eslint-disable-line no-unused-vars

test('GET a packet', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Retrieve a packet');
    t.deepEqual(await redisio.storeMedia('test_url', testUtil.pkt), ['OK','OK'],
      'test packet stored OK.');
    let response = await request(server).get('/beams/test_url/stream_3/42')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.ok(Array.isArray(response.body), 'result is an array.');
    let pkt = beamcoder.packet(response.body[0]);
    t.ok(pkt, 'roundtrip packet is truthy.');
    t.deepEqual(pkt.toJSON(), stripSize(testUtil.pkt.toJSON()),
      'retrieved packet as expected.');
    t.equal(pkt.buf_size, 16383, 'has expected buf_size parameter.');

    t.comment('### Packet not found');
    response = await request(server).get('/beams/test_url/stream_3/41')
      .expect(404);
    t.notOk(response.ok, 'response is not OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.deepEqual(response.body, { statusCode: 404,
      error: 'Not Found',
      message: `Stream with name 'test_url:stream_42' was not found: Unable to retrieve a stream with key 'beamengine:test_url:stream_42'.` },  // eslint-disable-line
    'error message structure as expected.');


  } catch (err) {
    t.fail(err);
  }
  t.end();
});

/* test('GET a frame', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET packet data', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET frame data', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('POST a format', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('PUT a packet', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('PUT a frame', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('Update a format', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('Stream as live', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
}); */
