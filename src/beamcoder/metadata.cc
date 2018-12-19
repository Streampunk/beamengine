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

#include "metadata.h"

void metadataExecute(napi_env env, void* data) {
  metadataCarrier* c = (metadataCarrier*) data;

  int ret;

  if ((ret = avformat_open_input(&c->format, c->filename, nullptr, nullptr))) {
    printf("DEBUG: Could not read file %s\n", c->filename);
    c->status = BEAMCODER_ERROR_START;
    c->errorMsg = "Failed to open file.";
    return;
  }

  if ((ret = avformat_find_stream_info(c->format, nullptr))) {
    printf("DEBUG: Counld not find stream info for file %s, retuen value %i.",
      c->filename, ret);
  }
}

void metadataComplete(napi_env env,  napi_status asyncStatus, void* data) {
  metadataCarrier* c = (metadataCarrier*) data;
  napi_value result, value, item, prop, subprop, nested;
  AVDictionaryEntry* tag = nullptr;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async capture creator failed to complete.";
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
    switch (codec->codec_type) {
      case AVMEDIA_TYPE_VIDEO:
        c->status = napi_create_string_utf8(env, "video", NAPI_AUTO_LENGTH, &subprop);
        break;
      case AVMEDIA_TYPE_AUDIO:
        c->status = napi_create_string_utf8(env, "audio", NAPI_AUTO_LENGTH, &subprop);
        break;
      case AVMEDIA_TYPE_DATA: // Opaque data information usually continuous.
        c->status = napi_create_string_utf8(env, "data", NAPI_AUTO_LENGTH, &subprop);
        break;
      case AVMEDIA_TYPE_SUBTITLE:
        c->status = napi_create_string_utf8(env, "subtitle", NAPI_AUTO_LENGTH, &subprop);
        break;
      case AVMEDIA_TYPE_ATTACHMENT: // Opaque data information usually sparse.
        c->status = napi_create_string_utf8(env, "attachment", NAPI_AUTO_LENGTH, &subprop);
        break;
      case AVMEDIA_TYPE_NB:
        c->status = napi_create_string_utf8(env, "nb", NAPI_AUTO_LENGTH, &subprop);
        break;
      default:
      case AVMEDIA_TYPE_UNKNOWN:
        c->status = napi_create_string_utf8(env, "unknown", NAPI_AUTO_LENGTH, &subprop);
        break;
    }
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

    char codecTag[5];
    codecTag[3] = (char) (codec->codec_tag >> 24);
    codecTag[2] = (char) ((codec->codec_tag >> 16) & 0xff);
    codecTag[1] = (char) ((codec->codec_tag >> 8) & 0xff);
    codecTag[0] = (char) (codec->codec_tag & 0xff);
    codecTag[4] = '\0';
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

      switch (codec->color_range) {
        case AVCOL_RANGE_MPEG:
          c->status = napi_create_string_utf8(env, "MPEG", NAPI_AUTO_LENGTH, &subprop);
          break;
        case AVCOL_RANGE_JPEG:
          c->status = napi_create_string_utf8(env, "JPEG", NAPI_AUTO_LENGTH, &subprop);
          break;
        case AVCOL_RANGE_NB:
          c->status = napi_create_string_utf8(env, "nb", NAPI_AUTO_LENGTH, &subprop);
          break;
        default:
        case AVCOL_RANGE_UNSPECIFIED:
          c->status = napi_create_string_utf8(env, "unspecified", NAPI_AUTO_LENGTH, &subprop);
          break;
      }
      REJECT_STATUS;
      c->status = napi_set_named_property(env, prop, "colorRange", subprop);
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
    }

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

    c->status = napi_create_array(env, &subprop);
    REJECT_STATUS;
    c->status = napi_create_int32(env, codec->sample_aspect_ratio.num, &nested);
    REJECT_STATUS;
    c->status = napi_set_element(env, subprop, 0, nested);
    REJECT_STATUS;
    c->status = napi_create_int32(env, codec->sample_aspect_ratio.den, &nested);
    REJECT_STATUS;
    c->status = napi_set_element(env, subprop, 1, nested);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, prop, "sampleAspectRatio", subprop);
    REJECT_STATUS;

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

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value metadata(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, resourceName, promise;
  napi_valuetype type;
  size_t bufSize, strLen;
  metadataCarrier* c = new metadataCarrier;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];

  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;

  if (argc < 1) {
    REJECT_ERROR_RETURN("Metadata requires a filename, URL or buffer.",
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

  c->status = napi_create_string_utf8(env, "Metadata", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, metadataExecute,
    metadataComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

//?''
