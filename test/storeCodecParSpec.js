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

const codecParToRedis = ({ codec_type, codec_id, name, codec_tag, extradata,
  format, bit_rate, bits_per_coded_sample, bits_per_raw_sample, profile, level,
  width, height, sample_aspect_ratio, field_order, color_range, color_primaries,
  color_trc, color_space, chroma_location, video_delay, channel_layout, channels,
  sample_rate, block_align, frame_size, initial_padding, trailing_padding,
  seek_preroll }) => ({
  codec_type,
  codec_id,
  name,
  codec_tag,
  extradata,
  format,
  bit_rate,
  bits_per_coded_sample,
  bits_per_raw_sample,
  profile,
  level,
  width,
  height,
  sample_aspect_ratio_num: sample_aspect_ratio[0],
  sample_aspect_ratio_den: sample_aspect_ratio[1],
  field_order,
  color_range,
  color_primaries,
  color_trc,
  color_space,
  chroma_location,
  video_delay,
  channel_layout: channel_layout === '0 channels' ? '' : channel_layout,
  channels,
  sample_rate,
  block_align,
  frame_size,
  initial_padding,
  trailing_padding,
  seek_preroll
});

const stringOrNum = (b, def) => {
  if (def && b.length === 0) return def;
  let v = b.toString();
  let n = parseInt(v);
  return isNaN(n) ? v : n;
};

const codecParFromRedis = ({ codec_id, extradata,
  format, bit_rate, bits_per_coded_sample, bits_per_raw_sample, profile, level,
  width, height, sample_aspect_ratio_num, sample_aspect_ratio_den, field_order,
  color_range, color_primaries, color_trc, color_space, chroma_location,
  video_delay, channel_layout, channels, sample_rate, block_align, frame_size,
  initial_padding, trailing_padding, seek_preroll }) => ({
  codec_id: parseInt(codec_id.toString()),
  extradata: extradata.length > 0 ? extradata : null,
  format: format.length > 0 ? format.toString() : null,
  bit_rate: parseInt(bit_rate.toString()),
  bits_per_coded_sample: parseInt(bits_per_coded_sample.toString()),
  bits_per_raw_sample: parseInt(bits_per_raw_sample.toString()),
  profile: stringOrNum(profile, -99),
  level: stringOrNum(level, -99),
  width: parseInt(width.toString()),
  height: parseInt(height.toString()),
  sample_aspect_ratio: [ parseInt(sample_aspect_ratio_num.toString()),
    parseInt(sample_aspect_ratio_den.toString()) ],
  field_order: field_order.toString(),
  color_range: color_range.toString(),
  color_primaries: color_primaries.toString(),
  color_trc: color_trc.toString(),
  color_space: color_space.toString(),
  chroma_location: chroma_location.toString(),
  video_delay: parseInt(video_delay.toString()),
  channel_layout: channel_layout.length > 0 ? channel_layout.toString() : null,
  channels: parseInt(channels.toString()),
  sample_rate: parseInt(sample_rate.toString()),
  block_align: parseInt(block_align.toString()),
  frame_size: parseInt(frame_size.toString()),
  initial_padding: parseInt(initial_padding.toString()),
  trailing_padding: parseInt(trailing_padding.toString()),
  seek_preroll: parseInt(seek_preroll.toString())
});

test('Roundtrip codec parameters', async t => {
  let cps = beamcoder.codecParameters({
    name: 'h264',
    width: 1920,
    height: 1080,
    format: 'yuv422p'
  });
  console.log(codecParToRedis(cps));
  let redis = new Redis();
  await redis.del('beam:codecpar:test');
  let start = process.hrtime();
  let cpr = codecParToRedis(cps);
  console.log('codecParToRedis', process.hrtime(start));
  start = process.hrtime();
  await redis.hmset('beam:codecpar:test', cpr);
  console.log('redis set', process.hrtime(start));

  start = process.hrtime();
  let rcpb = await redis.hgetallBuffer('beam:codecpar:test');
  console.log('redis get', process.hrtime(start));

  start = process.hrtime();
  let rcpo = codecParFromRedis(rcpb);
  console.log('codecParFromRedis', process.hrtime(start));

  let rcp = beamcoder.codecParameters(rcpo);
  console.log(rcp);
  t.ok(rcp, 'roundtrip codec parameters is truthy.');
  await redis.quit();
  t.end();
});
