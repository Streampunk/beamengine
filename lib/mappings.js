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

const codecParFromRedis = ({ codec_id, codec_tag, extradata,
  format, bit_rate, bits_per_coded_sample, bits_per_raw_sample, profile, level,
  width, height, sample_aspect_ratio_num, sample_aspect_ratio_den, field_order,
  color_range, color_primaries, color_trc, color_space, chroma_location,
  video_delay, channel_layout, channels, sample_rate, block_align, frame_size,
  initial_padding, trailing_padding, seek_preroll }) => {
  let cp = { codec_id: parseInt(codec_id.toString()) };
  if (codec_tag && codec_tag.length > 0) cp.codec_tag = codec_tag.toString();
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

const packetToRedis = ({ type, flags, side_data, ...p }) => { // eslint-disable-line no-unused-vars
  if (flags) {
    p.flags_KEY = flags.KEY;
    p.flags_CORRUPT = flags.CORRUPT;
    p.flags_DISCARD = flags.DISCARD;
    p.flags_TRUSTED = flags.TRUSTED;
    p.flags_DISPOSABLE = flags.DISPOSABLE;
  }
  if (side_data) {
    Object.keys(side_data).reduce((l, r) => {
      if (r !== 'type') l[`side_data_${r}`] = side_data[r];
      return l;
    }, p);
  }
  return p;
};

const packetFromRedis = ({ pts, dts, stream_index, flags_KEY, flags_CORRUPT,
  flags_DISCARD, flags_TRUSTED, flags_DISPOSABLE, duration, pos, size, ...sd }) => {
  let pkt = {
    stream_index: parseInt(stream_index.toString())
  };
  if (pts) pkt.pts = parseInt(pts.toString());
  if (dts) pkt.dts = parseInt(dts.toString());
  if (size) pkt.size = parseInt(size.toString());
  if (flags_KEY || flags_CORRUPT || flags_DISCARD || flags_TRUSTED || flags_DISPOSABLE) {
    pkt.flags = {
      KEY: flags_KEY && flags_KEY[0] === 116,
      CORRUPT: flags_CORRUPT && flags_CORRUPT[0] === 116,
      DISCARD: flags_DISCARD && flags_DISCARD[0] === 116,
      TRUSTED: flags_TRUSTED && flags_TRUSTED[0] === 116,
      DISPOSABLE: flags_DISPOSABLE && flags_DISPOSABLE[0] === 116
    }; }
  if (duration) pkt.duration = parseInt(duration.toString());
  if (pos) pkt.pos = parseInt(pos.toString());
  if (Object.keys(sd).length > 0) {
    pkt.side_data = Object.keys(sd)
      .filter(x => x.startsWith('side_data_'))
      .reduce((l, r) => {
        l[r.slice(10)] = sd[r];
        return l;
      }, {});
  }
  return pkt;
};

const frameToRedis = ({ type, linesize, sample_aspect_ratio,  // eslint-disable-line no-unused-vars
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
      return l;
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

const formatToRedis = ({ type, priv_data, ctx_flags, streams, flags, programs,  // eslint-disable-line no-unused-vars
  metadata, debug, event_flags, avio_flags, __streams, ...fmt }) => {  // eslint-disable-line no-unused-vars
  if (priv_data) {
    Object.keys(priv_data).reduce((l, r) => {
      l[`priv_data_${r}`] = priv_data[r];
      return l;
    }, fmt);
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
    Object.keys(metadata).reduce((l, r) => {
      l[`metadata_${r}`] = metadata[r];
      return l;
    }, fmt);
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
  probesize, max_analyze_duration, max_index_size, max_picture_buffer, key,
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
  if (url) fmt.url = url.toString();
  if (Object.keys(etc).find(x => x.startsWith('priv_data_'))) {
    fmt.priv_data = Object.keys(etc)
      .filter(x => x.startsWith('priv_data_'))
      .reduce((l, r) => {
        l[r.slice(10)] = etc[r].toString();
        return l;
      }, {});
  }
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
  if (key) fmt.key = key;
  // TODO programs
  if (max_index_size) fmt.max_index_size = parseInt(max_index_size.toString());
  if (max_picture_buffer) fmt.max_picture_buffer = parseInt(max_picture_buffer.toString());
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

module.exports = {
  packetToRedis,
  packetFromRedis,
  frameToRedis,
  frameFromRedis,
  codecParToRedis,
  codecParFromRedis,
  streamToRedis,
  streamFromRedis,
  formatToRedis,
  formatFromRedis,
  stringOrNum
};
