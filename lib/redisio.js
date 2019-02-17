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

const Redis = require('ioredis');
const mappings = require('./mappings.js');
const config = require('../config.json');
const uuid = require('uuid');
const beamcoder = require('beamcoder');

/*
   Basic idea ...

   format stored at 'beamcoder:<url>' - TODO how to deal with multiple formats
   list of formats at beamcoder:content
   streams for format stored at 'beamcoder:<url>:stream_<index>'
     (streams embed codec parameters and contain nb_streams counter)
   packets for stream stored at 'beamcoder:<url>:stream_<index>:packet_<pts>'
   frames for stream stored at 'beamcoder:<url>:stream_<index>:frame_<pts>'
   sorted index sorted set is stored at 'beamcoder:<url>:stream_<index>:index'

   If <pts> is not available, a counter will be used.
   If <url> is empty, a UUID will be generated and stored as a urn.
   'beamcoder' is taken from config file
   Index maps pts to keys of frames or packets
*/

const EMPTY_SLOT = Object.freeze(Object.create(null));

const redisPool = {
  pool: [],
  nextFree: null,
  queue: [],
  use: async function() {
    if (this.nextFree == null || this.nextFree == this.pool.length) {
      if (!this.grow( this.pool.length || 5 )) {
        let wait = new Promise((fulfil) => {
          this.queue.push(fulfil);
        });
        return wait;
      }
    }
    let obj = this.pool[this.nextFree];
    this.pool[this.nextFree++] = EMPTY_SLOT;
    return obj;
  },
  recycle: function (obj) {
    if (this.nextFree === null || this.nextFree === -1) {
      this.pool[this.pool.length] = obj;
    } else {
      this.pool[--this.nextFree] = obj;
    }
    if (this.queue.length > 0) {
      this.queue.shift()(this.use());
    }
  },
  grow: function (count = this.pool.length) {
    if (count > 0 && this.nextFree == null) {
      this.nextFree = 0;
    }

    if ((this.pool.length + count) > config.redis.pool) {
      return false;
    }

    if (count > 0) {
      let curLen = this.pool.length;
      this.pool.length += count;
      for ( let x = curLen ; x < this.pool.length ; x++ ) {
        this.pool[x] = new Redis();
      }
    }
    return true;
  },
  size: function () {
    return this.pool.length;
  },
  close: async function () {
    console.log(this.pool);
    let result = await Promise.all(
      this.pool.filter(x => x !== EMPTY_SLOT)
        .map(x => x.quit()));
    this.queue.forEach(x => x(null));
    this.queue = [];
    this.pool = [];
    this.nextFree = null;
    return result;
  }
};

['pool', 'nextFree', 'queue', 'grow'].forEach(p => {
  Object.defineProperty(redisPool, p, {
    enumerable: false
  });
});

async function listContent(start = 0, end = -1) {

}

async function storeFormat(fmt, overwrite = true) {
  let redis = await Promise.all(Array.from(
    new Array(fmt.streams.length + 1), () => redisPool.use()));
  if ((!fmt.url) || (fmt.url && fmt.url.length === 0)) {
    fmt.url = `urn:uuid:${uuid.v4()}`;
  }
  let key = `${config.redis.prepend}:${fmt.url}`;
  if (!overwrite) {
    let exists = await redis[0].exists(key);
    if (exists) throw new Error(`A format with key '${key}' already exists.`);
  }
  let delWork = [ redis[0].del(key) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    delWork.push(redis[x].del(`${key}:stream_${x-1}`));
  }
  await Promise.all(delWork);
  let setWork = [ redis[0].hmset(key, mappings.formatToRedis(fmt.toJSON())) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    setWork.push(redis[x].hmset(`${key}:stream_${x-1}`, mappings.streamToRedis(fmt.stream[x-1])));
  }
  let result = await Promise.all(setWork);

  redis.forEach(x => redisPool.recycle(x));
  return result;
}

// TODO wot no pts?

async function storePacket(fmt, packet) {
  let redis = await Promise.all([redisPool.use(), redisPool.use()]);
  let idxk = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${packet.stream_index}:index`;
  let key = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${packet.stream_index}:packet_${packet.pts}`;
  let datak = `${key}:data`;
  await Promise.all([redis[0].del(key), redis[1].del(datak)]);
  let result = await Promise.all([
    redis[0].hmset(key, mappings.packetToRedis(packet.toJSON())),
    redis[1].setBuffer(datak, packet.data) ]);
  await redis[0].zadd(idxk, packet.pts, key);
  redis.forEach(x => redisPool.recycle(x));
  return result;
}

async function storeFrame(fmt, frame, stream_index) {
  let redis = await Promise.all(
    Array.from(new Array(frame.data.length + 1), () => redisPool.use()));
  if (!stream_index) {
    stream_index = frame.stream_index;
    if (!stream_index) {
      throw new Error('Cannot determine stream_index for frame.');
    }
  }
  frame.stream_index = stream_index;
  let idxk = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${stream_index}:index`;
  let key = `${config.redis.prepend}:${typeof fmt === 'string' ? fmt : fmt.url}` +
    `:stream_${stream_index}:frame_${frame.pts}`;
  let datak_base = `${key}:data`;

  let delWork = [ redis[0].del(key) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    delWork.push(redis[x].del(`${datak_base}_${x-1}`));
  }
  await Promise.all(delWork);

  let setWork = [ redis[0].hmset(mappings.frameToRedis(frame.toJSON())) ];
  for ( let x = 1 ; x < redis.length ; x++ ) {
    setWork.push(redis[x].setBuffer(`${datak_base}_${x-1}`, frame.data[x-1]));
  }
  let result = await Promise.all(setWork);
  await redis[0].zadd(idxk, frame.pts, key);
  redis.forEach(x => redisPool.recycle(x));
  return result;
}

async function storeMedia(fmt, element, stream_index) {
  switch (element.type) {
  case 'Packet':
    return storePacket(fmt, element);
  case 'Frame':
    return storeFrame(fmt, element, stream_index);
  default:
    throw new Error('Cannot store media element unless of type Packet or Frame.');
  }
}

async function retrieveFormat(url) {
  let redisZero = await redisPool.use();
  let key = `${config.redis.prepend}:${url}`;
  let fmtb = await redisZero.hgetallBuffer(key);
  if (!fmtb) {
    throw new Error(`Unable to retrieve a format with key '${key}'.`);
  }
  let fmto = mappings.formatFromRedis(fmtb);
  let result = beamcoder.fmt(fmto);
  if (fmto.nb_streams > 0) {
    let redisProms = [ redisZero ];
    for ( let x = 1 ; x < fmto.nb_streams ; x++ ) {
      redisProms.push(redisPool.use());
    }
    let redis = await Promise.add(redisProms);
    let getWork = [];
    for ( let x = 0 ; x < redis.length ; x++ ) {
      getWork.push(redis.hgetallBuffer(`${key}:stream_${x}`));
    }
    let strs = await Promise.all(getWork);
    if (strs.some(x => x === null)) {
      throw new Error(`Cannot retrieve at least one of the streams '${strs.map(x => x ? '.' : 'X')}'.`);
    }
    for ( let x = 0 ; x < strs.length ; x++ ) {
      result.newStream(mappings.streamFromRedis(strs[x]));
    }
    redis.forEach(x => redisPool.recycle(x));
  } else {
    redisPool.recycle(redisZero);
  }
  return result;
}

async function retrieveStream(url, stream_index) {
  let redis = await redisPool.use();
  let key = `${config.redis.prepend}:${url}:stream_${stream_index}`;
  let strb = await redis.hgetallBuffer(key);
  if (!strb) {
    throw new Error(`Unable to retrieve a streams with key '${key}'.`);
  }
  redisPool.recycle(redis);
  let stro = mappings.streamFromRedis(strb);
  let fmt = beamcoder.format();
  let result = fmt.newStream(stro); // FIX_ME stream_index will be wrong
  return result;
}

async function retrieveMedia(url_or_fmt, stream_index, pts_start, pts_end = null) {

}

module.exports = {
  redisPool,
  storeFormat,
  storeMedia,
  retrieveFormat,
  retrieveStream,
  retrieveMedia
};
// Delete operations.
