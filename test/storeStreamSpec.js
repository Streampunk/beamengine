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
const streamToRedis = ({ type, time_base, disposition, sample_aspect_ratio, metadata, // eslint-disable-line no-unused-vars
  avg_frame_rate, side_data, event_flags, r_frame_rate, codecpar, ...str }) => {
  str.time_base_num = time_base[0];
  str.time_base_den = time_base[1];
  if (disposition) {
    if (disposition.DEFAULT) str.disposition_DEFAULT = disposition.DEFAULT;
    if (disposition.DUB) str.disposition_DUB = disposition.DUB;
    if (disposition.ORIGINAL) str.disposition_ORIGINAL = disposition.ORIGINAL;
    if (disposition.COMMENT) str.disposition_COMMENT = disposition.COMMENT;
    if (disposition.LYRICS) str.disposition_LYRICS = disposition.LYRICS;
    if (disposition.KARAOKE) str.disposition_KARAOKE = disposition.KARAOKE;
    if (disposition.FORCED) str.disposition_FORCED = disposition.FORCED;
    if (disposition.HEARING_IMPAIRED) str.disposition_HEARING_IMPAIRED = disposition.HEARING_IMPAIRED;
    if (disposition.VISUAL_IMPAIRED) str.disposition_VISUAL_IMPAIRED = disposition.VISUAL_IMPAIRED;
    if (disposition.CLEAN_EFFECTS) str.disposition_CLEAN_EFFECTS = disposition.CLEAN_EFFECTS;
    if (disposition.ATTACHED_PIC) str.disposition_ATTACHED_PIC = disposition.ATTACHED_PIC;
    if (disposition.TIMED_THUMBNAILS) str.disposition_TIMED_THUMBNAILS = disposition.TIMED_THUMBNAILS;
    if (disposition.CAPTIONS) str.disposition_CAPTIONS = disposition.CAPTIONS;
    if (disposition.DESCRIPTIONS) str.disposition_DESCRIPTIONS = disposition.DESCRIPTIONS;
    if (disposition.METADATA) str.disposition_METADATA = disposition.METADATA;
    if (disposition.DEPENDENT) str.disposition_DEPENDENT = disposition.DEPENDENT;
    if (disposition.STILL_IMAGE) str.disposition_STILL_IMAGE = disposition.STILL_IMAGE;
  }
  if (sample_aspect_ratio) {
    str.sample_aspect_ratio_num = sample_aspect_ratio[0];
    str.sample_aspect_ratio_den = sample_aspect_ratio[1];
  }
  if (avg_frame_rate) {
    str.avg_frame_rate_num = avg_frame_rate[0];
    str.avg_frame_rate_den = avg_frame_rate[1];
  }
  if (event_flags) {
    if (event_flags.METADATA_UPDATED)
      str.event_flags_METADATA_UPDATED = event_flags.METADATA_UPDATED;
  }
  if (r_frame_rate) {
    str.r_frame_rate_num = r_frame_rate[0];
    str.r_frame_rate_den = r_frame_rate[1];
  }
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
      parseInt(time_base_den.toString()) ]
  };
  if (start_time) str.start_time = parseInt(start_time.toString());
  if (duration) str.duration = parseInt(duration.toString());
  if (nb_frames) str.nb_frames = parseInt(nb_frames.toString());
  if (disposition_DEFAULT || disposition_DUB || disposition_ORIGINAL ||
    disposition_COMMENT || disposition_LYRICS || disposition_KARAOKE ||
    disposition_FORCED || disposition_VISUAL_IMPAIRED || disposition_HEARING_IMPAIRED ||
    disposition_CLEAN_EFFECTS || disposition_ATTACHED_PIC || disposition_TIMED_THUMBNAILS ||
    disposition_CAPTIONS || disposition_DESCRIPTIONS || disposition_METADATA ||
    disposition_DEPENDENT || disposition_STILL_IMAGE) {
    str.disposition = {
      DEFAULT: disposition_DEFAULT && disposition_DEFAULT[0] === 116,
      DUB: disposition_DUB && disposition_DUB[0] === 116,
      ORIGINAL: disposition_ORIGINAL && disposition_ORIGINAL[0] === 116,
      COMMENT: disposition_COMMENT && disposition_COMMENT[0] === 116,
      LYRICS: disposition_LYRICS && disposition_LYRICS[0] === 116,
      KARAOKE: disposition_KARAOKE && disposition_KARAOKE[0] === 116,
      FORCED: disposition_FORCED && disposition_FORCED[0] === 116,
      HEARING_IMPAIRED: disposition_HEARING_IMPAIRED && disposition_HEARING_IMPAIRED[0] === 116,
      VISUAL_IMPAIRED: disposition_VISUAL_IMPAIRED && disposition_VISUAL_IMPAIRED[0] === 116,
      CLEAN_EFFECTS: disposition_CLEAN_EFFECTS && disposition_CLEAN_EFFECTS[0] === 116,
      ATTACHED_PIC: disposition_ATTACHED_PIC && disposition_ATTACHED_PIC[0] === 116,
      TIMED_THUMBNAILS: disposition_TIMED_THUMBNAILS && disposition_TIMED_THUMBNAILS[0] === 116,
      CAPTIONS: disposition_CAPTIONS && disposition_CAPTIONS[0] === 116,
      DESCRIPTIONS: disposition_DESCRIPTIONS && disposition_DESCRIPTIONS[0] === 116,
      METADATA: disposition_METADATA && disposition_METADATA[0] === 116,
      DEPENDENT: disposition_DEPENDENT && disposition_DEPENDENT[0] === 116,
      STILL_IMAGE: disposition_STILL_IMAGE && disposition_STILL_IMAGE[0] === 116,
    }; }
  if (discard) str.discard = discard.toString();
  if (sample_aspect_ratio_num && sample_aspect_ratio_den)
    str.sample_aspect_ratio = [ parseInt(sample_aspect_ratio_num.toString()),
      parseInt(sample_aspect_ratio_den.toString() )];
  if (avg_frame_rate_num && avg_frame_rate_den)
    str.avg_frame_rate = [ parseInt(avg_frame_rate_num.toString()),
      parseInt(avg_frame_rate_den.toString()) ];
  if (event_flags_METADATA_UPDATED ) {
    str.event_flags = {
      METADATA_UPDATED: event_flags_METADATA_UPDATED && event_flags_METADATA_UPDATED[0] === 116
    }; }
  if (r_frame_rate_num && r_frame_rate_den)
    str.r_frame_rate = [ parseInt(r_frame_rate_num.toString()),
      parseInt(r_frame_rate_den.toString()) ];
  str.codecpar = codecParFromRedis(Object.keys(etc)
    .filter(x => x.startsWith('codecpar_'))
    .reduce((l, r) => {
      l[r.slice(9)] = etc[r];
      return l;
    }, {} ));
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

  for ( let x = 0 ; x < 100 ; x++ ) {
    let start = process.hrtime();
    let sr = streamToRedis(s.toJSON());
    let end = process.hrtime(start);
    console.log('streamToRedis', end);
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
    console.log('Creating stream', process.hrtime(start));
    t.ok(rs, 'roundtrip stream is truthy.');
  }

  await redis.quit();
  t.end();
});
