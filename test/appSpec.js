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

/* test('GET a packet', async t => {
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
      message: `Media with name 'test_url:stream_3:41' was not found: Unable to find requested media elements.` },  // eslint-disable-line
    'error message structure as expected.');

    t.comment('### Retrieve one packet from a range');
    response = await request(server).get('/beams/test_url/stream_3/40-45')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.ok(Array.isArray(response.body), 'result is an array.');
    pkt = beamcoder.packet(response.body[0]);
    t.ok(pkt, 'roundtrip packet is truthy.');
    t.deepEqual(pkt.toJSON(), stripSize(testUtil.pkt.toJSON()),
      'retrieved packet as expected.');
    t.equal(pkt.buf_size, 16383, 'has expected buf_size parameter.');

    t.comment('### Store ten packets');
    t.ok(await flushdb(), 'database flushed OK.');

    for ( let x = 0 ; x < 10 ; x++) {
      let tpkt = testUtil.pkt;
      tpkt.pts = (x * 10) - 40;
      t.deepEqual(await redisio.storeMedia('test_url', tpkt), ['OK','OK'],
        `test packet ${tpkt.pts} stored OK.`);
    }

    t.comment('### Retrieve three by range');
    response = await request(server).get('/beams/test_url/stream_3/-15-15')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.ok(Array.isArray(response.body), 'result is an array.');
    t.deepEqual(response.body.map(x => x.pts), [-10, 0, 10],
      '3 elements with expected timestamp.');

    t.comment('### Fuzzy match a timestamp');
    response = await request(server).get('/beams/test_url/stream_3/-17f')
      .expect(200);
    t.ok(response.ok, '-17f response claims OK.');
    t.equal(response.body[0].pts, -20, '-17f finds closest match.');

    response = await request(server).get('/beams/test_url/stream_3/-14f')
      .expect(200);
    t.ok(response.ok, '-14f response claims OK.');
    t.equal(response.body[0].pts, -10, '-14f finds closest match.');

    response = await request(server).get('/beams/test_url/stream_3/-15f')
      .expect(200);
    t.ok(response.ok, '-15f response claims OK.');
    t.equal(response.body[0].pts, -20, '-15f finds closest match.');

    response = await request(server).get('/beams/test_url/stream_3/-20f')
      .expect(200);
    t.ok(response.ok, '-20f response claims OK.');
    t.equal(response.body[0].pts, -20, '-20f finds exact match.');

    response = await request(server).get('/beams/test_url/stream_3/-1000f')
      .expect(200);
    t.ok(response.ok, '-1000f response claims OK.');
    t.equal(response.body[0].pts, -40, '-1000f finds first packet.');

    response = await request(server).get('/beams/test_url/stream_3/1000f')
      .expect(200);
    t.ok(response.ok, '1000f response claims OK.');
    t.equal(response.body[0].pts, 50, '1000f finds last packet.');

    t.comment('### Fuzzy match a timestamp range');
    response = await request(server).get('/beams/test_url/stream_3/-17f-17')
      .expect(200);
    t.ok(response.ok, '-17f-17 response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [-20, -10, 0, 10],
      '-17f-17 range of four.');

    response = await request(server).get('/beams/test_url/stream_3/-17f-17f')
      .expect(200);
    t.ok(response.ok, '-17f-17d response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [-20, -10, 0, 10],
      '-17f-17f range of four.');

    t.comment('### request by index');
    response = await request(server).get('/beams/test_url/stream_3/first')
      .expect(200);
    t.ok(response.ok, 'first response claims OK.');
    t.equal(response.body[0].pts, -40, 'first finds first packet.');

    response = await request(server).get('/beams/test_url/stream_3/last')
      .expect(200);
    t.ok(response.ok, 'last response claims OK.');
    t.equal(response.body[0].pts, 50, 'last finds last packet.');

    response = await request(server).get('/beams/test_url/stream_3/2nd')
      .expect(200);
    t.ok(response.ok, '2nd response claims OK.');
    t.equal(response.body[0].pts, -30, '2nd finds 2nd packet.');

    response = await request(server).get('/beams/test_url/stream_3/7th-last')
      .expect(200);
    t.ok(response.ok, '7th-last response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [20, 30, 40, 50],
      '7th-last finds expected packets.');

    response = await request(server).get('/beams/test_url/stream_3/first-last')
      .expect(200);
    t.ok(response.ok, 'first-last response claims OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [-40, -30, -20, -10, 0, 10, 20, 30, 40, 50],
      'first-last finds all packets.');

    response = await request(server).get('/beams/test_url/stream_3/first-last?limit=5')
      .expect(200);
    t.ok(response.ok, 'first-last with limit of 5 response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [-40, -30, -20, -10, 0 ],
      'first-last with limit of 5 finds first 5 packets.');

    response = await request(server).get('/beams/test_url/stream_3/first-last?offset=5')
      .expect(200);
    t.ok(response.ok, 'first-last with offset of 5 response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [ 10, 20, 30, 40, 50 ],
      'first-last with offset of 5 finds last 5 packets.');

    t.comment('### Realtime request fails without format');
    response = await request(server).get('/beams/test_url/stream_3/0s')
      .expect(404);
    t.notOk(response.ok, 'realtime 0s request not OK.');
    t.deepEqual(response.body, {
      statusCode: 404,
      error: 'Not Found',
      message:
        'Media with name \'test_url:stream_3:0s\' was not found: Unable to find requested media elements.'
    }, 'has expected error message.');

    t.comment('### Store ten packets with a format for time range tests');
    t.ok(await flushdb(), 'database flushed OK.');
    t.deepEqual(await redisio.storeFormat(testUtil.fmt), ['OK','OK','OK'],
      'test format stored.');
    for ( let x = 0 ; x < 10 ; x++) {
      let tpkt = testUtil.pkt;
      tpkt.pts = (x * 3600) - 14400;
      tpkt.stream_index = 1;
      t.deepEqual(await redisio.storeMedia('test_url', tpkt), ['OK','OK'],
        `test packet ${tpkt.pts} stored OK.`);
    }

    response = await request(server).get('/beams/test_url/audio/0s')
      .expect(200);
    t.ok(response.ok, 'realtime 0s request OK.');
    t.deepEqual(response.body.map(x => x.pts), [0], 'realtime 0s has expected PTS.');

    response = await request(server).get('/beams/test_url/audio/-0.04s')
      .expect(200);
    t.ok(response.ok, 'realtime -0.04s request OK.');
    t.deepEqual(response.body.map(x => x.pts), [-3600], 'realtime -0.04s has expected PTS.');

    response = await request(server).get('/beams/test_url/audio/0.0s-0.1s')
      .expect(200);
    t.ok(response.ok, 'realtime range 0.0s-0.1s request OK.');
    t.deepEqual(response.body.map(x => x.pts), [0, 3600, 7200],
      'realtime range 0.0s-0.1s has expected PTS values.');

    response = await request(server).get('/beams/test_url/audio/0.0s-0.1')
      .expect(200);
    t.ok(response.ok, 'realtime range 0.0s-0.1 (no 2nd s) request OK.');
    t.deepEqual(response.body.map(x => x.pts), [0, 3600, 7200],
      'realtime range 0.0s-0.1 (no 2nd s) has expected PTS values.');

    response = await request(server).get('/beams/test_url/stream_1/-1.0s-10s')
      .expect(200);
    t.ok(response.ok, 'realtime range -1.0s-10s request OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [ -14400, -10800, -7200, -3600, 0, 3600, 7200, 10800, 14400, 18000 ],
      'realtime range -1.0s-10 has expected PTS values.');

    response = await request(server)
      .get('/beams/test_url/stream_1/-1.0s-10s?limit=3')
      .expect(200);
    t.ok(response.ok, 'realtime range -1.0s-10s with limit request OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [ -14400, -10800, -7200 ],
      'realtime range -1.0s-10 with limit has expected PTS values.');

    response = await request(server)
      .get('/beams/test_url/stream_1/-1.0s-10s?offset=2&limit=3')
      .expect(200);
    t.ok(response.ok, 'realtime range -1.0s-10s with offset+limit request OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [ -7200, -3600, 0 ],
      'realtime range -1.0s-10 with offset+limit has expected PTS values.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET a packet directly', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Retrieve a packet directly');
    t.deepEqual(await redisio.storeMedia('test_url', testUtil.pkt), ['OK','OK'],
      'test packet stored OK.');
    let response = await request(server).get('/beams/test_url/stream_3/packet_42')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.notOk(Array.isArray(response.body), 'result is an array.');
    let pkt = beamcoder.packet(response.body);
    t.ok(pkt, 'roundtrip frame is truthy.');
    t.deepEqual(pkt.toJSON(), stripSize(testUtil.pkt.toJSON()),
      'retrieved packet as expected.');
    t.equal(pkt.buf_size, 16383, 'has expected buf_sizes parameter.');

    t.comment('### Packet not found');
    response = await request(server).get('/beams/test_url/stream_3/packet_41')
      .expect(404);
    t.notOk(response.ok, 'response is not OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.deepEqual(response.body, { statusCode: 404,
      error: 'Not Found',
      message: `Packet with name 'test_url:stream_3:packet_41' was not found: Packet in stream 'stream_3' with timestamp '41' is not found.` },  // eslint-disable-line
    'error message structure as expected.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET a frame', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Retrieve a frame');
    t.deepEqual(await redisio.storeMedia('test_url', testUtil.frm, 3),
      ['OK','OK', 'OK', 'OK'], 'test frame stored OK.');
    let response = await request(server).get('/beams/test_url/stream_3/42')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.ok(Array.isArray(response.body), 'result is an array.');
    let frm = beamcoder.frame(response.body[0]);
    t.ok(frm, 'roundtrip frame is truthy.');
    t.deepEqual(frm.toJSON(), frm.toJSON(),
      'retrieved frame as expected.');
    t.deepEqual(frm.buf_sizes, [ 2073664, 1036864, 1036864 ],
      'has expected buf_sizes parameter.');

    t.comment('### Frame not found');
    response = await request(server).get('/beams/test_url/stream_3/41')
      .expect(404);
    t.notOk(response.ok, 'response is not OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.deepEqual(response.body, { statusCode: 404,
      error: 'Not Found',
      message: `Media with name 'test_url:stream_3:41' was not found: Unable to find requested media elements.` },  // eslint-disable-line
    'error message structure as expected.');

    t.comment('### Retrieve one frame from a range');
    response = await request(server).get('/beams/test_url/stream_3/40-45')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.ok(Array.isArray(response.body), 'result is an array.');
    frm = beamcoder.frame(response.body[0]);
    t.ok(frm, 'roundtrip frame is truthy.');
    t.deepEqual(frm.toJSON(), testUtil.frm.toJSON(),
      'retrieved frame as expected.');
    t.deepEqual(frm.buf_sizes, [ 2073664, 1036864, 1036864 ],
      'has expected buf_sizes parameter.');

    t.comment('### Store ten frames');
    t.ok(await flushdb(), 'database flushed OK.');

    for ( let x = 0 ; x < 10 ; x++) {
      let tfrm = testUtil.frm;
      tfrm.pts = (x * 10) - 40;
      tfrm.stream_index = 3;
      t.deepEqual(await redisio.storeMedia('test_url', tfrm), ['OK','OK','OK','OK'],
        `test frame ${tfrm.pts} stored OK.`);
    }

    t.comment('### Retrieve three frames by range');
    response = await request(server).get('/beams/test_url/stream_3/-15-15')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.ok(Array.isArray(response.body), 'result is an array.');
    t.deepEqual(response.body.map(x => x.pts), [-10, 0, 10],
      '3 elements with expected timestamp.');

    t.comment('### Fuzzy match a timestamp');
    response = await request(server).get('/beams/test_url/stream_3/-17f')
      .expect(200);
    t.ok(response.ok, '-17f response claims OK.');
    t.equal(response.body[0].pts, -20, '-17f finds closest match.');

    response = await request(server).get('/beams/test_url/stream_3/-14f')
      .expect(200);
    t.ok(response.ok, '-14f response claims OK.');
    t.equal(response.body[0].pts, -10, '-14f finds closest match.');

    response = await request(server).get('/beams/test_url/stream_3/-15f')
      .expect(200);
    t.ok(response.ok, '-15f response claims OK.');
    t.equal(response.body[0].pts, -20, '-15f finds closest match.');

    response = await request(server).get('/beams/test_url/stream_3/-20f')
      .expect(200);
    t.ok(response.ok, '-20f response claims OK.');
    t.equal(response.body[0].pts, -20, '-20f finds exact match.');

    response = await request(server).get('/beams/test_url/stream_3/-1000f')
      .expect(200);
    t.ok(response.ok, '-1000f response claims OK.');
    t.equal(response.body[0].pts, -40, '-1000f finds first frame.');

    response = await request(server).get('/beams/test_url/stream_3/1000f')
      .expect(200);
    t.ok(response.ok, '1000f response claims OK.');
    t.equal(response.body[0].pts, 50, '1000f finds last frame.');

    t.comment('### Fuzzy match a timestamp range');
    response = await request(server).get('/beams/test_url/stream_3/-17f-17')
      .expect(200);
    t.ok(response.ok, '-17f-17 response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [-20, -10, 0, 10],
      '-17f-17 range of four.');

    response = await request(server).get('/beams/test_url/stream_3/-17f-17f')
      .expect(200);
    t.ok(response.ok, '-17f-17d response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [-20, -10, 0, 10],
      '-17f-17f range of four.');

    t.comment('### request by index');
    response = await request(server).get('/beams/test_url/stream_3/first')
      .expect(200);
    t.ok(response.ok, 'first response claims OK.');
    t.equal(response.body[0].pts, -40, 'first finds first frame.');

    response = await request(server).get('/beams/test_url/stream_3/last')
      .expect(200);
    t.ok(response.ok, 'last response claims OK.');
    t.equal(response.body[0].pts, 50, 'last finds last frame.');

    response = await request(server).get('/beams/test_url/stream_3/2nd')
      .expect(200);
    t.ok(response.ok, '2nd response claims OK.');
    t.equal(response.body[0].pts, -30, '2nd finds 2nd frame.');

    response = await request(server).get('/beams/test_url/stream_3/7th-last')
      .expect(200);
    t.ok(response.ok, '7th-last response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [20, 30, 40, 50],
      '7th-last finds expected frames.');

    response = await request(server).get('/beams/test_url/stream_3/first-last')
      .expect(200);
    t.ok(response.ok, 'first-last response claims OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [-40, -30, -20, -10, 0, 10, 20, 30, 40, 50],
      'first-last finds all frames.');

    response = await request(server).get('/beams/test_url/stream_3/first-last?limit=5')
      .expect(200);
    t.ok(response.ok, 'first-last with limit of 5 response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [-40, -30, -20, -10, 0 ],
      'first-last with limit of 5 finds first 5 frames.');

    response = await request(server).get('/beams/test_url/stream_3/first-last?offset=5')
      .expect(200);
    t.ok(response.ok, 'first-last with offset of 5 response claims OK.');
    t.deepEqual(response.body.map(x => x.pts), [ 10, 20, 30, 40, 50 ],
      'first-last with offset of 5 finds last 5 frames.');

    t.comment('### Realtime request fails without format');
    response = await request(server).get('/beams/test_url/stream_3/0s')
      .expect(404);
    t.notOk(response.ok, 'realtime 0s request not OK.');
    t.deepEqual(response.body, {
      statusCode: 404,
      error: 'Not Found',
      message:
        'Media with name \'test_url:stream_3:0s\' was not found: Unable to find requested media elements.'
    }, 'has expected error message.');

    t.comment('### Store ten frames with a format for time range tests');
    t.ok(await flushdb(), 'database flushed OK.');
    t.deepEqual(await redisio.storeFormat(testUtil.fmt), ['OK','OK','OK'],
      'test format stored.');
    for ( let x = 0 ; x < 10 ; x++) {
      let tfrm = testUtil.frm;
      tfrm.pts = (x * 3600) - 14400;
      tfrm.stream_index = 1;
      t.deepEqual(await redisio.storeMedia('test_url', tfrm), ['OK','OK','OK','OK'],
        `test frame ${tfrm.pts} stored OK.`);
    }

    response = await request(server).get('/beams/test_url/audio/0s')
      .expect(200);
    t.ok(response.ok, 'realtime 0s request OK.');
    t.deepEqual(response.body.map(x => x.pts), [0], 'realtime 0s has expected PTS.');

    response = await request(server).get('/beams/test_url/audio/-0.04s')
      .expect(200);
    t.ok(response.ok, 'realtime -0.04s request OK.');
    t.deepEqual(response.body.map(x => x.pts), [-3600], 'realtime -0.04s has expected PTS.');

    response = await request(server).get('/beams/test_url/audio/0.0s-0.1s')
      .expect(200);
    t.ok(response.ok, 'realtime range 0.0s-0.1s request OK.');
    t.deepEqual(response.body.map(x => x.pts), [0, 3600, 7200],
      'realtime range 0.0s-0.1s has expected PTS values.');

    response = await request(server).get('/beams/test_url/audio/0.0s-0.1')
      .expect(200);
    t.ok(response.ok, 'realtime range 0.0s-0.1 (no 2nd s) request OK.');
    t.deepEqual(response.body.map(x => x.pts), [0, 3600, 7200],
      'realtime range 0.0s-0.1 (no 2nd s) has expected PTS values.');

    response = await request(server).get('/beams/test_url/stream_1/-1.0s-10s')
      .expect(200);
    t.ok(response.ok, 'realtime range -1.0s-10s request OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [ -14400, -10800, -7200, -3600, 0, 3600, 7200, 10800, 14400, 18000 ],
      'realtime range -1.0s-10 has expected PTS values.');

    response = await request(server)
      .get('/beams/test_url/stream_1/-1.0s-10s?limit=3')
      .expect(200);
    t.ok(response.ok, 'realtime range -1.0s-10s with limit request OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [ -14400, -10800, -7200 ],
      'realtime range -1.0s-10 with limit has expected PTS values.');

    response = await request(server)
      .get('/beams/test_url/stream_1/-1.0s-10s.json?offset=2&limit=3')
      .expect(200);
    t.ok(response.ok, 'realtime range -1.0s-10s with offset+limit request OK.');
    t.deepEqual(response.body.map(x => x.pts),
      [ -7200, -3600, 0 ],
      'realtime range -1.0s-10 with offset+limit has expected PTS values.');

  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET a frame directly', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Retrieve a frame directly');
    t.deepEqual(await redisio.storeMedia('test_url', testUtil.frm, 3),
      ['OK','OK', 'OK', 'OK'], 'test frame stored OK.');
    let response = await request(server).get('/beams/test_url/stream_3/frame_42')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.notOk(Array.isArray(response.body), 'result is an array.');
    let frm = beamcoder.frame(response.body);
    t.ok(frm, 'roundtrip frame is truthy.');
    t.deepEqual(frm.toJSON(), frm.toJSON(),
      'retrieved frame as expected.');
    t.deepEqual(frm.buf_sizes, [ 2073664, 1036864, 1036864 ],
      'has expected buf_sizes parameter.');

    t.comment('### Frame not found');
    response = await request(server).get('/beams/test_url/stream_3/frame_41')
      .expect(404);
    t.notOk(response.ok, 'response is not OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.deepEqual(response.body, { statusCode: 404,
      error: 'Not Found',
      message: `Frame with name 'test_url:stream_3:frame_41' was not found: Frame in stream 'stream_3' with timestamp '41' is not found.` },  // eslint-disable-line
    'error message structure as expected.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET packet data', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.deepEqual(await redisio.storeMedia('test_url', testUtil.pkt), ['OK','OK'],
      'test packet stored OK.');

    t.comment('### Get packet data using .raw');
    let response = await request(server).get('/beams/test_url/stream_3/packet_42.raw')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length, 16383, 'buffer has expected length.');
    t.equal(response.headers['content-length'], '16383', 'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');

    t.comment('### Get packet data using /data');
    response = await request(server).get('/beams/test_url/stream_3/packet_42/data')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length, 16383, 'buffer has expected length.');
    t.equal(response.headers['content-length'], '16383', 'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');

    t.comment('### Get packet data with .raw_0');
    response = await request(server).get('/beams/test_url/stream_3/packet_42.raw_0')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length, 16383, 'buffer has expected length.');
    t.equal(response.headers['content-length'], '16383', 'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');

    t.comment('### Get packet data with /data_0');
    response = await request(server).get('/beams/test_url/stream_3/packet_42.raw_0')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length, 16383, 'buffer has expected length.');
    t.equal(response.headers['content-length'], '16383', 'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');

    t.comment('### Packet data not found');
    response = await request(server).get('/beams/test_url/stream_3/packet_41.raw')
      .expect(404);
    t.notOk(response.ok, 'data not found means response not ok.');
    t.equal(response.type, 'application/json', 'error response type is JSON.');
    console.log(response.body);
    t.deepEqual(response.body, {
      statusCode: 404,
      error: 'Not Found',
      message:
        'Packet with name \'test_url:stream_3:packet_41\' was not found: Packet data in stream \'stream_3\' with timestamp \'41\' is not found.'
    }, 'error message as expected.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('GET frame data', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.deepEqual(await redisio.storeMedia('test_url', testUtil.frm, 3),
      ['OK','OK', 'OK', 'OK'], 'test frame stored OK.');

    t.comment('### Get frame data using .raw');
    let response = await request(server).get('/beams/test_url/stream_3/frame_42.raw')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length,  2073664+1036864+1036864,
      'buffer has expected length.');
    t.equal(response.headers['content-length'], `${2073664+1036864+1036864}`,
      'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');
    t.deepEqual(JSON.parse(response.headers['beam-buf-sizes']),
      [ 2073664, 1036864, 1036864 ], 'has expected buffer sizes.');
    t.equals(response.headers['beam-data-index'], '', 'has an empty index.');

    t.comment('### Get frame data using /data');
    response = await request(server).get('/beams/test_url/stream_3/frame_42/data')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length,  2073664+1036864+1036864,
      'buffer has expected length.');
    t.equal(response.headers['content-length'], `${2073664+1036864+1036864}`,
      'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');
    t.deepEqual(JSON.parse(response.headers['beam-buf-sizes']),
      [ 2073664, 1036864, 1036864 ], 'has expected buffer sizes.');
    t.equals(response.headers['beam-data-index'], '', 'has an empty index.');

    t.comment('### Get frame data using .raw_0');
    response = await request(server).get('/beams/test_url/stream_3/frame_42.raw_0')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length,  2073664,
      'buffer has expected length.');
    t.equal(response.headers['content-length'], `${2073664}`,
      'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');
    t.deepEqual(JSON.parse(response.headers['beam-buf-sizes']),
      [ 2073664 ], 'has expected buffer sizes.');
    t.equals(response.headers['beam-data-index'], '0', 'has 0 index.');

    t.comment('### Get frame data using /data_0');
    response = await request(server).get('/beams/test_url/stream_3/frame_42/data_0')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length,  2073664,
      'buffer has expected length.');
    t.equal(response.headers['content-length'], `${2073664}`,
      'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');
    t.deepEqual(JSON.parse(response.headers['beam-buf-sizes']),
      [ 2073664 ], 'has expected buffer sizes.');
    t.equals(response.headers['beam-data-index'], '0', 'has 0 index.');

    t.comment('### Get frame data using .raw_2');
    response = await request(server).get('/beams/test_url/stream_3/frame_42.raw_2')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length, 1036864,
      'buffer has expected length.');
    t.equal(response.headers['content-length'], `${1036864}`,
      'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');
    t.deepEqual(JSON.parse(response.headers['beam-buf-sizes']),
      [ 1036864 ], 'has expected buffer sizes.');
    t.equals(response.headers['beam-data-index'], '2', 'has 2 index.');

    t.comment('### Get frame data using /data_2');
    response = await request(server).get('/beams/test_url/stream_3/frame_42/data_2')
      .expect(200);
    t.ok(response.ok, 'response claims OK.');
    t.equal(response.type, 'application/octet-stream', 'is an octet-stream.');
    t.ok(Buffer.isBuffer(response.body), 'body is a buffer.');
    t.equal(response.body.length,  1036864,
      'buffer has expected length.');
    t.equal(response.headers['content-length'], `${1036864}`,
      'header length correct.');
    t.equal(response.headers['beam-pts'], '42', 'Beam-PTS header as expected.');
    t.deepEqual(JSON.parse(response.headers['beam-buf-sizes']),
      [ 1036864 ], 'has expected buffer sizes.');
    t.equals(response.headers['beam-data-index'], '2', 'has 2 index.');

    t.comment('### Get frame PTS not found');
    response = await request(server).get('/beams/test_url/stream_3/frame_41.raw')
      .expect(404);
    t.notOk(response.ok, 'error response is not OK.');
    t.equal(response.type, 'application/json', 'error message is JSON.');
    t.deepEqual(response.body, {
      statusCode: 404,
      error: 'Not Found',
      message:
        'Frame data with name \'test_url:stream_3:frame_41\' was not found: Frame data in stream \'stream_3\' with timestamp \'41\' is not found, pattern [X,X,X].'
    }, 'has expected error message.');

    t.comment('### Get frame plane not found');
    response = await request(server).get('/beams/test_url/stream_3/frame_42.raw_3')
      .expect(404);
    t.notOk(response.ok, 'error response is not OK.');
    t.equal(response.type, 'application/json', 'error message is JSON.');
    t.deepEqual(response.body, {
      statusCode: 404,
      error: 'Not Found',
      message:
        'Frame data with name \'test_url:stream_3:frame_42\' was not found: Frame data in stream \'stream_3\' with timestamp \'42\' and index \'3\' is not found.'
    }, 'has expected error message.');

  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('Start redirect', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Start request with empty stream');
    let response = await request(server).get('/beams/test_url/stream_0/start')
      .expect(404);
    t.notOk(response.ok, 'response reports not OK.');
    t.ok(response.type, 'application/json', 'error message is JSON.');
    t.deepEqual(response.body, {
      statusCode: 404,
      error: 'Not Found',
      message:
        'Could not find start frame for stream \'test_url:stream_0\': Unable to find requested media elements.'
    }, 'has expected error message.');

    t.deepEqual(await redisio.storeFormat(testUtil.fmt), ['OK','OK','OK'],
      'test format stored.');

    for ( let x = 0 ; x < 10 ; x++) {
      let tpkt = testUtil.pkt;
      tpkt.stream_index = 0;
      tpkt.pts = (x * 10) - 40;
      t.deepEqual(await redisio.storeMedia('test_url', tpkt), ['OK','OK'],
        `test packet ${tpkt.pts} stored OK.`);
    }

    t.comment('### Request start via stream index');
    response = await request(server).get('/beams/test_url/stream_0/start')
      .expect(302);
    t.notOk(response.ok, 'response reports not OK.');
    t.equal(response.headers.location,
      '/beams/test_url/stream_0/packet_-40',
      'redirects to expected location.');

    t.comment('### Request start via stream alias');
    response = await request(server).get('/beams/test_url/video/start')
      .expect(302);
    t.notOk(response.ok, 'response reports not OK.');
    t.equal(response.headers.location,
      '/beams/test_url/stream_0/packet_-40',
      'redirects to expected location.');

  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('End redirect', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### End request with empty stream');
    let response = await request(server).get('/beams/test_url/stream_0/end')
      .expect(404);
    t.notOk(response.ok, 'response reports not OK.');
    t.ok(response.type, 'application/json', 'error message is JSON.');
    t.deepEqual(response.body, {
      statusCode: 404,
      error: 'Not Found',
      message:
        'Could not find end frame for stream \'test_url:stream_0\': Unable to find requested media elements.'
    }, 'has expected error message.');

    t.deepEqual(await redisio.storeFormat(testUtil.fmt), ['OK','OK','OK'],
      'test format stored.');

    for ( let x = 0 ; x < 10 ; x++) {
      let tpkt = testUtil.pkt;
      tpkt.stream_index = 0;
      tpkt.pts = (x * 10) - 40;
      t.deepEqual(await redisio.storeMedia('test_url', tpkt), ['OK','OK'],
        `test packet ${tpkt.pts} stored OK.`);
    }

    t.comment('### Request end via stream index');
    response = await request(server).get('/beams/test_url/stream_0/end')
      .expect(302);
    t.notOk(response.ok, 'response reports not OK.');
    t.equal(response.headers.location,
      '/beams/test_url/stream_0/packet_50',
      'redirects to expected location.');

    t.comment('### Request end via stream alias');
    response = await request(server).get('/beams/test_url/video/end')
      .expect(302);
    t.notOk(response.ok, 'response reports not OK.');
    t.equal(response.headers.location,
      '/beams/test_url/stream_0/packet_50',
      'redirects to expected location.');

    t.comment('### Request latest via stream index');
    response = await request(server).get('/beams/test_url/stream_0/latest')
      .expect(302);
    t.notOk(response.ok, 'response reports not OK.');
    t.equal(response.headers.location,
      '/beams/test_url/stream_0/packet_50',
      'redirects to expected location.');

    t.comment('### Request latest via stream alias');
    response = await request(server).get('/beams/test_url/video/latest')
      .expect(302);
    t.notOk(response.ok, 'response reports not OK.');
    t.equal(response.headers.location,
      '/beams/test_url/stream_0/packet_50',
      'redirects to expected location.');
  } catch (err) {
    t.fail(err);
  }
  t.end();
});

test('POST a format', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Store format with existing URL');
    let fmt = testUtil.fmt;
    let response = await request(server)
      .post('/beams')
      .send(fmt.toJSON())
      .expect(201);
    t.ok(response.ok, 'response reports OK.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    t.deepEqual(response.body, fmt.toJSON(), 'created equals sent.');
    t.equal(response.headers.location, '/beams/test_url',
      'has expected location header.');

    let dbfmt = await redisio.retrieveFormat('test_url');
    t.ok(dbfmt, 'stored format is truthy.');
    t.deepEqual(dbfmt.toJSON(), fmt.toJSON(),
      'stored format matches sent format.');

    t.comment('### Attempting to post same format');
    response = await request(server)
      .post('/beams')
      .send(fmt.toJSON())
      .expect(409);
    t.notOk(response.ok, 'response is not OK.');
    t.equal(response.type, 'application/json', 'error is JSON.');
    t.deepEqual(response.body, {
      statusCode: 409,
      error: 'Conflict',
      message: 'A format with key \'beamengine:test_url\' already exists.'
    }, 'error message is as expected.');

    fmt = testUtil.fmt;
    fmt.url = '';
    response = await request(server)
      .post('/beams')
      .send(fmt.toJSON())
      .expect(201);
    t.ok(response.ok, 'response is OK.');
    t.ok(response.body.url && response.body.url.length > 0, 'has new URL.');
    t.ok(response.headers.location && response.headers.location.startsWith('/beams'),
      'location headers starts correctly.');
    let location = response.headers.location.slice(7).replace(/%3A/g, ':');
    t.equal(location, response.body.url, 'location derived from fmt.url.');

    // TODO test more streams than the threadpool size
  } catch (err) {
    t.fail(err);
  }
  t.end();
}); */

test('PUT a packet', async t => {
  try {
    t.ok(await flushdb(), 'database flushed OK.');

    t.comment('### Store format with existing URL');
    let fmt = testUtil.fmt;
    let response = await request(server)
      .post('/beams')
      .send(fmt.toJSON())
      .expect(201);
    t.ok(response.ok, 'response reports OK.');

    t.comment('Put in a packet for the format with "stream_0"');
    let pkt = testUtil.pkt;
    pkt.stream_index = 0;
    response = await request(server)
      .put('/beams/test_url/stream_0/packet_42')
      .send(pkt.toJSON())
      .expect(201);
    t.ok(response.ok, 'response is truthy.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    let rpkt = beamcoder.packet(response.body);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'returned packet as expected.');
    rpkt = await redisio.retrievePacket('test_url', 0, 42);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'stored packet as expected.');

    t.comment('Put in a packet for the format with "0"');
    pkt = testUtil.pkt;
    pkt.stream_index = 0;
    response = await request(server)
      .put('/beams/test_url/0/packet_42')
      .send(pkt.toJSON())
      .expect(200);
    t.ok(response.ok, 'response is truthy.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    rpkt = beamcoder.packet(response.body);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'returned packet as expected.');
    rpkt = await redisio.retrievePacket('test_url', 0, 42);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'stored packet as expected.');

    t.comment('Put in a packet for the format with "video"');
    pkt = testUtil.pkt;
    pkt.stream_index = 0;
    response = await request(server)
      .put('/beams/test_url/video/packet_42')
      .send(pkt.toJSON())
      .expect(200);
    t.ok(response.ok, 'response is truthy.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    rpkt = beamcoder.packet(response.body);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'returned packet as expected.');
    rpkt = await redisio.retrievePacket('test_url', 0, 42);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'stored packet as expected.');

    t.comment('Put in a packet that does not identify');
    pkt = testUtil.pkt;
    pkt.stream_index = 0;
    response = await request(server)
      .put('/beams/test_url/stream_0/42')
      .send(pkt.toJSON())
      .expect(200);
    t.ok(response.ok, 'response is truthy.');
    t.equal(response.type, 'application/json', 'response is JSON.');
    rpkt = beamcoder.packet(response.body);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'returned packet as expected.');
    rpkt = await redisio.retrievePacket('test_url', 0, 42);
    t.deepEqual(rpkt.toJSON(), stripSize(pkt.toJSON()), 'stored packet as expected.');

    t.comment('Packet stream mismatch error');
    pkt = testUtil.pkt;
    pkt.stream_index = 0;
    response = await request(server)
      .put('/beams/test_url/stream_1/packet_42')
      .send(pkt.toJSON())
      .expect(400);
    t.notOk(response.ok, 'error response is not OK.');
    t.equal(response.type, 'application/json', 'error is JSON.');
    t.deepEqual(response.body, {
      statusCode: 400,
      error: 'Bad Request',
      message: 'Packet stream index \'0\' does not match URL stream index \'1\'.'
    }, 'error message as expected.');

    t.comment('Frame for packet error');
    pkt = testUtil.frm;
    response = await request(server)
      .put('/beams/test_url/stream_0/packet_42')
      .send(pkt.toJSON())
      .expect(400);
    t.notOk(response.ok, 'error response is not OK.');
    t.equal(response.type, 'application/json', 'error is JSON.');
    t.deepEqual(response.body, {
      statusCode: 400,
      error: 'Bad Request',
      message: 'Media element type \'Frame\' does not match URL parameter \'packet_42\'.'
    }, 'error message as expected.');

    t.comment('Post something that is not a packet by type name');
    response = await request(server)
      .put('/beams/test_url/stream_0/packet_42')
      .send({ type: 'Wibble' })
      .expect(400);
    t.notOk(response.ok, 'error response is not OK.');
    t.equal(response.type, 'application/json', 'error is JSON.');
    t.deepEqual(response.body, {
      statusCode: 400,
      error: 'Bad Request',
      message: 'Media element type \'Wibble\' does not match URL parameter \'packet_42\'.'
    }, 'error message as expected.');

    t.comment('Post something that is not a packet by media type');
    response = await request(server)
      .put('/beams/test_url/stream_0/packet_42')
      .send(Buffer.from('Wibbly'))
      .expect(400);
    t.notOk(response.ok, 'error response is not OK.');
    t.equal(response.type, 'application/json', 'error is JSON.');
    t.deepEqual(response.body, {
      statusCode: 400,
      error: 'Bad Request',
      message: 'Media element must be described with JSON.'
    }, 'error message as expected.');

  } catch (err) {
    t.fail(err);
  }
  t.end();
});

/* test('PUT a frame', async t => {
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
