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

const packetToRedis = ({ pts, dts, size, stream_index, flags,
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
};

const packetFromRedis = ({ pts, dts, stream_index, flags_KEY,
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
};

test('Roundtrip a packet', async t => {
  let data = Buffer.alloc(65536);
  for ( let x = 0 ; x < data.length ; x++ ) data[x] = x % 128;
  let pkt = beamcoder.packet({
    pts: 9876543210,
    dts: 43,
    data: data,
    flags: { KEY: true },
    side_data: { replaygain: Buffer.from('wibble') }
  });
  console.log(packetToRedis(pkt));
  let redis = new Redis();
  await redis.del(`beam:packet:${pkt.pts}`);
  let rdp = null;
  let start = process.hrtime();
  await redis.hmset(`beam:packet:${pkt.pts}`, packetToRedis(pkt));
  console.log('Set took', process.hrtime(start));
  start = process.hrtime();
  rdp = await redis.hgetallBuffer(`beam:packet:${pkt.pts}`);
  console.log('Get took', process.hrtime(start));
  console.log(packetFromRedis(rdp));
  // t.equal(Buffer.compare(rdp.data, data), 0, 'data has roundtripped OK.');
  let rp = beamcoder.packet(packetFromRedis(rdp));
  t.ok(rp, 'roundtrip packet is truthy.');
  console.log(rp);
  await redis.quit();
  t.end();
});
