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

/* const packetToRedis = ({ pts, dts, size, stream_index, flags,
  duration, pos, side_data }) => {
  let pkt = {
    pts,
    dts,
    size,
    stream_index,
    flags_KEY: flags.KEY,
    flags_CORRUPT: flags.CORRUPT,
    flags_TRUSTED: flags.TRUSTED,
    flags_DISPOSABLE: flags.DISPOSABLE,
    duration,
    pos
  };
  if (side_data) {
    Object.keys(side_data).reduce((l, r) => {
      if (r !== 'type') l[`side_data_${r}`] = side_data[r];
      return l;
    }, pkt);
  }
  return pkt;
}; */

/* const packetToRedis = ({ pts, dts, size, stream_index, flags,
  duration, pos, side_data }) => {
  let pkt = { stream_index };
  if (pts) pkt.pts = pts;
  if (dts) pkt.dts = dts;
  if (size) pkt.size = size;
  if (flags.KEY) { pkt.flags_KEY = flags.KEY; }
  if (pkt.flags_CORRUPT) { pkt.flags_CORRUPT = flags.CORRUPT; }
  if (pkt.flags_TRUSTED) { pkt.flags_TRUSTED = flags.TRUSTED; }
  if (pkt.flags_DISPOSABLE) { pkt.flags_DISPOSABLE = flags.DISPOSABLE; }
  if (duration) pkt.duration = duration;
  if (pos >= 0) pkt.pos = pos;
  if (side_data) {
    Object.keys(side_data).reduce((l, r) => {
      if (r !== 'type') l[`side_data_${r}`] = side_data[r];
      return l;
    }, pkt);
  }
  return pkt;
}; */

/* const packetToRedis = (p) => {
  let pkt = { stream_index: p.stream_index };
  if (p.pts) pkt.pts = p.pts;
  if (p.dts) pkt.dts = p.dts;
  if (p.size) pkt.size = p.size;
  if (p.flags.KEY) { pkt.flags_KEY = p.flags.KEY; }
  if (p.flags.CORRUPT) { pkt.flags_CORRUPT = p.flags.CORRUPT; }
  if (p.flags.TRUSTED) { pkt.flags_TRUSTED = p.flags.TRUSTED; }
  if (p.flags.DISPOSABLE) { pkt.flags_DISPOSABLE = p.flags.DISPOSABLE; }
  if (p.duration) pkt.duration = p.duration;
  if (p.pos >= 0) pkt.pos = p.pos;
  if (p.side_data) {
    Object.keys(p.side_data).reduce((l, r) => {
      if (r !== 'type') l[`side_data_${r}`] = p.side_data[r];
      return l;
    }, pkt);
  }
  return pkt;
}; */

const packetToRedis = ({ type, flags, side_data, ...p }) => { // eslint-disable-line no-unused-vars
  if (flags) {
    p.flags_KEY = flags.KEY;
    p.flags_CORRUPT = flags.CORRUPT;
    p.flags_DISCARD = flags.DISCARD;
    p.flags_TRUSTED = flags.TRUSTED;
    p.flags_DISPOSABLE = flags.DISPOSABLE;
  }
  if (side_data) {
    Object.keys(side_data).reduce((l, r) => {
      if (r !== 'type') l[`side_data_${r}`] = side_data[r];
      return l;
    }, p);
  }
  return p;
};

/* const packetToRedis = p => {
  let pkt = Object.keys(p).reduce((l, r) => {
    switch (typeof p[r]) {
    case 'string':
    case 'number':
    case 'boolean':
      l[r] = p[r];
      break;
    case 'object':
      if (Array.isArray(p[r])) {
        // do array stuff
        break;
      }
      if (Buffer.isBuffer(p[r])) {
        // do buffer stuff
        break;
      }
      Object.keys(p[r]).reduce((x, y) => {
        if (y != 'type') x[`${r}_${y}`] = p[r][y];
        return l;
      }, l);
      break;
    case 'null':
      l[r] = '';
      break;
    case 'undefined':
      break;
    }
    return l;
  }, {});
  return pkt;
}; */

/* const packetFromRedis = ({ pts, dts, stream_index, flags_KEY,
  flags_CORRUPT, flags_TRUSTED, flags_DISPOSABLE, duration, pos, size, ...sd }) => {
  let pkt = {
    pts: pts.length !== 0 ? parseInt(pts.toString()) : null,
    dts: dts.length !== 0 ? parseInt(dts.toString()) : null,
    stream_index: parseInt(stream_index.toString()),
    flags: {
      KEY: flags_KEY[0] === 116, // t
      CORRUPT: flags_CORRUPT[0] === 116,
      TRUSTED: flags_TRUSTED[0] === 116,
      DISPOSABLE: flags_DISPOSABLE[0] === 116
    },
    duration: parseInt(duration.toString()),
    pos: parseInt(pos.toString()),
    buf_size: parseInt(size)
  };
  if (Object.keys(sd).length > 0) {
    pkt.side_data = {};
    Object.keys(sd)
      .filter(x => x.startsWith('side_data_'))
      .reduce((l, r) => {
        l[r.slice(10)] = sd[r];
        return l;
      }, pkt.side_data);
  } else {
    pkt.side_data = null;
  }
  return pkt;
}; */

const packetFromRedis = ({ pts, dts, stream_index, flags_KEY, flags_CORRUPT,
  flags_DISCARD, flags_TRUSTED, flags_DISPOSABLE, duration, pos, size, ...sd }) => {
  let pkt = {
    stream_index: parseInt(stream_index.toString()),
    buf_size: parseInt(size.toString())
  };
  if (pts) pkt.pts = parseInt(pts.toString());
  if (dts) pkt.dts = parseInt(dts.toString());
  if (flags_KEY || flags_CORRUPT || flags_DISCARD || flags_TRUSTED || flags_DISPOSABLE) {
    pkt.flags = {
      KEY: flags_KEY && flags_KEY[0] === 116,
      CORRUPT: flags_CORRUPT && flags_CORRUPT === 116,
      DISCARD: flags_DISCARD && flags_DISCARD === 116,
      TRUSTED: flags_TRUSTED && flags_TRUSTED === 116,
      DISPOSABLE: flags_DISPOSABLE && flags_DISPOSABLE === 116
    }; }
  if (duration) pkt.duration = parseInt(duration.toString());
  if (pos) pkt.pos = parseInt(pos.toString());
  if (Object.keys(sd).length > 0) {
    pkt.side_data = Object.keys(sd)
      .filter(x => x.startsWith('side_data_'))
      .reduce((l, r) => {
        l[r.slice(10)] = sd[r];
        return l;
      }, {});
  }
  return pkt;
};

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
