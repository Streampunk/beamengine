/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2018  Streampunk Media Ltd.

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

/* API sketch

  let fmt = await beamcoder.metadata(<string>); // read from url

  let fmt = awaitbeamcoder.metadata(<buffer>); // use buffer to probe

  fmt is {
    type: "format",
    container: { <container> },
    streams: [],
    metadata: {
      <key/value pairs>
    }
  }

*/

#include "format.h"

void formatExecute(napi_env env, void* data) {
  formatCarrier* c = (formatCarrier*) data;

  int ret;

  if ((ret = avformat_open_input(&c->format, c->filename, nullptr, nullptr))) {
    c->status = BEAMCODER_ERROR_START;
    c->errorMsg = avErrorMsg("Problem opening input format: ", ret);
    return;
  }

  if ((ret = avformat_find_stream_info(c->format, nullptr))) {
    printf("DEBUG: Counld not find stream info for file %s, return value %i.",
      c->filename, ret);
  }
}

void formatComplete(napi_env env,  napi_status asyncStatus, void* data) {
  formatCarrier* c = (formatCarrier*) data;
  napi_value result, value, item, prop, subprop;
  AVDictionaryEntry* tag = nullptr;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Format creator failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  c->status = napi_create_string_utf8(env, "format", NAPI_AUTO_LENGTH, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", value);
  REJECT_STATUS;

  c->status = napi_create_object(env, &value);
  REJECT_STATUS;
  c->status = napi_create_string_utf8(env, "container", NAPI_AUTO_LENGTH, &item);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, value, "type", item);
  if (c->format->iformat->name != nullptr) {
    REJECT_STATUS;
    c->status = napi_create_string_utf8(env, c->format->iformat->name, NAPI_AUTO_LENGTH, &item);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, value, "name", item);
  }
  REJECT_STATUS;
  if (c->format->iformat->long_name != nullptr) {
    c->status = napi_create_string_utf8(env, c->format->iformat->long_name, NAPI_AUTO_LENGTH, &item);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, value, "longName", item);
    REJECT_STATUS;
  }
  if (c->format->iformat->mime_type != nullptr) {
    c->status = napi_create_string_utf8(env, c->format->iformat->mime_type, NAPI_AUTO_LENGTH, &item);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, value, "mimeType", item);
    REJECT_STATUS;
  }
  c->status = napi_set_named_property(env, result, "container", value);
  REJECT_STATUS;

  c->status = napi_create_array(env, &value);
  REJECT_STATUS;
  for ( uint32_t x = 0 ; x < c->format->nb_streams ; x++ ) {
    AVStream *stream = c->format->streams[x];
    AVCodecParameters *codec = stream->codecpar;
    AVCodec *codecImpl = avcodec_find_decoder(codec->codec_id);

    c->status = napi_create_object(env, &item);
    REJECT_STATUS;
    c->status = napi_create_string_utf8(env, "stream", NAPI_AUTO_LENGTH, &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, item, "type", prop);
    REJECT_STATUS;

    if (stream->index != x) {
      c->status = napi_create_int32(env, stream->index, &prop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, item, "index", prop);
      REJECT_STATUS;
    }

    c->status = napi_create_array(env, &prop);
    REJECT_STATUS;
    c->status = napi_create_int32(env, stream->time_base.num, &subprop);
    REJECT_STATUS;
    c->status = napi_set_element(env, prop, 0, subprop);
    REJECT_STATUS;
    c->status = napi_create_int32(env, stream->time_base.den, &subprop);
    REJECT_STATUS;
    c->status = napi_set_element(env, prop, 1, subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, item, "timeBase", prop);
    REJECT_STATUS;

    if (stream->start_time != AV_NOPTS_VALUE) {
      c->status = napi_create_int64(env, stream->start_time, &prop);
      REJECT_STATUS;
    }
    else {
      c->status = napi_get_null(env, &prop);
      REJECT_STATUS;
    }
    c->status = napi_set_named_property(env, item, "startTime", prop);
    REJECT_STATUS;

    if (stream->duration != AV_NOPTS_VALUE) {
      c->status = napi_create_int64(env, stream->duration, &prop);
      REJECT_STATUS;
    }
    else {
      c->status = napi_get_null(env, &prop);
      REJECT_STATUS;
    }
    c->status = napi_set_named_property(env, item, "duration", prop);
    REJECT_STATUS;

    c->status = napi_create_int64(env, stream->nb_frames, &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, item, "nbFrames", prop);
    REJECT_STATUS;

    if (stream->sample_aspect_ratio.num != 0) {
      c->status = napi_create_array(env, &prop);
      REJECT_STATUS;
      c->status = napi_create_int32(env, stream->sample_aspect_ratio.num, &subprop);
      REJECT_STATUS;
      c->status = napi_set_element(env, prop, 0, subprop);
      REJECT_STATUS;
      c->status = napi_create_int32(env, stream->sample_aspect_ratio.den, &subprop);
      REJECT_STATUS;
      c->status = napi_set_element(env, prop, 1, subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, item, "sampleAspectRatio", prop);
      REJECT_STATUS;
    }

    c->status = napi_create_object(env, &prop);
    REJECT_STATUS;
    while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
      c->status = napi_create_string_utf8(env, tag->value, NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, tag->key, subprop);
      REJECT_STATUS;
    }
    c->status = napi_set_named_property(env, item, "metadata", prop);
    REJECT_STATUS;

    if (stream->avg_frame_rate.num != 0) {
      c->status = napi_create_array(env, &prop);
      REJECT_STATUS;
      c->status = napi_create_int32(env, stream->avg_frame_rate.num, &subprop);
      REJECT_STATUS;
      c->status = napi_set_element(env, prop, 0, subprop);
      REJECT_STATUS;
      c->status = napi_create_int32(env, stream->avg_frame_rate.den, &subprop);
      REJECT_STATUS;
      c->status = napi_set_element(env, prop, 1, subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, item, "avgFrameRate", prop);
      REJECT_STATUS;
    }

    c->status = napi_create_object(env, &prop);
    REJECT_STATUS;
    c->status = napi_create_string_utf8(env, "codec", NAPI_AUTO_LENGTH, &subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, "type", subprop);
    REJECT_STATUS;
    c->status = napi_create_string_utf8(env, av_get_media_type_string(codec->codec_type),
      NAPI_AUTO_LENGTH, &subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, "codecType", subprop);
    REJECT_STATUS;

    if (codecImpl != nullptr) {
      c->status = napi_create_string_utf8(env, codecImpl->name, NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "name", subprop);
      REJECT_STATUS;
      c->status = napi_create_string_utf8(env, codecImpl->long_name, NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "longName", subprop);
      REJECT_STATUS;
    }
    else {
      c->status = napi_create_string_utf8(env, "unknown", NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "name", subprop);
      REJECT_STATUS;
    }

    char codecTag[64];
    av_get_codec_tag_string(codecTag, 64, codec->codec_tag);
    c->status = napi_create_string_utf8(env, codecTag, NAPI_AUTO_LENGTH, &subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, "codecTag", subprop);
    REJECT_STATUS;

    if (codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      const char* pixelFormatName = av_get_pix_fmt_name((AVPixelFormat) codec->format);
      if (pixelFormatName != nullptr) {
        c->status = napi_create_string_utf8(env, pixelFormatName, NAPI_AUTO_LENGTH, &subprop);
        REJECT_STATUS;
        c->status = napi_set_named_property(env, prop, "pixelFormat", subprop);
        REJECT_STATUS;
      }

      c->status = napi_create_int32(env, codec->width, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "width", subprop);
      REJECT_STATUS;

      c->status = napi_create_int32(env, codec->height, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "height", subprop);
      REJECT_STATUS;

      switch (codec->field_order) {
        case AV_FIELD_PROGRESSIVE:
          c->status = napi_create_string_utf8(env, "progressive", NAPI_AUTO_LENGTH, &subprop);
          break;
        case AV_FIELD_TT:
          c->status = napi_create_string_utf8(env,
            "top coded_first, top displayed first", NAPI_AUTO_LENGTH, &subprop);
          break;
        case AV_FIELD_BB:
          c->status = napi_create_string_utf8(env,
            "bottom coded first, bottom displayed first", NAPI_AUTO_LENGTH, &subprop);
          break;
        case AV_FIELD_TB:
          c->status = napi_create_string_utf8(env,
            "top coded first, bottom displayed first", NAPI_AUTO_LENGTH, &subprop);
          break;
        case AV_FIELD_BT:
          c->status = napi_create_string_utf8(env,
            "bottom coded first, top displayed first", NAPI_AUTO_LENGTH, &subprop);
          break;
        default:
        case AV_FIELD_UNKNOWN:
          c->status = napi_get_null(env, &subprop);
          break;
      }
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "fieldOrder", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_range_name(codec->color_range), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "colorRange", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_primaries_name(codec->color_primaries), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "colorPrimaries", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_transfer_name(codec->color_trc), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "transferCharacteristic", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_space_name(codec->color_space), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "colorSpace", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_chroma_location_name(codec->chroma_location), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "chromaLocation", subprop);
      REJECT_STATUS;
    } // Video-only properties

    if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      const char* sampleFormatName = av_get_sample_fmt_name((AVSampleFormat) codec->format);
      if (sampleFormatName != nullptr) {
        c->status = napi_create_string_utf8(env, sampleFormatName, NAPI_AUTO_LENGTH, &subprop);
        REJECT_STATUS;
        c->status = napi_set_named_property(env, prop, "sampleFormat", subprop);
        REJECT_STATUS;
      }


      c->status = napi_create_int32(env, codec->channels, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "channels", subprop);
      REJECT_STATUS;

      c->status = napi_create_int32(env, codec->sample_rate, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "sampleRate", subprop);
      REJECT_STATUS;

      c->status = napi_create_int32(env, codec->block_align, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "blockAlign", subprop);
      REJECT_STATUS;

      if (codec->channel_layout != 0) {
        char cl[64];
        av_get_channel_layout_string(cl, 64, codec->channels, codec->channel_layout);
        c->status = napi_create_string_utf8(env, cl, NAPI_AUTO_LENGTH, &subprop);
        REJECT_STATUS;
        c->status = napi_set_named_property(env, prop, "channelLayout", subprop);
        REJECT_STATUS;
      }
    } // Audio-only properties

    c->status = napi_create_int32(env, codec->bit_rate, &subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, "bitrate", subprop);
    REJECT_STATUS;

    if (codec->profile >= 0) {
      const char* profileName = avcodec_profile_name(codec->codec_id, codec->profile);
      if (profileName != nullptr) {
        c->status = napi_create_string_utf8(env, profileName, NAPI_AUTO_LENGTH, &subprop);
        REJECT_STATUS;
        c->status = napi_set_named_property(env, prop, "profile", subprop);
        REJECT_STATUS;
      }
    }

    if (codec->level >= 0) {
      c->status = napi_create_int32(env, codec->level, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "level", subprop);
      REJECT_STATUS;
    }

    c->status = napi_set_named_property(env, item, "codec", prop);
    REJECT_STATUS;

    c->status = napi_set_element(env, value, stream->index, item);
    REJECT_STATUS;
  } // End of stream section
  c->status = napi_set_named_property(env, result, "streams", value);
  REJECT_STATUS;

  c->status = napi_create_object(env, &prop);
  REJECT_STATUS;
  while ((tag = av_dict_get(c->format->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
    c->status = napi_create_string_utf8(env, tag->value, NAPI_AUTO_LENGTH, &subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, tag->key, subprop);
    REJECT_STATUS;
  }
  c->status = napi_set_named_property(env, result, "metadata", prop);
  REJECT_STATUS;

  c->status = napi_create_double(env, (double) c->format->start_time / AV_TIME_BASE, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "startTime", prop);
  REJECT_STATUS;

  c->status = napi_create_double(env, (double) c->format->duration / AV_TIME_BASE, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "duration", prop);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->format->bit_rate, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "bitrate", prop);
  REJECT_STATUS;

  if (c->format->start_time_realtime != AV_NOPTS_VALUE) {
    c->status = napi_create_int64(env, c->format->start_time_realtime, &prop);
  }
  else {
    c->status = napi_get_null(env, &prop);
  }
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "startRealtime", prop);
  REJECT_STATUS;

  c->status = napi_create_uint32(env, c->format->packet_size, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "packetSize", prop);
  REJECT_STATUS;

  c->status = napi_create_uint32(env, c->format->max_delay, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "maxDelay", prop);
  REJECT_STATUS;

  c->status = napi_create_external(env, c->format, formatFinalizer, nullptr, &prop);
  REJECT_STATUS;
  c->format = nullptr;
  c->status = napi_set_named_property(env, result, "_format", prop);
  REJECT_STATUS;

  c->status = napi_create_function(env, "readFrame", NAPI_AUTO_LENGTH, readFrame,
    nullptr, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "readFrame", prop);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value format(napi_env env, napi_callback_info info) {
  napi_value resourceName, promise;
  napi_valuetype type;
  size_t strLen;
  formatCarrier* c = new formatCarrier;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];

  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;

  if (argc < 1) {
    REJECT_ERROR_RETURN("Format requires a filename, URL or buffer.",
      BEAMCODER_INVALID_ARGS);
  }

  c->status = napi_typeof(env, args[0], &type);
  REJECT_RETURN;

  if (type == napi_string) {
    c->status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
    REJECT_RETURN;
    c->filename = (const char *) malloc((strLen + 1) * sizeof(char));
    c->status = napi_get_value_string_utf8(env, args[0], (char *) c->filename, strLen + 1, &strLen);
    REJECT_RETURN;
  }

  c->status = napi_create_string_utf8(env, "Format", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, formatExecute,
    formatComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

void formatFinalizer(napi_env env, void* data, void* hint) {
  AVFormatContext *fmtCtx = (AVFormatContext*) data;
  avformat_close_input(&fmtCtx);
}

void readFrameExecute(napi_env env, void* data) {
  readFrameCarrier* c = (readFrameCarrier*) data;
  int ret;

  if ((ret = av_read_frame(c->format, c->packet))) {
    c->status = BEAMCODER_ERROR_READ_FRAME;
    c->errorMsg = avErrorMsg("Problem reading frame: ", ret);
    return;
  }
}

void readFrameComplete(napi_env env, napi_status asyncStatus, void* data) {
  readFrameCarrier* c = (readFrameCarrier*) data;
  napi_value result, value;
  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Read frame failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  c->status = napi_create_string_utf8(env, "packet", NAPI_AUTO_LENGTH, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", value);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->packet->stream_index, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "stream", value);
  REJECT_STATUS;

  // TODO convert to use BigInts when more stable in Javascript
  if (c->packet->pts == AV_NOPTS_VALUE) {
    c->status = napi_get_null(env, &value);
  }
  else {
    c->status = napi_create_int64(env, c->packet->pts, &value);
  }
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "pts", value);
  REJECT_STATUS;

  c->status = napi_create_int64(env, c->packet->dts, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "dts", value);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->packet->size, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "size", value);
  REJECT_STATUS;

  c->status = napi_create_int64(env, c->packet->duration, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "duration", value);
  REJECT_STATUS;

  if (c->packet->pos >= 0) {
    c->status = napi_create_int64(env, c->packet->pos, &value);
  }
  else {
    c->status = napi_get_null(env, &value);
  }
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "pos", value);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value readFrame(napi_env env, napi_callback_info info) {
  napi_value resourceName, promise, formatJS, formatExt;
  readFrameCarrier* c = new readFrameCarrier;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 0;
  c->status = napi_get_cb_info(env, info, &argc, nullptr, &formatJS, nullptr);
  REJECT_RETURN;
  c->status = napi_get_named_property(env, formatJS, "_format", &formatExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, formatExt, (void**) &c->format);
  REJECT_RETURN;

  c->status = napi_create_reference(env, formatJS, 1, &c->passthru);
  REJECT_RETURN;

  c->status = napi_create_string_utf8(env, "ReadFrame", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, readFrameExecute,
    readFrameComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}
