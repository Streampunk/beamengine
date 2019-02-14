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

const frameToRedis = ({ type, linesize, sample_aspect_ratio,
  flags, decode_error_flags, side_data, metadata, buf_sizes, ...f }) => {
  f.linesize = JSON.stringify(linesize);
  if (sample_aspect_ratio) {
    f.sample_aspect_ratio_num = sample_aspect_ratio[0];
    f.sample_aspect_ratio_den = sample_aspect_ratio[1];
  }
  if (flags) {
    f.flags_CORRUPT = flags.CORRUPT;
    f.flags_DISCARD = flags.DISCARD;
  }
  if (decode_error_flags) {
    f.decode_error_flags_INVALID_BITSTREAM = decode_error_flags.INVALID_BITSTREAM;
    f.decode_error_flags_MISSING_REFERENCE = decode_error_flags.MISSING_REFERENCE;
  }
  if (buf_sizes) {
    f.buf_sizes =  JSON.stringify(buf_sizes);
  }
  if (side_data) {
    Object.keys(side_data).reduce((l, r) => {
      if (r != 'type') l[`side_data_${r}`] = side_data[r];
      return l;
    }, f);
  }
  if (metadata) {
    Object.keys(metadata).reduce((l, r) => {
      l[`metadata_${r}`] = metadata[r];
    }, f);
  }
  return f;
};

const frameFromRedis = ({ linesize, width, height, nb_samples, format, key_frame,
  pict_type, sample_aspect_ratio_num, sample_aspect_ratio_den, pts, pkt_dts,
  coded_picture_number, display_picture_number, quality, repeat_pict, interlaced_frame,
  top_field_first, palette_has_changed, reordered_opaque, sample_rate, channel_layout,
  flags_CORRUPT, flags_DISCARD, color_range, color_primaries, color_trc, colorspace,
  chroma_location, best_effort_timestamp, pkt_pos, pkt_duration,
  decode_error_flags_INVALID_BITSTREAM, decode_error_flags_MISSING_REFERENCE,
  channels, pkt_size, crop_top, crop_bottom, crop_left, crop_right, buf_sizes, ...sdmd }) => {
  let f = { linesize: JSON.parse(linesize) };
  if (width) f.width = parseInt(width.toString());
  if (height) f.height = parseInt(height.toString());
  if (nb_samples) f.nb_samples = parseInt(nb_samples.toString());
  if (format && format.length > 0) f.format = format.toString();
  if (key_frame) f.key_frame = key_frame[0] === 116; // t for true
  if (pict_type) f.pict_type = pict_type.toString();
  if (sample_aspect_ratio_num && sample_aspect_ratio_den)
    f.sample_aspect_ratio = [ parseInt(sample_aspect_ratio_num.toString()),
      parseInt(sample_aspect_ratio_den.toString()) ];
  if (pts) f.pts = parseInt(pts.toString());
  if (pkt_dts) f.pkt_dts = parseInt(pkt_dts.toString());
  if (coded_picture_number) f.coded_picture_number = parseInt(coded_picture_number.toString());
  if (display_picture_number) f.display_picture_number = parseInt(display_picture_number.toString());
  if (quality) f.quality = parseInt(quality.toString());
  if (repeat_pict) f.repeat_pict = parseInt(repeat_pict.toString());
  if (interlaced_frame) f.interlaced_frame = interlaced_frame[0] === 116;
  if (top_field_first) f.top_field_first= top_field_first[0] === 116;
  if (palette_has_changed) f.palette_has_changed = palette_has_changed[0] === 116;
  if (reordered_opaque) f.reordered_opaque = parseInt(reordered_opaque.toString());
  if (sample_rate) f.sample_rate = parseInt(sample_rate.toString());
  if (channel_layout) f.channel_layout = channel_layout.toString();
  if (flags_CORRUPT || flags_DISCARD) {
    f.flags = {
      CORRUPT: flags_CORRUPT && flags_CORRUPT[0] === 116,
      DISCARD: flags_DISCARD && flags_DISCARD[0] === 116
    }; }
  if (color_range) f.color_range = color_range.toString();
  if (color_primaries) f.color_primaries= color_primaries.toString();
  if (color_trc) f.color_trc = color_trc.toString();
  if (colorspace) f.colorspace = colorspace.toString();
  if (chroma_location) f.chroma_location = chroma_location.toString();
  if (best_effort_timestamp) f.best_effort_timestamp = parseInt(best_effort_timestamp.toString());
  if (pkt_pos) f.pkt_pos = parseInt(pkt_pos.toString());
  if (pkt_duration) f.pkt_duration = parseInt(pkt_duration.toString());
  if (decode_error_flags_INVALID_BITSTREAM || decode_error_flags_MISSING_REFERENCE) {
    f.decode_error_flags = {
      INVALID_BITSTREAM: decode_error_flags_INVALID_BITSTREAM && decode_error_flags_INVALID_BITSTREAM[0] === 116,
      MISSING_REFERENCE: decode_error_flags_MISSING_REFERENCE && decode_error_flags_MISSING_REFERENCE[0] === 116
    }; }
  if (channels) f.channels = parseInt(channels.toString());
  if (pkt_size) f.pkt_size = parseInt(pkt_size.toString());
  if (crop_top) f.crop_top = parseInt(crop_top.toString());
  if (crop_bottom) f.crop_bottom = parseInt(crop_bottom.toString());
  if (crop_left) f.crop_left = parseInt(crop_left.toString());
  if (crop_right) f.crop_right = parseInt(crop_right.toString());
  if (buf_sizes) f.buf_sizes = JSON.parse(buf_sizes);
  if (Object.keys(sdmd).length > 0) {
    if (Object.keys(sdmd).find(x => x.startsWith('metadata_'))) {
      f.metadata = Object.keys(sdmd)
        .filter(x => x.startsWith('metadata_'))
        .reduce((l, r) => {
          l[r.slice(9)] = sdmd[r].toString() ;
          return l;
        }, {});
    }
    if (Object.keys(sdmd).find(x => x.startsWith('side_data_'))) {
      f.side_data = Object.keys(sdmd)
        .filter(x => x.startsWith('side_data_'))
        .reduce((l, r) => {
          l[r.slice(10)] = sdmd[r];
          return l;
        }, {});
    }
  }
  return f;
};

test('Roundtrip a frame', async t => {
  let frm = beamcoder.frame({
    pts: 1234567890,
    width: 1920,
    height: 1080,
    format: 'yuv422p',
    metadata: { wibble: 'wobble' },
    side_data: { replaygain : Buffer.from('wombles') }
  }).alloc();
  let redis = new Redis();
  await redis.del(`beam:frame:${frm.pts}`);
  for ( let x = 0 ; x < 100 ; x++ ) {
    let start = process.hrtime();
    let frmr = frameToRedis(frm.toJSON());
    console.log('frameToRedis', process.hrtime(start));

    start = process.hrtime();

    t.equal(await redis.hmset(`beam:frame:${frm.pts}`, frmr), 'OK',
      'redis reports set OK.');
    console.log('redis set', process.hrtime(start));

    start = process.hrtime();
    let rfrmb = await redis.hgetallBuffer(`beam:frame:${frm.pts}`);
    console.log('redis get', process.hrtime(start));
    start = process.hrtime();
    let rfrm = frameFromRedis(rfrmb);
    console.log('frameFromRedis', process.hrtime(start));
    console.log(rfrm);

  }
  /*
  console.log(rfrm);
  let rf = beamcoder.frame(rfrm);
  console.log(rf);
  t.ok(rf, 'roundtrip frame is truthy.'); */
  await redis.quit();
  t.end();
});

/* test('Frame JSON performance', t => {
  let f = beamcoder.frame();
  for ( let x = 0 ; x < 100 ; x++ ) {
    let start = process.hrtime();
    let j = f.toJSON();
    console.log(process.hrtime(start));
    console.log(j);
  }
  t.end();
}); */

/* test('Buffer roundtrip performance', t => {
  let start = process.hrtime();
  let b = Buffer.from('9876543210');
  for ( let x = 0 ; x < 10000000 ; x++ ) {
    let n = JSON.parse(b);
    if (n != 9876543210) console.error('Blah!');
  }
  console.log('JSON.parse', process.hrtime(start));

  start = process.hrtime();
  for ( let x = 0 ; x < 10000000 ; x++ ) {
    let n = +b.toString();
    if (n != 9876543210) console.error('Blah!');
  }
  console.log('plus toString', process.hrtime(start));

  start = process.hrtime();
  for ( let x = 0 ; x < 10000000 ; x++ ) {
    let n = parseInt(b);
    if (n != 9876543210) console.error('Blah!');
  }
  console.log('parseInt', process.hrtime(start));

  t.end();
}); */
