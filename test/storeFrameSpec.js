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

const frameToRedis = ({ linesize, width, height, nb_samples, format,
  key_frame, pict_type, sample_aspect_ratio, pts, pkt_dts, coded_picture_number,
  display_picture_number, quality, repeat_pict, interlaced_frame, top_field_first,
  palette_has_changed, reordered_opaque, sample_rate, channel_layout, side_data,
  flags, color_range, color_primaries, color_trc, colorspace, chroma_location,
  best_effort_timestamp, pkt_pos, pkt_duration, metadata, decode_error_flags,
  channels, pkt_size, crop_top, crop_bottom, crop_left, crop_right, data }) => {

  let frm = {
    linesize: JSON.stringify(linesize),
    width,
    height,
    nb_samples,
    format,
    key_frame,
    pict_type,
    sample_aspect_ratio_num: sample_aspect_ratio[0],
    sample_aspect_ratio_den: sample_aspect_ratio[1],
    pts,
    pkt_dts,
    coded_picture_number,
    display_picture_number,
    quality,
    repeat_pict,
    interlaced_frame,
    top_field_first,
    palette_has_changed,
    reordered_opaque,
    sample_rate,
    channel_layout,
    flags_CORRUPT: flags.CORRUPT,
    flags_DISCARD: flags.DISCARD,
    color_range,
    color_primaries,
    color_trc,
    colorspace,
    chroma_location,
    best_effort_timestamp,
    pkt_pos,
    pkt_duration,
    decode_error_flags_INVALID_BITSTREAM: decode_error_flags.INVALID_BITSTREAM,
    decode_error_flags_MISSING_REFERENCE: decode_error_flags.MISSING_REFERENCE,
    channels,
    pkt_size,
    crop_top,
    crop_bottom,
    crop_left,
    crop_right,
    buf_sizes: JSON.stringify(data.map(x => x.length))
  };
  if (side_data) {
    Object.keys(side_data).reduce((l, r) => {
      if (r != 'type') l[`side_data_${r}`] = side_data[r];
      return l;
    }, frm);
  }
  if (metadata) {
    Object.keys(metadata).reduce((l, r) => {
      l[`metadata_${r}`] = metadata[r];
    }, frm);
  }
  return frm;
};

const frameFromRedis = ({ linesize, width, height, nb_samples, format, key_frame,
  pict_type, sample_aspect_ratio_num, sample_aspect_ratio_den, pts, pkt_dts,
  coded_picture_number, display_picture_number, quality, repeat_pict, interlaced_frame,
  top_field_first, palette_has_changed, reordered_opaque, sample_rate, channel_layout,
  flags_CORRUPT, flags_DISCARD, color_range, color_primaries, color_trc, colorspace,
  chroma_location, best_effort_timestamp, pkt_pos, pkt_duration,
  decode_error_flags_INVALID_BITSTREAM, decode_error_flags_MISSING_REFERENCE,
  channels, pkt_size, crop_top, crop_bottom, crop_left, crop_right, buf_sizes, ...sdmd }) => {
  let frm = {
    linesize: JSON.parse(linesize),
    width: parseInt(width.toString()),
    height: parseInt(height.toString()),
    nb_samples: parseInt(nb_samples.toString()),
    format: format.length > 0 ? format.toString() : null,
    key_frame: key_frame[0] === 116, // t for true
    pict_type: pict_type.length !== 0 ? pict_type : null,
    sample_aspect_ratio: [ parseInt(sample_aspect_ratio_num.toString()),
      parseInt(sample_aspect_ratio_den.toString()) ],
    pts: pts.length > 0 ? parseInt(pts.toString()) : null,
    pkt_dts: pkt_dts.length > 0 ? parseInt(pkt_dts.toString()) : null,
    coded_picture_number: parseInt(coded_picture_number.toString()),
    display_picture_number: parseInt(display_picture_number.toString()),
    quality: parseInt(quality.toString()),
    repeat_pict: parseInt(repeat_pict.toString()),
    interlaced_frame: interlaced_frame[0] === 116,
    top_field_first: top_field_first[0] === 116,
    palette_has_changed: palette_has_changed[0] === 116,
    reordered_opaque: reordered_opaque.length > 0 ? parseInt(reordered_opaque.toString()) : null,
    sample_rate: parseInt(sample_rate.toString()),
    channel_layout: channel_layout.toString(),
    flags: {
      CORRUPT: flags_CORRUPT[0] === 116,
      DISCARD: flags_DISCARD[0] === 116
    },
    color_range: color_range.toString(),
    color_primaries: color_primaries.toString(),
    color_trc: color_trc.toString(),
    colorspace: colorspace.toString(),
    chroma_location: chroma_location.toString(),
    best_effort_timestamp: best_effort_timestamp.length > 0 ? parseInt(best_effort_timestamp.toString()) : null,
    pkt_pos: parseInt(pkt_pos.toString()),
    pkt_duration: parseInt(pkt_duration.toString()),
    decode_error_flags : {
      INVALID_BITSTREAM: decode_error_flags_INVALID_BITSTREAM[0] === 116,
      MISSING_REFERENCE: decode_error_flags_MISSING_REFERENCE[0] === 116
    },
    channels: parseInt(channels.toString()),
    pkt_size: parseInt(pkt_size.toString()),
    crop_top: parseInt(crop_top.toString()),
    crop_bottom: parseInt(crop_bottom.toString()),
    crop_left: parseInt(crop_left.toString()),
    crop_right: parseInt(crop_right.toString()),
    buf_sizes: JSON.parse(buf_sizes)
  };
  if (Object.keys(sdmd).length > 0) {
    if (Object.keys(sdmd).find(x => x.startsWith('metadata_'))) {
      frm.metadata = {};
      Object.keys(sdmd)
        .filter(x => x.startsWith('metadata_'))
        .reduce((l, r) => {
          l[r.slice(9)] = sdmd[r].toString() ;
          return l;
        }, frm.metadata);
    }
    if (Object.keys(sdmd).find(x => x.startsWith('side_data_'))) {
      frm.side_data = {};
      Object.keys(sdmd)
        .filter(x => x.startsWith('side_data_'))
        .reduce((l, r) => {
          l[r.slice(10)] = sdmd[r];
          return l;
        }, frm.side_data);
    }
  }
  return frm;
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
  let start = process.hrtime();
  let frmr = frameToRedis(frm);
  console.log('frameToRedis', process.hrtime(start));
  start = process.hrtime();
  await redis.hmset(`beam:frame:${frm.pts}`, frmr);
  console.log('redis set', process.hrtime(start));

  start = process.hrtime();
  let rfrmb = await redis.hgetallBuffer(`beam:frame:${frm.pts}`);
  console.log('redis get', process.hrtime(start));

  start = process.hrtime();
  let rfrm = frameFromRedis(rfrmb);
  console.log('frameFromRedis', process.hrtime(start));

  console.log(rfrm);
  let rf = beamcoder.frame(rfrm);
  console.log(rf);
  t.ok(rf, 'roundtrip frame is truthy.');
  await redis.quit();
  t.end();
});

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
