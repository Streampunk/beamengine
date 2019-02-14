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

const codecParToRedis = ({ type, sample_aspect_ratio, ...cp }) => { // eslint-disable-line no-unused-vars
  if (sample_aspect_ratio) {
    cp.sample_aspect_ratio_num = sample_aspect_ratio[0];
    cp.sample_aspect_ratio_den = sample_aspect_ratio[1];
  }
  return cp;
};

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
  initial_padding, trailing_padding, seek_preroll }) => {
  let cp = { codec_id: parseInt(codec_id.toString()) };
  if (extradata && extradata.length > 0) cp.extradata = extradata;
  if (format) cp.format = format.toString();
  if (bit_rate) cp.bit_rate = parseInt(bit_rate.toString());
  if (bits_per_coded_sample) cp.bits_per_coded_sample = parseInt(bits_per_coded_sample.toString());
  if (bits_per_raw_sample) cp.bits_per_raw_sample = parseInt(bits_per_raw_sample.toString());
  if (profile) cp.profile = stringOrNum(profile, -99);
  if (level) cp.level = stringOrNum(level, -99);
  if (width) cp.width = parseInt(width.toString());
  if (height) cp.height = parseInt(height.toString());
  if (sample_aspect_ratio_num && sample_aspect_ratio_den)
    cp.sample_aspect_ratio = [ parseInt(sample_aspect_ratio_num.toString()),
      parseInt(sample_aspect_ratio_den.toString()) ];
  if (field_order) cp.field_order = field_order.toString();
  if (color_range) cp.color_range = color_range.toString();
  if (color_primaries) cp.color_primaries = color_primaries.toString();
  if (color_trc) cp.color_trc = color_trc.toString();
  if (color_space) cp.color_space = color_space.toString();
  if (chroma_location) cp.chroma_location = chroma_location.toString();
  if (video_delay) cp.video_delay = parseInt(video_delay.toString());
  if (channel_layout) cp.channel_layout = channel_layout.toString();
  if (channels) cp.channels = parseInt(channels.toString());
  if (sample_rate) cp.sample_rate = parseInt(sample_rate.toString());
  if (block_align) cp.block_align = parseInt(block_align.toString());
  if (frame_size) cp.frame_size = parseInt(frame_size.toString());
  if (initial_padding) cp.initial_padding = parseInt(initial_padding.toString());
  if (trailing_padding) cp.trailing_padding = parseInt(trailing_padding.toString());
  if (seek_preroll) cp.seek_preroll = parseInt(seek_preroll.toString());
  return cp;
};

module.exports = {
  codecParToRedis,
  codecParFromRedis,
  stringOrNum
};
