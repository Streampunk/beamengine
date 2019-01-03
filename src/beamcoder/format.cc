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

  c->status = beam_set_string_utf8(env, result, "type", "format");
  REJECT_STATUS;

  c->status = napi_create_object(env, &value);
  REJECT_STATUS;
  c->status = beam_set_string_utf8(env, value, "type", "container");
  REJECT_STATUS;
  if (c->format->iformat->name != nullptr) {
    c->status = beam_set_string_utf8(env, value, "name", (char*) c->format->iformat->name);
    REJECT_STATUS;
  }
  if (c->format->iformat->long_name != nullptr) {
    c->status = beam_set_string_utf8(env, value, "long_name", (char*) c->format->iformat->long_name);
    REJECT_STATUS;
  }
  if (c->format->iformat->mime_type != nullptr) {
    c->status = beam_set_string_utf8(env, value, "mime_type", (char*) c->format->iformat->mime_type);
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
    c->status = beam_set_string_utf8(env, item, "type", "stream");
    REJECT_STATUS;

    if (stream->index != x) {
      c->status = beam_set_int32(env, item, "index", stream->index);
      REJECT_STATUS;
    }

    c->status = beam_set_rational(env, item, "time_base", stream->time_base);
    REJECT_STATUS;

    if (stream->start_time != AV_NOPTS_VALUE) {
      c->status = napi_create_int64(env, stream->start_time, &prop);
      REJECT_STATUS;
    }
    else {
      c->status = napi_get_null(env, &prop);
      REJECT_STATUS;
    }
    c->status = napi_set_named_property(env, item, "start_time", prop);
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

    c->status = beam_set_int64(env, item, "nb_frames", stream->nb_frames);
    REJECT_STATUS;

    if (stream->sample_aspect_ratio.num != 0) {
      c->status = beam_set_rational(env, item, "sample_aspect_ratio", stream->sample_aspect_ratio);
      REJECT_STATUS;
    }

    c->status = napi_create_object(env, &prop);
    REJECT_STATUS;
    while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
      c->status = beam_set_string_utf8(env, prop, tag->key, tag->value);
      REJECT_STATUS;
    }
    c->status = napi_set_named_property(env, item, "metadata", prop);
    REJECT_STATUS;

    if (stream->avg_frame_rate.num != 0) {
      c->status = beam_set_rational(env, item, "avg_frame_rate", stream->avg_frame_rate);
      REJECT_STATUS;
    }

    c->status = napi_create_object(env, &prop);
    REJECT_STATUS;
    c->status = beam_set_string_utf8(env, prop, "type", "codec");
    REJECT_STATUS;

    c->status = beam_set_string_utf8(env, prop, "codec_type",
      (char*) av_get_media_type_string(codec->codec_type));
    REJECT_STATUS;

    if (codecImpl != nullptr) {
      c->status = beam_set_string_utf8(env, prop, "name", (char*) codecImpl->name);
      REJECT_STATUS;
      c->status = beam_set_string_utf8(env, prop, "long_name", (char*) codecImpl->long_name);
      REJECT_STATUS;
    }
    else {
      c->status = beam_set_string_utf8(env, prop, "name", "unknown");
      REJECT_STATUS;
    }

    c->status = beam_set_int32(env, prop, "codec_id", codec->codec_id);
    REJECT_STATUS;

    char codecTag[64];
    av_get_codec_tag_string(codecTag, 64, codec->codec_tag);
    c->status = napi_create_string_utf8(env, codecTag, NAPI_AUTO_LENGTH, &subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, "codec_tag", subprop);
    REJECT_STATUS;

    if (codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      const char* pixelFormatName = av_get_pix_fmt_name((AVPixelFormat) codec->format);
      if (pixelFormatName != nullptr) {
        c->status = beam_set_string_utf8(env, prop, "format", (char*) pixelFormatName);
        REJECT_STATUS;
      }

      c->status = beam_set_int32(env, prop, "width", codec->width);
      REJECT_STATUS;
      c->status = beam_set_int32(env, prop, "height", codec->height);
      REJECT_STATUS;

      c->status = beam_set_string_utf8(env, prop, "field_order",
        (char*) beam_field_order_name(codec->field_order));
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_range_name(codec->color_range), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "color_range", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_primaries_name(codec->color_primaries), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "color_primaries", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_transfer_name(codec->color_trc), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "color_trc", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_color_space_name(codec->color_space), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "color_space", subprop);
      REJECT_STATUS;

      c->status = napi_create_string_utf8(env,
        av_chroma_location_name(codec->chroma_location), NAPI_AUTO_LENGTH, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "chroma_location", subprop);
      REJECT_STATUS;
    } // Video-only properties

    if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      const char* sampleFormatName = av_get_sample_fmt_name((AVSampleFormat) codec->format);
      if (sampleFormatName != nullptr) {
        c->status = napi_create_string_utf8(env, sampleFormatName, NAPI_AUTO_LENGTH, &subprop);
        REJECT_STATUS;
        c->status = napi_set_named_property(env, prop, "format", subprop);
        REJECT_STATUS;
      }

      c->status = napi_create_int32(env, codec->channels, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "channels", subprop);
      REJECT_STATUS;

      c->status = napi_create_int32(env, codec->sample_rate, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "sample_rate", subprop);
      REJECT_STATUS;

      c->status = napi_create_int32(env, codec->block_align, &subprop);
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "block_align", subprop);
      REJECT_STATUS;

      if (codec->channel_layout != 0) {
        char cl[64];
        av_get_channel_layout_string(cl, 64, codec->channels, codec->channel_layout);
        c->status = napi_create_string_utf8(env, cl, NAPI_AUTO_LENGTH, &subprop);
        REJECT_STATUS;
        c->status = napi_set_named_property(env, prop, "channe_layout", subprop);
        REJECT_STATUS;
      }
    } // Audio-only properties

    c->status = napi_create_int32(env, codec->bit_rate, &subprop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, "bit_rate", subprop);
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
  c->status = napi_set_named_property(env, result, "start_time", prop);
  REJECT_STATUS;

  c->status = napi_create_double(env, (double) c->format->duration / AV_TIME_BASE, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "duration", prop);
  REJECT_STATUS;

  c->status = napi_create_int32(env, c->format->bit_rate, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "bit_rate", prop);
  REJECT_STATUS;

  if (c->format->start_time_realtime != AV_NOPTS_VALUE) {
    c->status = napi_create_int64(env, c->format->start_time_realtime, &prop);
  }
  else {
    c->status = napi_get_null(env, &prop);
  }
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "start_time_realtime", prop);
  REJECT_STATUS;

  c->status = napi_create_uint32(env, c->format->packet_size, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "packet_size", prop);
  REJECT_STATUS;

  c->status = napi_create_uint32(env, c->format->max_delay, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "max_delay", prop);
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

  c->status = napi_create_function(env, "seekFrame", NAPI_AUTO_LENGTH, seekFrame,
    nullptr, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "seekFrame", prop);
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
  AVBufferRef* hintRef;
  int64_t externalMemory;

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

  c->status = napi_get_boolean(env, (c->packet->flags & AV_PKT_FLAG_KEY) != 0, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "key", value);
  REJECT_STATUS;

  c->status = napi_get_boolean(env, (c->packet->flags & AV_PKT_FLAG_KEY) != 0, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "key", value);
  REJECT_STATUS;

  if ((c->packet->flags & AV_PKT_FLAG_CORRUPT) != 0) {
    c->status = napi_get_boolean(env, true, &value);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "corrupt", value);
    REJECT_STATUS;
  }

  hintRef = av_buffer_ref(c->packet->buf);
  c->status = napi_create_external_buffer(env, hintRef->size, hintRef->data,
    readBufferFinalizer, hintRef, &value);
  REJECT_STATUS;
  c->status = napi_adjust_external_memory(env, hintRef->size, &externalMemory);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "data", value);
  REJECT_STATUS;

  c->status = napi_create_external(env, c->packet, packetFinalizer, nullptr, &value);
  REJECT_STATUS;
  c->packet = nullptr;
  c->status = napi_set_named_property(env, result, "_packet", value);
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

void packetFinalizer(napi_env env, void* data, void* hint) {
  AVPacket* pkt = (AVPacket*) data;
  av_packet_free(&pkt);
}

void readBufferFinalizer(napi_env env, void* data, void* hint) {
  AVBufferRef* hintRef = (AVBufferRef*) hint;
  napi_status status;
  int64_t externalMemory;
  status = napi_adjust_external_memory(env, -hintRef->size, &externalMemory);
  if (status != napi_ok) {
    printf("DEBUG: Napi failure to adjust external memory. In beamcoder format.cc readFrameFinalizer.");
  }
  av_buffer_unref(&hintRef);
}

void seekFrameExecute(napi_env env, void *data) {
  seekFrameCarrier* c = (seekFrameCarrier*) data;
  int ret;

  ret = av_seek_frame(c->format, c->streamIndex, c->timestamp, c->flags);
  if (ret < 0) {
    c->status = BEAMCODER_ERROR_SEEK_FRAME;
    c->errorMsg = avErrorMsg("Problem seeking frame: ", ret);
    return;
  }
};

void seekFrameComplete(napi_env env, napi_status asyncStatus, void *data) {
  seekFrameCarrier* c = (seekFrameCarrier*) data;
  napi_value result, value;
  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Seek frame failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_get_null(env, &result);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
};

/*
  let frame = await format.seek({
    streamIndex: 0, // Default is -1 - use primary stream, seek in seconds
    timestamp: 12345, // Timestamp - default units are stream timeBase
    backward: false, // Seek backwards
    byte: false, // Timestamp is a byte position
    any: false, // Select any frame, not just key frames
    frame: false // Timestamp is frame number
  });
*/

napi_value seekFrame(napi_env env, napi_callback_info info) {
  napi_value resourceName, promise, formatJS, formatExt, value;
  napi_valuetype type;
  seekFrameCarrier* c = new seekFrameCarrier;
  bool isArray, bValue;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value argv[1];

  c->status = napi_get_cb_info(env, info, &argc, argv, &formatJS, nullptr);
  REJECT_RETURN;
  c->status = napi_get_named_property(env, formatJS, "_format", &formatExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, formatExt, (void**) &c->format);
  REJECT_RETURN;

  c->status = napi_create_reference(env, formatJS, 1, &c->passthru);
  REJECT_RETURN;

  if ((argc < 1) || (argc > 1)) {
    REJECT_ERROR_RETURN("Seek must have exactly one options object argument.",
      BEAMCODER_INVALID_ARGS);
  }

  c->status = napi_typeof(env, argv[0], &type);
  REJECT_RETURN;
  c->status = napi_is_array(env, argv[0], &isArray);
  REJECT_RETURN;
  if ((type != napi_object) || (isArray == true)) {
    REJECT_ERROR_RETURN("Single argument options object must be an object and not an array.",
      BEAMCODER_INVALID_ARGS);
  }

  c->status = napi_get_named_property(env, argv[0], "streamIndex", &value);
  REJECT_RETURN;
  c->status = napi_typeof(env, value, &type);
  REJECT_RETURN;
  if (type == napi_number) {
    c->status = napi_get_value_int32(env, value, &c->streamIndex);
    REJECT_RETURN;
  }

  c->status = napi_get_named_property(env, argv[0], "timestamp", &value);
  REJECT_RETURN;
  c->status = napi_typeof(env, value, &type);
  REJECT_RETURN;
  if (type != napi_number) {
    REJECT_ERROR_RETURN("Seek must have a timestamp specified.",
      BEAMCODER_INVALID_ARGS);
  }
  c->status = napi_get_value_int64(env, value, &c->timestamp);
  REJECT_RETURN;

  if (c->streamIndex == -1) {
    c->timestamp = c->timestamp * AV_TIME_BASE;
  }

  c->status = napi_get_named_property(env, argv[0], "backward", &value);
  REJECT_RETURN;
  c->status = napi_typeof(env, value, &type);
  REJECT_RETURN;
  if (type == napi_boolean) {
    c->status = napi_get_value_bool(env, value, &bValue);
    REJECT_RETURN;
    c->flags = (bValue) ? c->flags & AVSEEK_FLAG_BACKWARD : c->flags;
  }

  c->status = napi_get_named_property(env, argv[0], "byte", &value);
  REJECT_RETURN;
  c->status = napi_typeof(env, value, &type);
  REJECT_RETURN;
  if (type == napi_boolean) {
    c->status = napi_get_value_bool(env, value, &bValue);
    REJECT_RETURN;
    c->flags = (bValue) ? c->flags & AVSEEK_FLAG_BYTE : c->flags;
  }

  c->status = napi_get_named_property(env, argv[0], "any", &value);
  REJECT_RETURN;
  c->status = napi_typeof(env, value, &type);
  REJECT_RETURN;
  if (type == napi_boolean) {
    c->status = napi_get_value_bool(env, value, &bValue);
    REJECT_RETURN;
    c->flags = (bValue) ? c->flags & AVSEEK_FLAG_ANY : c->flags;
  }

  c->status = napi_get_named_property(env, argv[0], "frame", &value);
  REJECT_RETURN;
  c->status = napi_typeof(env, value, &type);
  REJECT_RETURN;
  if (type == napi_boolean) {
    c->status = napi_get_value_bool(env, value, &bValue);
    REJECT_RETURN;
    c->flags = (bValue) ? c->flags & AVSEEK_FLAG_FRAME : c->flags;
  }

  c->status = napi_typeof(env, argv[0], &type);
  REJECT_RETURN;
  c->status = napi_is_array(env, argv[0], &isArray);
  REJECT_RETURN;
  if ((type != napi_object) || (isArray == true)) {
    REJECT_ERROR_RETURN("Single argument options object must be an object and not an array.",
      BEAMCODER_INVALID_ARGS);
  }

  c->status = napi_create_string_utf8(env, "SeekFrame", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, seekFrameExecute,
    seekFrameComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
};
