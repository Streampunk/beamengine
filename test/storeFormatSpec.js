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

const formatToRedis = ({ type, priv_data, ctx_flags, streams, flags, programs,  // eslint-disable-line no-unused-vars
  metadata, debug, event_flags, avio_flags, __streams, ...fmt }) => {  // eslint-disable-line no-unused-vars
  if (priv_data) {
    fmt.priv_data = Object.keys(priv_data).reduce((l, r) => {
      l[`priv_data_${r}`] = priv_data[r];
      return l;
    }, {});
  }
  if (ctx_flags) {
    if (ctx_flags.NOHEADER) fmt.ctx_flags_HOHEADER = ctx_flags.NOHEADER;
    if (ctx_flags.UNSEEKABLE) fmt.ctx_flags_UNSEEKABLE = ctx_flags.UNSEEKABLE;
  }
  fmt.nb_streams = streams.length;
  if (flags) {
    if (flags.GENPTS) fmt.ctx_flags_GENPTS = ctx_flags.GENPTS;
    if (flags.IGNIDX) fmt.ctx_flags_IGNIDX = ctx_flags.IGNIDX;
    if (flags.NONBLOCK) fmt.ctx_flags_NONBLOCK = ctx_flags.NONBLOCK;
    if (flags.IGNDTS) fmt.ctx_flags_IGNDTS = ctx_flags.IGNDTS;
    if (flags.NOFILLIN) fmt.ctx_flags_NOFILLIN = ctx_flags.NOFILLIN;
    if (flags.NOPARSE) fmt.ctx_flags_NOPARSE = ctx_flags.NOPARSE;
    if (flags.NOBUFFER) fmt.ctx_flags_NOBUFFER = ctx_flags.NOBUFFER;
    if (flags.CUSTOM_IO) fmt.ctx_flags_CUSTOM_IO = ctx_flags.CUSTOM_IO;
    if (flags.DISCARD_CORRUPT) fmt.ctx_flags_DISCARD_CORRUPT = ctx_flags.DISCARD_CORRUPT;
    if (flags.FLUSH_PACKETS) fmt.ctx_flags_FLUSH_PACKETS = ctx_flags.FLUSH_PACKETS;
    if (flags.BITEXACT) fmt.ctx_flags_BITEXACT = ctx_flags.BITEXACT;
    if (flags.SORT_DTS) fmt.ctx_flags_SORT_DTS = ctx_flags.SORT_DTS;
    if (flags.PRIV_OPT) fmt.ctx_flags_PRIV_OPT = ctx_flags.PRIV_OPT;
    if (flags.FAST_SEEK) fmt.ctx_flags_FAST_SEEK = ctx_flags.FAST_SEEK;
    if (flags.SHORTEST) fmt.ctx_flags_SHORTEST = ctx_flags.SHORTEST;
    if (!flags.AUTO_BSF) fmt.ctx_flags_AUTO_BSF = ctx_flags.AUTO_BSF; // default is true
  }
  if (metadata) {
    fmt.metadata = Object.keys(metadata).reduce((l, r) => {
      l[`metadata_${r}`] = metadata[r];
      return l;
    }, {});
  }
  if (debug) {
    if (debug.TS) fmt.debug_TS = debug.TS;
  }
  if (event_flags) {
    if (event_flags.METADATA_UPDATED)
      fmt.event_flags_METADATA_UPDATED = event_flags.METADATA_UPDATED;
  }
  if (avio_flags) {
    if (avio_flags.READ) fmt.avio_flags_READ = avio_flags.READ;
    if (avio_flags.WRITE) fmt.avio_flags_WRITE = avio_flags.WRITE;
    if (avio_flags.NONBLOCK) fmt.avio_flags_NONBLOCK = avio_flags.NONBLOCK;
    if (avio_flags.DIRECT) fmt.avio_flags_DIRECT = avio_flags.DIRECT;
  }
  return fmt;
};

const formatFromRedis = ({ nb_streams, oformat, iformat,  url, duration, bit_rate,
  packet_size, max_delay, flags_GENPTS, flags_IGNIDX, flags_NONBLOCK, flags_IGNDTS,
  flags_NOFILLIN, flags_NOPARSE, flags_NOBUFFER, flags_CUSTOM_IO,
  flags_DISCARD_CORRUPT, flags_FLUSH_PACKETS, flags_BITEXACT, flags_SORT_DTS,
  flags_PRIV_OPT, flags_FAST_SEEK, flags_SHORTEST, flags_AUTO_BSF,
  probesize, max_analyze_duration, max_index_size, max_picture_buffer,
  start_time_realtime, fps_probe_size, error_recognition, debug_TS,
  max_interleave_delta, strict_std_compliance, event_flags_METADATA_UPDATED,
  max_ts_probe, avoid_negative_ts, audio_preload, max_chunk_duration,
  max_chunk_size, use_wallclock_as_timestamps, avio_flags_READ,
  avio_flags_WRITE, avio_flags_NONBLOCK, avio_flags_DIRECT,
  skip_initial_bytes, correct_ts_overflow, seek2any, flush_packets,
  format_probesize, codec_whitelist, format_whitelist, metadata_header_padding,
  output_ts_offset, dump_separator, protocol_whitelist, protocol_blacklist,
  max_streams, skip_estimate_duration_from_pts, interleaved, ...etc}) => {
  let fmt = { nb_streams: parseInt(nb_streams.toString()) };
  if (oformat) fmt.oformat = oformat.toString();
  if (iformat) fmt.iformat = iformat.toString();
  // TODO priv_data: null,
  if (url) fmt.url = url.toString();
  // No start_time ... not user settable
  if (duration) fmt.duration = parseInt(duration.toString());
  if (bit_rate) fmt.bit_rate = parseInt(bit_rate.toString());
  if (packet_size) fmt.packet_size = parseInt(packet_size.toString());
  if (max_delay) fmt.max_delay = parseInt(max_delay.toString());
  if (flags_GENPTS || flags_IGNIDX || flags_NONBLOCK || flags_IGNDTS ||
    flags_NOFILLIN || flags_NOPARSE || flags_NOBUFFER || flags_CUSTOM_IO ||
    flags_DISCARD_CORRUPT || flags_FLUSH_PACKETS || flags_BITEXACT ||
    flags_SORT_DTS || flags_PRIV_OPT || flags_FAST_SEEK || flags_SHORTEST ||
    flags_AUTO_BSF) {
    fmt.flags = {
      GENPTS: flags_GENPTS && flags_GENPTS[0] === 116,
      IGNIDX: flags_IGNIDX && flags_IGNIDX[0] === 116,
      NONBLOCK: flags_NONBLOCK && flags_NONBLOCK[0] === 116,
      IGNDTS: flags_IGNDTS && flags_IGNDTS[0] === 116,
      NOFILLIN: flags_NOFILLIN && flags_NOFILLIN[0] === 116,
      NOPARSE: flags_NOPARSE && flags_NOPARSE[0] === 116,
      NOBUFFER: flags_NOBUFFER && flags_NOBUFFER[0] === 116,
      CUSTOM_IO: flags_CUSTOM_IO && flags_CUSTOM_IO[0] === 116,
      DISCARD_CORRUPT: flags_DISCARD_CORRUPT && flags_DISCARD_CORRUPT[0] === 116,
      FLUSH_PACKETS: flags_FLUSH_PACKETS && flags_FLUSH_PACKETS[0] === 116,
      BITEXACT: flags_BITEXACT && flags_BITEXACT[0] === 116,
      SORT_DTS: flags_SORT_DTS && flags_SORT_DTS[0] === 116,
      PRIV_OPT: flags_PRIV_OPT && flags_PRIV_OPT[0] === 116,
      FAST_SEEK: flags_FAST_SEEK && flags_FAST_SEEK[0] === 116,
      SHORTEST: flags_SHORTEST && flags_SHORTEST[0] === 116,
      AUTO_BSF: flags_AUTO_BSF && flags_AUTO_BSF[0] === 116
    }; }
  if (probesize) fmt.probesize = parseInt(probesize.toString());
  if (max_analyze_duration) fmt.max_analyze_duration = parseInt(max_analyze_duration.toString());
  // TODO key if (key) fmt.key = key;
  // TODO programs
  if (max_index_size) fmt.max_index_size = parseInt(max_index_size.toString());
  if (max_picture_buffer) fmt.max_picture_buffer = parseInt(max_picture_buffer.toStrintg());
  if (start_time_realtime) fmt.start_time_realtime = parseInt(start_time_realtime.toString());
  if (fps_probe_size) fmt.fps_probe_size = parseInt(fps_probe_size.toString());
  if (error_recognition) fmt.error_recognition = parseInt(error_recognition.toString());
  if (debug_TS) {
    fmt.debug = {
      TS: debug_TS && debug_TS[0] === 116
    }; }
  if (max_interleave_delta) fmt.max_interleave_delta = parseInt(max_interleave_delta.toString());
  if (strict_std_compliance) fmt.strict_std_compliance = strict_std_compliance.toString();
  if (event_flags_METADATA_UPDATED) {
    fmt.event_flags = {
      METADATA_UPDATED: event_flags_METADATA_UPDATED && event_flags_METADATA_UPDATED[0] === 116
    }; }
  if (max_ts_probe) fmt.max_ts_probe = parseInt(max_ts_probe.toString());
  if (avoid_negative_ts) fmt.avoid_negative_ts = avoid_negative_ts.toString();
  if (audio_preload) fmt.audio_preload = parseInt(audio_preload.toString());
  if (max_chunk_duration) fmt.max_chunk_duration = parseInt(max_chunk_duration.toString());
  if (max_chunk_size) fmt.max_chunk_size = parseInt(max_chunk_size.toString());
  if (use_wallclock_as_timestamps) fmt.use_wallclock_as_timestamps = use_wallclock_as_timestamps[0] === 116;
  if (avio_flags_READ || avio_flags_WRITE || avio_flags_NONBLOCK || avio_flags_DIRECT) {
    fmt.avio_flags = {
      READ: avio_flags_READ && avio_flags_READ[0] === 116,
      WRITE: avio_flags_WRITE && avio_flags_WRITE[0] === 116,
      NONBLOCK: avio_flags_NONBLOCK && avio_flags_NONBLOCK[0] === 116,
      DIRECT: avio_flags_DIRECT && avio_flags_DIRECT[0] === 116
    }; }
  // no duration_estimation_method - libav automatic
  if (skip_initial_bytes) fmt.skip_initial_bytes = parseInt(skip_initial_bytes.toString());
  if (correct_ts_overflow) fmt.correct_ts_overflow = correct_ts_overflow[0] === 116;
  if (seek2any) fmt.seek2any = seek2any[0] === 116;
  if (flush_packets) fmt.flush_packets = parseInt(flush_packets.toString());
  // probe_score set by libav
  if (format_probesize) fmt.format_probesize = parseInt(format_probesize.toString());
  if (codec_whitelist) fmt.codec_whitelist = codec_whitelist.toString();
  if (format_whitelist) fmt.format_whitelist = format_whitelist.toString();
  // io_repositioned set by libav
  if (metadata_header_padding) fmt.metadata_header_padding = parseInt(metadata_header_padding.toString());
  if (output_ts_offset) fmt.output_ts_offset = parseInt(output_ts_offset.toString());
  if (dump_separator) fmt.dump_separator = dump_separator.toString();
  if (protocol_whitelist) fmt.protocol_whitelist = protocol_whitelist.toString();
  if (protocol_blacklist) fmt.protocol_blacklist = protocol_blacklist.toString();
  if (max_streams) fmt.max_streams = parseInt(max_streams.toString());
  if (skip_estimate_duration_from_pts) fmt.skip_estimate_duration_from_pts = skip_estimate_duration_from_pts[0] === 116;
  if (interleaved) fmt.interleaved = interleaved[0] === 116;
  fmt.metadata = Object.keys(etc)
    .filter(x => x.startsWith('metadata_'))
    .reduce((l, r) => {
      l[r.slice(9)] = etc[r];
      return l; 
    }, {});

  return fmt;
};

test('Roundtrip format', async t => {
  let fmt = beamcoder.format();
  let redis = new Redis();
  console.log(fmt);
  console.log(formatToRedis(fmt.toJSON()));
  await redis.quit();
  t.end();
});
