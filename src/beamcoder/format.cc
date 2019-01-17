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

  status = napi_create_string_utf8(env,
    (oformat->priv_class != nullptr) ? oformat->priv_class->class_name : "",
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getIFormatPrivClass(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVInputFormat* iformat;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &iformat);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    (iformat->priv_class != nullptr) ? iformat->priv_class->class_name : "",
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

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

napi_status fromAVFormatContext(napi_env env, AVFormatContext* fmtCtx,
    napi_value* result, bool isMuxer) {
  napi_status status;
  napi_value jsFmtCtx, extFmtCtx, typeName;

  status = napi_create_object(env, &jsFmtCtx);
  PASS_STATUS;
  status = napi_create_string_utf8(env, isMuxer ? "muxer" : "demuxer",
    NAPI_AUTO_LENGTH, &typeName);
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
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx },
    { "flags", nullptr, nullptr, getFmtCtxFlags, setFmtCtxFlags, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), fmtCtx }, // 10
    { "newStream", nullptr, newStream, nullptr, nullptr, nullptr,
      napi_enumerable, fmtCtx },
    { "_formatContext", nullptr, nullptr, nullptr, nullptr, extFmtCtx, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsFmtCtx, 11, desc);
  PASS_STATUS;

  *result = jsFmtCtx;
  return napi_ok;
}

void formatContextFinalizer(napi_env env, void* data, void* hint) {
  AVFormatContext* fc = (AVFormatContext*) data;
  avformat_free_context(fc);
}

napi_value newStream(napi_env env, napi_callback_info info) {
  // stream = avformat_new_stream(fmtCtx, codec or nullptr);
  // status = fromAVStream(env, stream, &result);
  return nullptr;
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

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "_stream", nullptr, nullptr, nullptr, nullptr, extStream, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsStream, 2, desc);
  PASS_STATUS;

  *result = jsStream;
  return napi_ok;
}
