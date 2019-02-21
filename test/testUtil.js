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

const beamcoder = require('beamcoder');

const stream0 = { type: 'Stream',
  index: 0,
  id: 301,
  time_base: [ 1, 90000 ],
  start_time: 7200,
  duration: 53666250,
  r_frame_rate: [ 24, 1 ],
  codecpar:
   { type: 'CodecParameters',
     codec_type: 'video',
     codec_id: 173,
     name: 'hevc',
     extradata: Buffer.from([0x00, 0x00, 0x01, 0x40, 0x01, 0x0c, 0x01, 0xff, 0xff]),
     format: 'yuv420p',
     profile: 0,
     level: 0,
     width: 1920,
     height: 1080,
     color_range: 'tv',
     video_delay: 1 } };

const stream1 = { type: 'Stream',
  index: 1,
  id: 302,
  time_base: [ 1, 90000 ],
  start_time: 7200,
  duration: 53641985,
  codecpar:
   { type: 'CodecParameters',
     codec_type: 'audio',
     codec_id: 86018,
     name: 'aac',
     format: 'fltp',
     bit_rate: 66494,
     profile: 'LC',
     channel_layout: 'stereo',
     channels: 2,
     sample_rate: 44100,
     frame_size: 1024 } };

const fmt = {
  iformat: 'mpegts',
  priv_data:
   { resync_size: 65536,
     fix_teletext_pts: true,
     scan_all_pmts: true,
     skip_unknown_pmt: false,
     merge_pmt_versions: false,
     skip_changes: false,
     skip_clear: false },
  url: 'test_url',
  start_time: 80000,
  duration: 596291667,
  bit_rate: 2176799,
  probe_score: 50,
  protocol_whitelist: 'file,crypto',
  interleaved: true };

module.exports = {
  stream0,
  stream1,
  get fmt() {
    let bfmt = beamcoder.format(fmt);
    bfmt.newStream(stream0);
    bfmt.newStream(stream1);
    return bfmt;
  }
};
