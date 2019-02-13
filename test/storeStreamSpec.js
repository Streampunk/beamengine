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
const { codecParToRedis, codecParFromRedis } = require('../lib/mappings.js');

// TODO attached pic
const streamToRedis = ({ index, id, time_base, start_time, duration, nb_frames,
  disposition, discard, sample_aspect_ratio, metadata, avg_frame_rate,
  side_data, event_flags, r_frame_rate, codecpar }) => {
  let str = {
    index,
    id,
    time_base_num: time_base[0],
    time_base_den: time_base[1],
    start_time,
    duration,
    nb_frames,
    disposition_DEFAULT: disposition.DEFAULT,
    disposition_DUB: disposition.DUB,
    disposition_ORIGINAL: disposition.ORIGINAL,
    disposition_COMMENT: disposition.COMMENT,
    disposition_LYRICS: disposition.LYRICS,
    disposition_KARAOKE: disposition.KARAOKE,
    disposition_FORCED: disposition.FORCED,
    disposition_HEARING_IMPAIRED: disposition.HEARING_IMPAIRED,
    disposition_VISUAL_IMPAIRED: disposition.VISUAL_IMPAIRED,
    disposition_CLEAN_EFFECTS: disposition.CLEAN_EFFECTS,
    disposition_ATTACHED_PIC: disposition.ATTACHED_PIC,
    disposition_TIMED_THUMBNAILS: disposition.TIMED_THUMBNAILS,
    disposition_CAPTIONS: disposition.CAPTIONS,
    disposition_DESCRIPTIONS: disposition.DESCRIPTIONS,
    disposition_METADATA: disposition.METADATA,
    disposition_DEPENDENT: disposition.DEPENDENT,
    disposition_STILL_IMAGE: disposition.STILL_IMAGE,
    discard,
    sample_aspect_ratio_num: sample_aspect_ratio[0],
    sample_aspect_ratio_den: sample_aspect_ratio[1],
    avg_frame_rate_num: avg_frame_rate[0],
    avg_frame_rate_den: avg_frame_rate[1],
    event_flags_METADATA_UPDATED: event_flags.METADATA_UPDATED,
    r_frame_rate_num: r_frame_rate[0],
    r_frame_rate_den: r_frame_rate[1]
  };
  let embeddedPars = codecParToRedis(codecpar);
  Object.keys(embeddedPars).reduce((l, r) => {
    l[`codecpar_${r}`] = embeddedPars[r];
    return l;
  }, str);
  if (side_data) {
    Object.keys(side_data).reduce((l, r) => {
      if (r != 'type') l[`side_data_${r}`] = side_data[r];
      return l;
    }, str);
  }
  if (metadata) {
    Object.keys(metadata).reduce((l, r) => {
      l[`metadata_${r}`] = metadata[r];
    }, str);
  }
  return str;
};

const streamFromRedis = ({ index, id, time_base_num, time_base_den, start_time,
  duration, nb_frames, disposition_DEFAULT, disposition_DUB, disposition_ORIGINAL,
  disposition_COMMENT, disposition_LYRICS, disposition_KARAOKE, disposition_FORCED,
  disposition_HEARING_IMPAIRED, disposition_VISUAL_IMPAIRED,
  disposition_CLEAN_EFFECTS, disposition_ATTACHED_PIC, disposition_TIMED_THUMBNAILS,
  disposition_CAPTIONS, disposition_DESCRIPTIONS, disposition_METADATA,
  disposition_DEPENDENT, disposition_STILL_IMAGE, discard,
  sample_aspect_ratio_num, sample_aspect_ratio_den, avg_frame_rate_num,
  avg_frame_rate_den, event_flags_METADATA_UPDATED, r_frame_rate_num,
  r_frame_rate_den, ...etc }) => {
  let str = {
    index: parseInt(index.toString()),
    id: parseInt(id.toString()),
    time_base: [ parseInt(time_base_num.toString()),
      parseInt(time_base_den.toString()) ],
    start_time: start_time.length > 0 ? parseInt(start_time.toString()) : null,
    duration: duration.length > 0 ? parseInt(duration.toString()) : null,
    nb_frames: parseInt(nb_frames.toString()),
    disposition: {
      DEFAULT: disposition_DEFAULT[0] === 116,
      DUB: disposition_DUB[0] === 116,
      ORIGINAL: disposition_ORIGINAL[0] === 116,
      COMMENT: disposition_COMMENT[0] === 116,
      LYRICS: disposition_LYRICS[0] === 116,
      KARAOKE: disposition_KARAOKE[0] === 116,
      FORCED: disposition_FORCED[0] === 116,
      HEARING_IMPAIRED: disposition_HEARING_IMPAIRED[0] === 116,
      VISUAL_IMPAIRED: disposition_VISUAL_IMPAIRED[0] === 116,
      CLEAN_EFFECTS: disposition_CLEAN_EFFECTS[0] === 116,
      ATTACHED_PIC: disposition_ATTACHED_PIC[0] === 116,
      TIMED_THUMBNAILS: disposition_TIMED_THUMBNAILS[0] === 116,
      CAPTIONS: disposition_CAPTIONS[0] === 116,
      DESCRIPTIONS: disposition_DESCRIPTIONS[0] === 116,
      METADATA: disposition_METADATA[0] === 116,
      DEPENDENT: disposition_DEPENDENT[0] === 116,
      STILL_IMAGE: disposition_STILL_IMAGE[0] === 116,
    },
    discard: discard.length > 0 ? discard.toString() : null,
    sample_aspect_ratio: [ parseInt(sample_aspect_ratio_num.toString()),
      parseInt(sample_aspect_ratio_den.toString() )],
    avg_frame_rate: [ parseInt(avg_frame_rate_num.toString()),
      parseInt(avg_frame_rate_den.toString()) ],
    event_flags: {
      METADATA_UPDATED: event_flags_METADATA_UPDATED[0] === 116
    },
    r_frame_rate : [ parseInt(r_frame_rate_num.toString()),
      parseInt(r_frame_rate_den.toString()) ],
    codecpar: codecParFromRedis(Object.keys(etc)
      .filter(x => x.startsWith('codecpar_'))
      .reduce((l, r) => {
        l[r.slice(9)] = etc[r];
        return l;
      }, {} ))
  };
  if (Object.keys(etc).length > 0) {
    if (Object.keys(etc).find(x => x.startsWith('metadata_'))) {
      str.metadata = Object.keys(etc)
        .filter(x => x.startsWith('metadata_'))
        .reduce((l, r) => {
          l[r.slice(9)] = etc[r].toString() ;
          return l;
        }, {});
    }
    if (Object.keys(etc).find(x => x.startsWith('side_data_'))) {
      str.side_data = Object.keys(etc)
        .filter(x => x.startsWith('side_data_'))
        .reduce((l, r) => {
          l[r.slice(10)] = etc[r];
          return l;
        }, {});
    }
  }
  return str;
};

test('Roundtrip a stream', async t => {
  let fmt = beamcoder.format();
  let s = fmt.newStream({
    name:'h264',
    side_data: { replaygain: Buffer.from('Magic Roundabout') },
    metadata: { jelly: 'plate' }
  });
  const redis = new Redis();
  await redis.del('beam:stream:test');

  let start = process.hrtime();
  let sr = streamToRedis(s);
  console.log('streamToRedis', process.hrtime(start));
  start = process.hrtime();
  t.equal(await redis.hmset('beam:stream:test', sr), 'OK', 'reports set OK.');
  console.log('redis set', process.hrtime(start));

  start = process.hrtime();
  let srb = await redis.hgetallBuffer('beam:stream:test');
  console.log('redis get', process.hrtime(start));
  start = process.hrtime();
  let rso = streamFromRedis(srb);
  console.log('streamFromRedis', process.hrtime(start));

  start = process.hrtime();
  let rs = fmt.newStream(rso);
  console.log('Creatint stream', process.hrtime(start));
  // console.log(rs);
  t.ok(rs, 'roundtrip stream is truthy.');

  await redis.quit();
  t.end();
});
