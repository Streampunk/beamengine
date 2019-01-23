/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2019  Streampunk Media Ltd.

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

#include "format.h"

napi_value getIFormatName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, iformat->name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, oformat->name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getIFormatLongName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, iformat->long_name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatLongName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, oformat->long_name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getIFormatMimeType(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (iformat->mime_type != nullptr) ? iformat->mime_type : "", NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatMimeType(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (oformat->mime_type != nullptr) ? oformat->mime_type : "", NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getIFormatExtensions(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (iformat->extensions != nullptr) ? iformat->extensions : "", NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatExtensions(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (oformat->extensions != nullptr) ? oformat->extensions : "", NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_status getIOFormatFlags(napi_env env, int flags, napi_value* result, bool isInput) {
  napi_status status;
  napi_value value;

  status = napi_create_object(env, &value);
  PASS_STATUS;
  status = beam_set_bool(env, value, "NOFILE", flags & AVFMT_NOFILE); // O I
  PASS_STATUS;
  status = beam_set_bool(env, value, "NEEDNUMBER", flags & AVFMT_NEEDNUMBER); // O I
  PASS_STATUS;
  if (isInput) {
    status = beam_set_bool(env, value, "SHOW_IDS", flags & AVFMT_SHOW_IDS); // I
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "GLOBALHEADER", flags & AVFMT_GLOBALHEADER); // O
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "NOTIMESTAMPS", flags & AVFMT_NOTIMESTAMPS); // O
    PASS_STATUS;
  }
  if (isInput) {
    status = beam_set_bool(env, value, "GENERIC_INDEX", flags & AVFMT_GENERIC_INDEX); // I
    PASS_STATUS;
  }
  if (isInput) {
    status = beam_set_bool(env, value, "TS_DISCONT", flags & AVFMT_TS_DISCONT); // I
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "VARIABLE_FPS", flags & AVFMT_VARIABLE_FPS); // O
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "NODIMENSIONS", flags & AVFMT_NODIMENSIONS); // O
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "NOSTREAMS", flags & AVFMT_NOSTREAMS); // O
    PASS_STATUS;
  }
  if (isInput) {
    status = beam_set_bool(env, value, "NOBINSEARCH", flags & AVFMT_NOBINSEARCH); // I
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "NODIMENSIONS", flags & AVFMT_NODIMENSIONS); // O
    PASS_STATUS;
  }
  if (isInput) {
    status = beam_set_bool(env, value, "NOGENSEARCH", flags & AVFMT_NOGENSEARCH); // I
    PASS_STATUS;
  }
  if (isInput) {
    status = beam_set_bool(env, value, "NO_BYTE_SEEK", flags & AVFMT_NO_BYTE_SEEK); // I
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "ALLOW_FLUSH", flags & AVFMT_ALLOW_FLUSH); // O
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "TS_NONSTRICT", flags & AVFMT_TS_NONSTRICT); // O
    PASS_STATUS;
  }
  if (!isInput) {
    status = beam_set_bool(env, value, "TS_NEGATIVE", flags & AVFMT_TS_NEGATIVE); // O
    PASS_STATUS;
  }
  if (isInput) {
    status = beam_set_bool(env, value, "SEEK_TO_PTS", flags & AVFMT_SEEK_TO_PTS); // I
    PASS_STATUS;
  }

  *result = value;
  return napi_ok;
}

napi_value getOFormatFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = getIOFormatFlags(env, oformat->flags, &result, false);
  CHECK_STATUS;

  return result;
}

napi_value getIFormatFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = getIOFormatFlags(env, iformat->flags, &result, true);
  CHECK_STATUS;

  return result;
}

napi_value getIFormatRawCodecID(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = napi_create_int32(env, iformat->raw_codec_id, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatPrivDataSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_int32(env, oformat->priv_data_size, &result);
  CHECK_STATUS;

  return result;
}

napi_value getIFormatPrivDataSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = napi_create_int32(env, iformat->priv_data_size, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatPrivClass(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  if (oformat->priv_class != nullptr) {
    status = fromAVClass(env, (const AVClass*) oformat->priv_class, &result);
    CHECK_STATUS;
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_value getIFormatPrivClass(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  if (iformat->priv_class != nullptr) {
    status = fromAVClass(env, (const AVClass*) iformat->priv_class, &result);
    CHECK_STATUS;
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_value getOFormatAudioCodec(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (char*) avcodec_get_name(oformat->audio_codec),
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatVideoCodec(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (char*) avcodec_get_name(oformat->video_codec),
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getOFormatSubtitleCodec(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVOutputFormat* oformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &oformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (char*) avcodec_get_name(oformat->subtitle_codec),
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value muxers(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, muxer;
  void* opaque = nullptr;
  const AVOutputFormat* oformat = nullptr;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  oformat = av_muxer_iterate(&opaque);
  while ( oformat != nullptr ) {
    status = fromAVOutputFormat(env, oformat, &muxer);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, oformat->name, muxer);
    CHECK_STATUS;

    oformat = av_muxer_iterate(&opaque);
  }

  return result;
}

napi_value demuxers(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, demuxer;
  void* opaque = nullptr;
  const AVInputFormat* iformat = nullptr;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  iformat = av_demuxer_iterate(&opaque);
  while ( iformat != nullptr ) {
    status = fromAVInputFormat(env, iformat, &demuxer);
    CHECK_STATUS;
    status = napi_set_named_property(env, result, iformat->name, demuxer);
    CHECK_STATUS;

    iformat = av_demuxer_iterate(&opaque);
  }

  return result;
}

napi_value guessFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  char* testName;
  size_t strLen;
  AVOutputFormat* oformat;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Unable to guess an output format without a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Cannot guess an output format without a string value.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  testName = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], testName, strLen + 1, &strLen);
  CHECK_STATUS;

  oformat = av_guess_format((const char*) testName, nullptr, nullptr);
  if (oformat == nullptr) {
    oformat = av_guess_format(nullptr, (const char*) testName, nullptr);
  }
  if (oformat == nullptr) {
    oformat = av_guess_format(nullptr, nullptr, (const char*) testName);
  }
  if (oformat == nullptr) {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  } else {
    status = fromAVOutputFormat(env, oformat, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_status fromAVOutputFormat(napi_env env,
    const AVOutputFormat* oformat, napi_value* result) {
  napi_status status;
  napi_value jsOFormat, extOFormat, typeName;

  status = napi_create_object(env, &jsOFormat);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "OutputFormat", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  status = napi_create_external(env, (void*) oformat, nullptr, nullptr, &extOFormat);
  PASS_STATUS;

  // TODO - codec_tag is a bit hard
  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "name", nullptr, nullptr, getOFormatName, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "long_name", nullptr, nullptr, getOFormatLongName, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "mime_type", nullptr, nullptr, getOFormatMimeType, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "extensions", nullptr, nullptr, getOFormatExtensions, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "flags", nullptr, nullptr, getOFormatFlags, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "priv_data_size", nullptr, nullptr, getOFormatPrivDataSize, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "priv_class", nullptr, nullptr, getOFormatPrivClass, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "audio_codec", nullptr, nullptr, getOFormatAudioCodec, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "video_codec", nullptr, nullptr, getOFormatVideoCodec, nullptr,
      nullptr, napi_enumerable, (void*) oformat }, // 10
    { "subtitle_codec", nullptr, nullptr, getOFormatSubtitleCodec, nullptr,
      nullptr, napi_enumerable, (void*) oformat },
    { "_oformat", nullptr, nullptr, nullptr, nullptr, extOFormat, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsOFormat, 12, desc);
  PASS_STATUS;

  *result = jsOFormat;
  return napi_ok;
}

napi_status fromAVInputFormat(napi_env env,
    const AVInputFormat* iformat, napi_value* result) {
  napi_status status;
  napi_value jsIFormat, extIFormat, typeName;

  status = napi_create_object(env, &jsIFormat);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "InputFormat", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  status = napi_create_external(env, (void*) iformat, nullptr, nullptr, &extIFormat);
  PASS_STATUS;

  // TODO - codec_tag is a bit hard
  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "name", nullptr, nullptr, getIFormatName, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "long_name", nullptr, nullptr, getIFormatLongName, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "mime_type", nullptr, nullptr, getIFormatMimeType, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "extensions", nullptr, nullptr, getIFormatExtensions, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "flags", nullptr, nullptr, getIFormatFlags, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "raw_codec_id", nullptr, nullptr, getIFormatRawCodecID, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "priv_data_size", nullptr, nullptr, getIFormatPrivDataSize, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "priv_class", nullptr, nullptr, getOFormatPrivClass, nullptr,
      nullptr, napi_enumerable, (void*) iformat },
    { "_iformat", nullptr, nullptr, nullptr, nullptr, extIFormat, napi_default, nullptr } // 10
  };
  status = napi_define_properties(env, jsIFormat, 10, desc);
  PASS_STATUS;

  *result = jsIFormat;
  return napi_ok;
}

napi_value getFmtCtxOFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  if (fmtCtx->oformat != nullptr) {
    status = fromAVOutputFormat(env, fmtCtx->oformat, &result);
    CHECK_STATUS;
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_value setFmtCtxOFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, prop;
  napi_valuetype type;
  bool isArray;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Setting a muxer output format requires a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Setting a muxer output requires an object representing an output format.");
  }

  status = napi_get_named_property(env, args[0], "_oformat", &prop);
  CHECK_STATUS;
  status = napi_typeof(env, prop, &type);
  CHECK_STATUS;
  if (type != napi_external) {
    NAPI_THROW_ERROR("Object provided to set muxer output does not embed an AVOutputFormat.");
  }
  status = napi_get_value_external(env, prop, (void**) &fmtCtx->oformat);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxIFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  if (fmtCtx->iformat != nullptr) {
    status = fromAVInputFormat(env, fmtCtx->iformat, &result);
    CHECK_STATUS;
  } else {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_value getFmtCtxCtxFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "NOHEADER", fmtCtx->ctx_flags & AVFMTCTX_NOHEADER);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "UNSEEKABLE", fmtCtx->ctx_flags & AVFMTCTX_UNSEEKABLE);
  CHECK_STATUS;

  return result;
}

napi_value getFmtCtxStreams(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;

  for ( uint32_t x = 0 ; x < fmtCtx->nb_streams ; x++ ) {
    if (fmtCtx->streams[x] == nullptr) continue;
    status = fromAVStream(env, fmtCtx->streams[x], &element);
    CHECK_STATUS;
    status = napi_set_element(env, result, x, element);
    CHECK_STATUS;
  }

  return result;
}

napi_value setFmtCtxStreams(napi_env env, napi_callback_info info) {
  NAPI_THROW_ERROR("Streams can only be created with newStream() and then set via reference.");
}

napi_value getFmtCtxURL(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (fmtCtx->url != nullptr) ? fmtCtx->url : "", NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxURL(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;
  char* url;
  size_t strLen;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Muxer URL must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("Muxer URL must be set with a string value.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  url = (char*) av_malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], url, strLen + 1, &strLen);
  CHECK_STATUS;

  fmtCtx->url = url;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxStartTime(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_int64(env, fmtCtx->start_time, &result);
  CHECK_STATUS;

  return result;
}

napi_value getFmtCtxDuration(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_int64(env, fmtCtx->duration, &result);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxDuration(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Muxer duration must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Muxer duration must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &fmtCtx->duration);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxBitRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_int64(env, fmtCtx->bit_rate, &result);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxBitRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Format context bit_rate must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Format context bit_rate must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &fmtCtx->bit_rate);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxPacketSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_uint32(env, fmtCtx->packet_size, &result);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxPacketSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Format context packet_size must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Format context packet_size must be set with a number.");
  }
  status = napi_get_value_uint32(env, args[0], &fmtCtx->packet_size);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxMaxDelay(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_int32(env, fmtCtx->max_delay, &result);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxMaxDelay(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Format context max_delay must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Format context max_delay must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &fmtCtx->max_delay);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  // Generate missing pts even if it requires parsing future frames.
  status = beam_set_bool(env, result, "GENPTS", fmtCtx->flags & AVFMT_FLAG_GENPTS);
  CHECK_STATUS;
  // Ignore index.
  status = beam_set_bool(env, result, "IGNIDX", fmtCtx->flags & AVFMT_FLAG_IGNIDX);
  CHECK_STATUS;
  // Do not block when reading packets from input.
  status = beam_set_bool(env, result, "NONBLOCK", fmtCtx->flags & AVFMT_FLAG_NONBLOCK);
  CHECK_STATUS;
  // Ignore DTS on frames that contain both DTS & PTS
  status = beam_set_bool(env, result, "IGNDTS", fmtCtx->flags & AVFMT_FLAG_IGNDTS);
  CHECK_STATUS;
  // Do not infer any values from other values, just return what is stored in the container
  status = beam_set_bool(env, result, "NOFILLIN", fmtCtx->flags & AVFMT_FLAG_NOFILLIN);
  CHECK_STATUS;
  // Do not use AVParsers, you also must set AVFMT_FLAG_NOFILLIN as the fillin code works on frames and no parsing -> no frames. Also seeking to frames can not work if parsing to find frame boundaries has been disabled
  status = beam_set_bool(env, result, "NOPARSE", fmtCtx->flags & AVFMT_FLAG_NOPARSE);
  CHECK_STATUS;
  // Do not buffer frames when possible
  status = beam_set_bool(env, result, "NOBUFFER", fmtCtx->flags & AVFMT_FLAG_NOBUFFER);
  CHECK_STATUS;
  // The caller has supplied a custom AVIOContext, don't avio_close() it.
  status = beam_set_bool(env, result, "CUSTOM_IO", fmtCtx->flags & AVFMT_FLAG_CUSTOM_IO);
  CHECK_STATUS;
  // Discard frames marked corrupted
  status = beam_set_bool(env, result, "DISCARD_CORRUPT", fmtCtx->flags & AVFMT_FLAG_DISCARD_CORRUPT);
  CHECK_STATUS;
  // Flush the AVIOContext every packet.
  status = beam_set_bool(env, result, "FLUSH_PACKETS", fmtCtx->flags & AVFMT_FLAG_FLUSH_PACKETS);
  CHECK_STATUS;
  // For testing
  status = beam_set_bool(env, result, "BITEXACT", fmtCtx->flags & AVFMT_FLAG_BITEXACT);
  CHECK_STATUS;
  // try to interleave outputted packets by dts (using this flag can slow demuxing down)
  status = beam_set_bool(env, result, "SORT_DTS", fmtCtx->flags & AVFMT_FLAG_SORT_DTS);
  CHECK_STATUS;
  // Enable use of private options by delaying codec open (this could be made default once all code is converted)
  status = beam_set_bool(env, result, "PRIV_OPT", fmtCtx->flags & AVFMT_FLAG_PRIV_OPT);
  CHECK_STATUS;
  // Enable fast, but inaccurate seeks for some formats
  status = beam_set_bool(env, result, "FAST_SEEK", fmtCtx->flags & AVFMT_FLAG_FAST_SEEK);
  CHECK_STATUS;
  // Stop muxing when the shortest stream stops.
  status = beam_set_bool(env, result, "SHORTEST", fmtCtx->flags & AVFMT_FLAG_SHORTEST);
  CHECK_STATUS;
  // Add bitstream filters as requested by the muxer
  status = beam_set_bool(env, result, "AUTO_BSF", fmtCtx->flags & AVFMT_FLAG_AUTO_BSF);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;
  bool present, flag;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Format context flags must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_object) {
    NAPI_THROW_ERROR("Format context flags must be set with an object of Boolean values.");
  }
  // Generate missing pts even if it requires parsing future frames.
  status = beam_get_bool(env, args[0], "GENPTS", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_GENPTS :
    fmtCtx->flags & ~AVFMT_FLAG_GENPTS; }
  // Ignore index.
  status = beam_get_bool(env, args[0], "IGNIDX", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_IGNIDX :
    fmtCtx->flags & ~AVFMT_FLAG_IGNIDX; }
  // Do not block when reading packets from input.
  status = beam_get_bool(env, args[0], "NONBLOCK", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_NONBLOCK :
    fmtCtx->flags & ~AVFMT_FLAG_NONBLOCK; }
  // Ignore DTS on frames that contain both DTS & PTS
  status = beam_get_bool(env, args[0], "IGNDTS", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_IGNDTS :
    fmtCtx->flags & ~AVFMT_FLAG_IGNDTS; }
  // Do not infer any values from other values, just return what is stored in the container
  status = beam_get_bool(env, args[0], "NOFILLIN", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_NOFILLIN :
    fmtCtx->flags & ~AVFMT_FLAG_NOFILLIN; }
  // Do not use AVParsers, you also must set AVFMT_FLAG_NOFILLIN as the fillin code works on frames and no parsing -> no frames. Also seeking to frames can not work if parsing to find frame boundaries has been disabled
  status = beam_get_bool(env, args[0], "NOPARSE", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_NOPARSE :
    fmtCtx->flags & ~AVFMT_FLAG_NOPARSE; }
  // Do not buffer frames when possible
  status = beam_get_bool(env, args[0], "NOBUFFER", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_NOBUFFER :
    fmtCtx->flags & ~AVFMT_FLAG_NOBUFFER; }
  // The caller has supplied a custom AVIOContext, don't avio_close() it.
  status = beam_get_bool(env, args[0], "CUSTOM_IO", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_CUSTOM_IO :
    fmtCtx->flags & ~AVFMT_FLAG_CUSTOM_IO; }
  // Discard frames marked corrupted
  status = beam_get_bool(env, args[0], "DISCARD_CORRUPT", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_DISCARD_CORRUPT :
    fmtCtx->flags & ~AVFMT_FLAG_DISCARD_CORRUPT; }
  // Flush the AVIOContext every packet.
  status = beam_get_bool(env, args[0], "FLUSH_PACKETS", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_FLUSH_PACKETS :
    fmtCtx->flags & ~AVFMT_FLAG_FLUSH_PACKETS; }
  // Testing only
  status = beam_get_bool(env, args[0], "BITEXACT", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_BITEXACT :
    fmtCtx->flags & ~AVFMT_FLAG_BITEXACT; }
  // try to interleave outputted packets by dts (using this flag can slow demuxing down)
  status = beam_get_bool(env, args[0], "SORT_DTS", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_SORT_DTS :
    fmtCtx->flags & ~AVFMT_FLAG_SORT_DTS; }
  // Enable use of private options by delaying codec open (this could be made default once all code is converted)
  status = beam_get_bool(env, args[0], "PRIV_OPT", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_PRIV_OPT :
    fmtCtx->flags & ~AVFMT_FLAG_PRIV_OPT; }
  // Enable fast, but inaccurate seeks for some formats
  status = beam_get_bool(env, args[0], "FAST_SEEK", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_FAST_SEEK :
    fmtCtx->flags & ~AVFMT_FLAG_FAST_SEEK; }
  // Stop muxing when the shortest stream stops.
  status = beam_get_bool(env, args[0], "SHORTEST", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_SHORTEST :
    fmtCtx->flags & ~AVFMT_FLAG_SHORTEST; }
  // Add bitstream filters as requested by the muxer
  status = beam_get_bool(env, args[0], "AUTO_BSF", &present, &flag);
  CHECK_STATUS;
  if (present) { fmtCtx->flags = (flag) ?
    fmtCtx->flags | AVFMT_FLAG_AUTO_BSF :
    fmtCtx->flags & ~AVFMT_FLAG_AUTO_BSF; }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxProbeSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_int64(env, fmtCtx->probesize, &result);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxProbeSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Demuxer probesize must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Demuxer probesize must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &fmtCtx->probesize);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxMaxAnDur(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_int64(env, fmtCtx->max_analyze_duration, &result);
  CHECK_STATUS;

  return result;
}

napi_value setFmtCtxMaxAnDur(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Demuxer max_analyze_duration must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Demuxer max_analyze_duration must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &fmtCtx->max_analyze_duration);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFmtCtxMetadata(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;
  AVDictionaryEntry* tag = nullptr;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  while ((tag = av_dict_get(fmtCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
    status = beam_set_string_utf8(env, result, tag->key, tag->value);
    CHECK_STATUS;
  }

  return result;
}

napi_value setFmtCtxMetadata(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, names, key, value, valueS;
  napi_valuetype type;
  AVFormatContext* fmtCtx;
  bool isArray;
  uint32_t propCount;
  AVDictionary* dict = nullptr;
  char* keyStr, *valueStr;
  size_t strLen;
  int ret;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the format context's metadata property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Format context metadata can only be set with an object of tag names and values.");
  }

  status = napi_get_property_names(env, args[0], &names);
  CHECK_STATUS;
  status = napi_get_array_length(env, names, &propCount);
  CHECK_STATUS;

  // Replace all metadata values ... no partial operation
  for ( uint32_t x = 0 ; x < propCount ; x++ ) {
    status = napi_get_element(env, names, x, &key);
    CHECK_STATUS;
    status = napi_get_property(env, args[0], key, &value);
    CHECK_STATUS;
    status = napi_coerce_to_string(env, value, &valueS);
    CHECK_STATUS;

    status = napi_get_value_string_utf8(env, key, nullptr, 0, &strLen);
    CHECK_STATUS;
    keyStr = (char*) malloc(sizeof(char) * (strLen + 1));
    status = napi_get_value_string_utf8(env, key, keyStr, strLen + 1, &strLen);
    CHECK_STATUS;

    status = napi_get_value_string_utf8(env, valueS, nullptr, 0, &strLen);
    CHECK_STATUS;
    valueStr = (char*) malloc(sizeof(char) * (strLen + 1));
    status = napi_get_value_string_utf8(env, valueS, valueStr, strLen + 1, &strLen);
    CHECK_STATUS;

    ret = av_dict_set(&dict, keyStr, valueStr, 0);
    free(keyStr);
    free(valueStr);
    if (ret < 0) {
      NAPI_THROW_ERROR(avErrorMsg("Failed to set a dictionary entry: ", ret));
    }
  }
  fmtCtx->metadata = dict;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

// TODO use Javascript Date?
napi_value getFmtCtxStartTRealT(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFormatContext* fmtCtx;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  if (fmtCtx->start_time_realtime == AV_NOPTS_VALUE) {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  } else {
    status = napi_create_int64(env, fmtCtx->start_time_realtime, &result);
    CHECK_STATUS;
  }

  return result;
}

napi_value setFmtCtxStartTRealT(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVFormatContext* fmtCtx;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Format context start_time_realtime must be set with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;

  if ((type == napi_undefined) || (type == napi_null)) {
    fmtCtx->start_time_realtime = AV_NOPTS_VALUE;
    goto done;
  }

  if (type != napi_number) {
    NAPI_THROW_ERROR("Format context start_time_realtime must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &fmtCtx->start_time_realtime);
  CHECK_STATUS;

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_status fromAVFormatContext(napi_env env, AVFormatContext* fmtCtx,
    napi_value* result, bool isMuxer) {
  napi_status status;
  napi_value jsFmtCtx, extFmtCtx, typeName, truth;

  status = napi_create_object(env, &jsFmtCtx);
  PASS_STATUS;
  status = napi_create_string_utf8(env, isMuxer ? "muxer" : "demuxer",
    NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  status = napi_get_boolean(env, true, &truth);
  PASS_STATUS;
  status = napi_create_external(env, fmtCtx, formatContextFinalizer, nullptr, &extFmtCtx);
  PASS_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { isMuxer ? "oformat" : "iformat", nullptr, nullptr,
      isMuxer ? getFmtCtxOFormat : getFmtCtxIFormat,
      isMuxer ? setFmtCtxOFormat : nullptr, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "ctx_flags", nullptr, nullptr, getFmtCtxCtxFlags, nullptr, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "streams", nullptr, nullptr, getFmtCtxStreams, setFmtCtxStreams, nullptr,
       (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "url", nullptr, nullptr, getFmtCtxURL,
      isMuxer ? setFmtCtxURL : nullptr, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "start_time", nullptr, nullptr,
      isMuxer ? nullptr : getFmtCtxStartTime, nullptr, nullptr,
      isMuxer ? napi_default : napi_enumerable, fmtCtx },
    { "duration", nullptr, nullptr, getFmtCtxDuration,
      isMuxer ? setFmtCtxDuration : nullptr, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "bit_rate", nullptr, nullptr, getFmtCtxBitRate, setFmtCtxBitRate, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "packet_size", nullptr, nullptr, getFmtCtxPacketSize, setFmtCtxPacketSize, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "max_delay", nullptr, nullptr, getFmtCtxMaxDelay, setFmtCtxMaxDelay, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },  // 10
    { "flags", nullptr, nullptr, getFmtCtxFlags, setFmtCtxFlags, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "probesize", nullptr, nullptr,
      isMuxer ? nullptr : getFmtCtxProbeSize,
      isMuxer ? nullptr : setFmtCtxProbeSize, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "max_analyze_duration", nullptr, nullptr,
      isMuxer ? nullptr : getFmtCtxMaxAnDur,
      isMuxer ? nullptr : setFmtCtxMaxAnDur, nullptr,
      isMuxer ? napi_default : (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
      // key?
      // programs?
      // max_index_size
      // max_picture_buffer
      // chapters?
      // fps_probe_size
    { "metadata", nullptr, nullptr, getFmtCtxMetadata, setFmtCtxMetadata, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "start_time_realtime", nullptr, nullptr, getFmtCtxStartTRealT, setFmtCtxStartTRealT, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
      // error_recognition
      // debug
      // max_interleave_delta
      // strict_std_compliance
      // event_flags
      // max_ts_probe
      // avoid_negative_ts
      // audio_preload
      // max_chunk_duration
      // max_chunk_size
      // use_wallclock_as_timestamps
      // avio_flags
      // duration_estimation_method
      // skip_initial_bytes
      // correct_ts_overflow
      // seek2any
      // flush_packets
      // probe_score
      // format_probesize
      // codec_whitelist
      // format_whitelist
      // metaadata_header_padding
      // output_ts_offset
      // dump_separator
      // protocol_whitelist
      // protocol_blacklist
      // max_streams
      // skip_estimate_duration_from_pts
    { "interleaved", nullptr, nullptr, nullptr, nullptr, truth,
      (napi_property_attributes) (napi_writable | napi_enumerable), nullptr }, // format is interleaved?
    { "newStream", nullptr, newStream, nullptr, nullptr, nullptr,
      napi_enumerable, fmtCtx },
    { "_formatContext", nullptr, nullptr, nullptr, nullptr, extFmtCtx, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsFmtCtx, 18, desc);
  PASS_STATUS;

  *result = jsFmtCtx;
  return napi_ok;
}

void formatContextFinalizer(napi_env env, void* data, void* hint) {
  AVFormatContext* fc = (AVFormatContext*) data;
  int ret;
  if (fc->pb != nullptr) {
    ret = avio_closep(&fc->pb);
    if (ret < 0) {
      printf("DEBUG: For url '%s', %s", (fc->url != nullptr) ? fc->url : "unknown",
        avErrorMsg("error closing IO: ", ret));
    }
  }
  avformat_free_context(fc);
}

napi_value newStream(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, name, assign, global, jsObject;
  napi_valuetype type;
  AVFormatContext* fmtCtx;
  AVStream* stream;
  char* codecName = nullptr;
  size_t strLen;
  AVCodec* codec = nullptr;
  const AVCodecDescriptor* codecDesc = nullptr;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &fmtCtx);
  CHECK_STATUS;

  if (argc >= 1) {
    status = napi_typeof(env, args[0], &type);
    CHECK_STATUS;
    if ((type != napi_string) && (type != napi_object)) {
      NAPI_THROW_ERROR("New stream for a format context requires a string value to specify a codec, where provided.");
    }
    if (type == napi_object) {
      status = napi_get_named_property(env, args[0], "name", &name);
      CHECK_STATUS;
    } else {
      name = args[0];
    }
    status = napi_get_value_string_utf8(env, name, nullptr, 0, &strLen);
    CHECK_STATUS;
    codecName = (char*) malloc(sizeof(char) * (strLen + 1));
    status = napi_get_value_string_utf8(env, name, codecName, strLen + 1, &strLen);
    CHECK_STATUS;

    if (fmtCtx->oformat) { // Look for encoders
      codec = avcodec_find_encoder_by_name(codecName);
      if (codec == nullptr) {
        codecDesc = avcodec_descriptor_get_by_name(codecName);
        if (codecDesc != nullptr) {
          codec = avcodec_find_encoder(codecDesc->id);
        }
      }
    }
    if (codec == nullptr) {
      codec = avcodec_find_decoder_by_name(codecName);
      if (codec == nullptr) {
        codecDesc = avcodec_descriptor_get_by_name(codecName);
        if (codecDesc != nullptr) {
          codec = avcodec_find_decoder(codecDesc->id);
        }
      }
    }
    printf("From name %s, selected AVCodec %s\n", codecName,
      (codec != nullptr) ? codec->name : "");
    free(codecName);
  }
  stream = avformat_new_stream(fmtCtx, codec);

  if (stream == nullptr) {
    NAPI_THROW_ERROR("Unable to create a stream for this format context.");
  }

  if (codec != nullptr) {
    stream->codecpar->codec_type = codec->type;
    stream->codecpar->codec_id = codec->id;
    // TODO set codec_tag here
  }
  status = fromAVStream(env, stream, &result);
  CHECK_STATUS;

  if ((argc >=1) && (type == napi_object)) {
    status = napi_get_global(env, &global);
    CHECK_STATUS;
    status = napi_get_named_property(env, global, "Object", &jsObject);
    CHECK_STATUS;
    status = napi_get_named_property(env, jsObject, "assign", &assign);
    CHECK_STATUS;
    const napi_value fargs[] = { result, args[0] };
    status = napi_call_function(env, result, assign, 2, fargs, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_value getStreamIndex(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_int32(env, stream->index, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamID(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_int32(env, stream->id, &result);
  CHECK_STATUS;
  return result;
}

napi_value setStreamID(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the stream id property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("The stream's id property must be set with a number.");
  }

  status = napi_get_value_int32(env, args[0], &stream->id);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamTimeBase(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->time_base.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->time_base.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setStreamTimeBase(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the stream time_base property.");
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("The stream's time_base property must be set with an array of two numbers.");
  }
  for ( uint32_t x = 0 ; x < 2 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type != napi_number) {
      NAPI_THROW_ERROR("The stream's time_base property array elements must be numbers.");
    }
  }
  status = napi_get_element(env, args[0], 0, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->time_base.num);
  CHECK_STATUS;
  status = napi_get_element(env, args[0], 1, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->time_base.den);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamStartTime(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  if (stream->start_time == AV_NOPTS_VALUE) {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  } else {
    status = napi_create_int64(env, stream->start_time, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_value setStreamStartTime(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set a stream's start_time property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_null) || (type == napi_undefined)) {
    stream->start_time = AV_NOPTS_VALUE;
    goto done;
  }
  if (type != napi_number) {
    NAPI_THROW_ERROR("The stream's start_time property must be set with a number.");
  }

  status = napi_get_value_int64(env, args[0], &stream->start_time);
  CHECK_STATUS;

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamDuration(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  if (stream->duration == AV_NOPTS_VALUE) {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  } else {
    status = napi_create_int64(env, stream->duration, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_value setStreamDuration(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set a stream's duration property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_null) || (type == napi_undefined)) {
    stream->start_time = AV_NOPTS_VALUE;
    goto done;
  }
  if (type != napi_number) {
    NAPI_THROW_ERROR("The stream's duration property must be set with a number.");
  }

  status = napi_get_value_int64(env, args[0], &stream->duration);
  CHECK_STATUS;

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamNbFrames(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_int64(env, stream->nb_frames, &result);
  CHECK_STATUS;
  return result;
}

napi_value setStreamNbFrames(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set a stream's nb_frames property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_null) || (type == napi_undefined)) {
    stream->start_time = 0;
    goto done;
  }
  if (type != napi_number) {
    NAPI_THROW_ERROR("The stream's nb_frames property must be set with a number.");
  }

  status = napi_get_value_int64(env, args[0], &stream->nb_frames);
  CHECK_STATUS;

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamDisposition(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  status = beam_set_bool(env, result, "DEFAULT", stream->disposition & AV_DISPOSITION_DEFAULT);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "DUB", stream->disposition & AV_DISPOSITION_DUB);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "ORIGINAL", stream->disposition & AV_DISPOSITION_ORIGINAL);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "COMMENT", stream->disposition & AV_DISPOSITION_COMMENT);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "LYRICS", stream->disposition & AV_DISPOSITION_LYRICS);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "KARAOKE", stream->disposition & AV_DISPOSITION_KARAOKE);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "FORCED", stream->disposition & AV_DISPOSITION_FORCED);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "HEARING_IMPAIRED", stream->disposition & AV_DISPOSITION_HEARING_IMPAIRED);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "VISUAL_IMPAIRED", stream->disposition & AV_DISPOSITION_VISUAL_IMPAIRED);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "CLEAN_EFFECTS", stream->disposition & AV_DISPOSITION_CLEAN_EFFECTS);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "ATTACHED_PIC", stream->disposition & AV_DISPOSITION_ATTACHED_PIC);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "TIMED_THUMBNAILS", stream->disposition & AV_DISPOSITION_TIMED_THUMBNAILS);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "CAPTIONS", stream->disposition & AV_DISPOSITION_CAPTIONS);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "DESCRIPTIONS", stream->disposition & AV_DISPOSITION_DESCRIPTIONS);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "METADATA", stream->disposition & AV_DISPOSITION_METADATA);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "DEPENDENT", stream->disposition & AV_DISPOSITION_DEPENDENT);
  CHECK_STATUS;
  status = beam_set_bool(env, result, "STILL_IMAGE", stream->disposition & AV_DISPOSITION_STILL_IMAGE);
  CHECK_STATUS;

  return result;
}

napi_value setStreamDisposition(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVStream* stream;
  bool isArray, present, flag;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set a stream's disposition flags.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("The stream's disposition flags must be set with an object with Boolean-valued properties.");
  }

  status = beam_get_bool(env, args[0], "DEFAULT", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_DEFAULT :
    stream->disposition & ~AV_DISPOSITION_DEFAULT; }
  status = beam_get_bool(env, args[0], "DUB", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_DUB :
    stream->disposition & ~AV_DISPOSITION_DUB; }
  status = beam_get_bool(env, args[0], "ORIGINAL", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_ORIGINAL :
    stream->disposition & ~AV_DISPOSITION_ORIGINAL; }
  status = beam_get_bool(env, args[0], "COMMENT", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_COMMENT :
    stream->disposition & ~AV_DISPOSITION_COMMENT; }
  status = beam_get_bool(env, args[0], "LYRICS", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_LYRICS :
    stream->disposition & ~AV_DISPOSITION_LYRICS; }
  status = beam_get_bool(env, args[0], "KARAOKE", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_KARAOKE :
    stream->disposition & ~AV_DISPOSITION_KARAOKE; }
  status = beam_get_bool(env, args[0], "FORCED", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_FORCED :
    stream->disposition & ~AV_DISPOSITION_FORCED; }
  status = beam_get_bool(env, args[0], "HEARING_IMPAIRED", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_HEARING_IMPAIRED :
    stream->disposition & ~AV_DISPOSITION_HEARING_IMPAIRED; }
  status = beam_get_bool(env, args[0], "VISUAL_IMPAIRED", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_VISUAL_IMPAIRED :
    stream->disposition & ~AV_DISPOSITION_VISUAL_IMPAIRED; }
  status = beam_get_bool(env, args[0], "CLEAN_EFFECTS", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_CLEAN_EFFECTS :
    stream->disposition & ~AV_DISPOSITION_CLEAN_EFFECTS; }
  status = beam_get_bool(env, args[0], "ATTACHED_PIC", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_ATTACHED_PIC :
    stream->disposition & ~AV_DISPOSITION_ATTACHED_PIC; }
  status = beam_get_bool(env, args[0], "TIMED_THUMBNAILS", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_TIMED_THUMBNAILS :
    stream->disposition & ~AV_DISPOSITION_TIMED_THUMBNAILS; }
  status = beam_get_bool(env, args[0], "CAPTIONS", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_CAPTIONS :
    stream->disposition & ~AV_DISPOSITION_CAPTIONS; }
  status = beam_get_bool(env, args[0], "DESCRIPTIONS", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_DESCRIPTIONS :
    stream->disposition & ~AV_DISPOSITION_DESCRIPTIONS; }
  status = beam_get_bool(env, args[0], "METADATA", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_METADATA :
    stream->disposition & ~AV_DISPOSITION_METADATA; }
  status = beam_get_bool(env, args[0], "DEPENDENT", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_DEPENDENT :
    stream->disposition & ~AV_DISPOSITION_DEPENDENT; }
  status = beam_get_bool(env, args[0], "STILL_IMAGE", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->disposition = (flag) ?
    stream->disposition | AV_DISPOSITION_STILL_IMAGE :
    stream->disposition & ~AV_DISPOSITION_STILL_IMAGE; }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamDiscard(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    beam_lookup_name(beam_avdiscard->forward, stream->discard),
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;
  return result;
}

napi_value setStreamDiscard(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVStream* stream;
  char* enumName;
  size_t strLen;
  int enumValue;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set a stream's discard property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_string) {
    NAPI_THROW_ERROR("The stream's discard property must be set with a string.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen);
  CHECK_STATUS;
  enumName = (char*) malloc(sizeof(char) * (strLen + 1));
  status = napi_get_value_string_utf8(env, args[0], enumName, strLen + 1, &strLen);
  CHECK_STATUS;

  enumValue = beam_lookup_enum(beam_avdiscard->inverse, enumName);
  stream->discard = (enumValue != BEAM_ENUM_UNKNOWN) ? (AVDiscard) enumValue : AVDISCARD_NONE;
  free(enumName);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamSmpAspectRt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->sample_aspect_ratio.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->sample_aspect_ratio.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setStreamSmpAspectRt(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the stream sample_aspect_ratio property.");
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("The stream's sample_aspect_ratio property must be set with an array of two numbers.");
  }
  for ( uint32_t x = 0 ; x < 2 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type != napi_number) {
      NAPI_THROW_ERROR("The stream's sample_aspect_ratio property array elements must be numbers.");
    }
  }
  status = napi_get_element(env, args[0], 0, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->sample_aspect_ratio.num);
  CHECK_STATUS;
  status = napi_get_element(env, args[0], 1, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->sample_aspect_ratio.den);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamEventFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  status = beam_set_bool(env, result, "METADATA_UPDATED",
    stream->event_flags & AVSTREAM_EVENT_FLAG_METADATA_UPDATED);
  CHECK_STATUS;

  return result;
}

napi_value setStreamEventFlags(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  AVStream* stream;
  bool isArray, present, flag;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the stream event_flags property.")
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Stream property event_flags is set with an object of Boolean values.");
  }
  status = beam_get_bool(env, args[0], "METADATA_UPDATED", &present, &flag);
  CHECK_STATUS;
  if (present) { stream->event_flags = (flag) ?
    stream->event_flags | AVSTREAM_EVENT_FLAG_METADATA_UPDATED :
    stream->event_flags & ~AVSTREAM_EVENT_FLAG_METADATA_UPDATED; }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamMetadata(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;
  AVDictionaryEntry* tag = nullptr;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
    status = beam_set_string_utf8(env, result, tag->key, tag->value);
    CHECK_STATUS;
  }

  return result;
}

napi_value setStreamMetadata(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, names, key, value, valueS;
  napi_valuetype type;
  AVStream* stream;
  bool isArray;
  uint32_t propCount;
  AVDictionary* dict = nullptr;
  char* keyStr, *valueStr;
  size_t strLen;
  int ret;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the stream's metadata property.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Stream metadata can only be set with an object of tag names and values.");
  }

  status = napi_get_property_names(env, args[0], &names);
  CHECK_STATUS;
  status = napi_get_array_length(env, names, &propCount);
  CHECK_STATUS;

  // Replace all metadata values ... no partial operation
  for ( uint32_t x = 0 ; x < propCount ; x++ ) {
    status = napi_get_element(env, names, x, &key);
    CHECK_STATUS;
    status = napi_get_property(env, args[0], key, &value);
    CHECK_STATUS;
    status = napi_coerce_to_string(env, value, &valueS);
    CHECK_STATUS;

    status = napi_get_value_string_utf8(env, key, nullptr, 0, &strLen);
    CHECK_STATUS;
    keyStr = (char*) malloc(sizeof(char) * (strLen + 1));
    status = napi_get_value_string_utf8(env, key, keyStr, strLen + 1, &strLen);
    CHECK_STATUS;

    status = napi_get_value_string_utf8(env, valueS, nullptr, 0, &strLen);
    CHECK_STATUS;
    valueStr = (char*) malloc(sizeof(char) * (strLen + 1));
    status = napi_get_value_string_utf8(env, valueS, valueStr, strLen + 1, &strLen);
    CHECK_STATUS;

    ret = av_dict_set(&dict, keyStr, valueStr, 0);
    free(keyStr);
    free(valueStr);
    if (ret < 0) {
      NAPI_THROW_ERROR(avErrorMsg("Failed to set a dictionary entry: ", ret));
    }
  }
  stream->metadata = dict;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamAvgFrameRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->avg_frame_rate.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->avg_frame_rate.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setStreamAvgFrameRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the stream avg_frame_rate property.");
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("The stream's avg_frame_rate property must be set with an array of two numbers.");
  }
  for ( uint32_t x = 0 ; x < 2 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type != napi_number) {
      NAPI_THROW_ERROR("The stream's avg_frame_rate property array elements must be numbers.");
    }
  }
  status = napi_get_element(env, args[0], 0, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->avg_frame_rate.num);
  CHECK_STATUS;
  status = napi_get_element(env, args[0], 1, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->avg_frame_rate.den);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamRFrameRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVStream* stream;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->r_frame_rate.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, stream->r_frame_rate.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setStreamRFrameRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  bool isArray;
  AVStream* stream;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("A value is required to set the stream r_frame_rate property.");
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("The stream's r_frame_rate property must be set with an array of two numbers.");
  }
  for ( uint32_t x = 0 ; x < 2 ; x++ ) {
    status = napi_get_element(env, args[0], x, &element);
    CHECK_STATUS;
    status = napi_typeof(env, element, &type);
    CHECK_STATUS;
    if (type != napi_number) {
      NAPI_THROW_ERROR("The stream's r_frame_rate property array elements must be numbers.");
    }
  }
  status = napi_get_element(env, args[0], 0, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->r_frame_rate.num);
  CHECK_STATUS;
  status = napi_get_element(env, args[0], 1, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &stream->r_frame_rate.den);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamCodecPar(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;
  codecParData* cpd = new codecParData;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  cpd->codecPars = stream->codecpar;
  status = fromAVCodecParameters(env, cpd, &result);
  CHECK_STATUS;
  return result;
}

napi_value setStreamCodecPar(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, jsCodecPar, extCodecPar;
  napi_valuetype type;
  AVStream* stream;
  bool isArray;
  codecParData* cpd;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &stream);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Cannot set stream codec parameters without a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Setting stream codec parameters requires an object.");
  }
  status = napi_get_named_property(env, args[0], "_codecPar", &extCodecPar);
  CHECK_STATUS;
  status = napi_typeof(env, extCodecPar, &type);
  CHECK_STATUS;
  if (type != napi_external) {
    jsCodecPar = makeCodecParameters(env, info); // Try and make some
    status = napi_get_named_property(env, jsCodecPar, "_codecPar", &extCodecPar);
    CHECK_STATUS;
  }
  status = napi_get_value_external(env, extCodecPar, (void**) &cpd);
  CHECK_STATUS;
  stream->codecpar = cpd->codecPars;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getStreamAttachedPic(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVStream* stream;
  packetData* pd = new packetData;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &stream);
  CHECK_STATUS;

  if (stream->attached_pic.data == nullptr) {
    status = napi_get_null(env, &result);
    CHECK_STATUS;
  } else {
    pd->packet = &stream->attached_pic;
    status = fromAVPacket(env, pd, &result);
    CHECK_STATUS;
  }
  return result;
}

napi_status fromAVStream(napi_env env, AVStream* stream, napi_value* result) {
  napi_status status;
  napi_value jsStream, extStream, typeName;

  status = napi_create_object(env, &jsStream);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "Stream", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  // Note - streams are cleaned by avcodec_close() and avcodec_free_context()
  status = napi_create_external(env, stream, nullptr, nullptr, &extStream);
  PASS_STATUS;

  // Note - name is a dummy property used to avoid it being accidentally set
  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "index", nullptr, nullptr, getStreamIndex, nullptr, nullptr, napi_enumerable, stream },
    { "id", nullptr, nullptr, getStreamID, setStreamID, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "time_base", nullptr, nullptr, getStreamTimeBase, setStreamTimeBase, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "start_time", nullptr, nullptr, getStreamStartTime, setStreamStartTime, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "duration", nullptr, nullptr, getStreamDuration, setStreamDuration, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "nb_frames", nullptr, nullptr, getStreamNbFrames, setStreamNbFrames, nullptr,
       (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "disposition", nullptr, nullptr, getStreamDisposition, setStreamDisposition, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "discard", nullptr, nullptr, getStreamDiscard, setStreamDiscard, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "sample_aspect_ratio", nullptr, nullptr, getStreamSmpAspectRt, setStreamSmpAspectRt, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream }, // 10
    { "metadata", nullptr, nullptr, getStreamMetadata, setStreamMetadata, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "avg_frame_rate", nullptr, nullptr, getStreamAvgFrameRate, setStreamAvgFrameRate, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "attached_pic", nullptr, nullptr, getStreamAttachedPic, nullptr, nullptr,
       napi_enumerable, stream },
    // side data
    { "event_flags", nullptr, nullptr, getStreamEventFlags, setStreamEventFlags, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "r_frame_rate", nullptr, nullptr, getStreamRFrameRate, setStreamRFrameRate, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "codecpar", nullptr, nullptr, getStreamCodecPar, setStreamCodecPar, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), stream },
    { "name", nullptr, nullptr, nullptr, nullptr, typeName, napi_writable, nullptr },
    { "_stream", nullptr, nullptr, nullptr, nullptr, extStream, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsStream, 18, desc);
  PASS_STATUS;

  *result = jsStream;
  return napi_ok;
}
