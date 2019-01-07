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

#include "node_api.h"
#include "beamcoder_util.h"
#include "governor.h"
#include "format.h"
#include "decode.h"
#include "encode.h"
#include <stdio.h>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavdevice/avdevice.h>
  #include <libavfilter/avfilter.h>
  #include <libavformat/avformat.h>
  #include <libavutil/avutil.h>
  #include <libpostproc/postprocess.h>
  #include <libswresample/swresample.h>
  #include <libswscale/swscale.h>
}

napi_value versions(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  status = napi_create_uint32(env, avcodec_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avcodec", value);
  CHECK_STATUS;
  status = napi_create_uint32(env, avdevice_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avdevice", value);
  CHECK_STATUS;
  status = napi_create_uint32(env, avfilter_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avfilter", value);
  CHECK_STATUS;
  status = napi_create_uint32(env, avformat_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avformat", value);
  CHECK_STATUS;
  status = napi_create_uint32(env, avutil_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avutil", value);
  CHECK_STATUS;
  status = napi_create_uint32(env, postproc_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "postproc", value);
  CHECK_STATUS;
  status = napi_create_uint32(env, swresample_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swresample", value);
  CHECK_STATUS;
  status = napi_create_uint32(env, swscale_version(), &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swscale", value);
  CHECK_STATUS;

  return result;
}

napi_value avVersionInfo(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;

  const char* verInfo = av_version_info();

  status = napi_create_string_utf8(env, verInfo, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value versionStrings(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;
  char vstr[16];

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBAVCODEC_VERSION_MAJOR, LIBAVCODEC_VERSION_MINOR,
    LIBAVCODEC_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avcodec", value);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBAVDEVICE_VERSION_MAJOR, LIBAVDEVICE_VERSION_MINOR,
    LIBAVDEVICE_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avdevice", value);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBAVFILTER_VERSION_MAJOR, LIBAVFILTER_VERSION_MINOR,
    LIBAVFILTER_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avfilter", value);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR,
    LIBAVFORMAT_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avformat", value);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR,
    LIBAVUTIL_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avutil", value);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBPOSTPROC_VERSION_MAJOR, LIBPOSTPROC_VERSION_MINOR,
    LIBPOSTPROC_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "postproc", value);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBSWRESAMPLE_VERSION_MAJOR, LIBSWRESAMPLE_VERSION_MINOR,
    LIBSWRESAMPLE_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swresample", value);
  CHECK_STATUS;

  sprintf(vstr, "%i.%i.%i", LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR,
    LIBSWSCALE_VERSION_MICRO);
  status = napi_create_string_utf8(env, vstr, NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swscale", value);
  CHECK_STATUS;

  return result;
}

napi_value configurations(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, avcodec_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avcodec", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avdevice_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avdevice", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avfilter_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avfilter", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avformat_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avformat", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avutil_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avutil", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, postproc_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "postproc", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, swresample_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swresample", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, swscale_configuration(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swscale", value);
  CHECK_STATUS;

  return result;
}

napi_value licenses(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, avcodec_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avcodec", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avdevice_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avdevice", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avfilter_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avfilter", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avformat_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avformat", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, avutil_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "avutil", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, postproc_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "postproc", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, swresample_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swresample", value);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, swscale_license(), NAPI_AUTO_LENGTH, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "swscale", value);
  CHECK_STATUS;

  return result;
}

napi_value testSetProps(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVCodec* codec;
  AVCodecContext* codecCtx;

  size_t argc = 1;
  napi_value args[1];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  if (argc != 1) NAPI_THROW_ERROR("Arguments object must be provided.");

  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  codecCtx = avcodec_alloc_context3(codec);
  status = setCodecFromProps(env, codecCtx, args[0], true);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  status = getPropsFromCodec(env, result, codecCtx, true);
  return result;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc[] = {
    DECLARE_NAPI_METHOD("versions", versions),
    DECLARE_NAPI_METHOD("avVersionInfo", avVersionInfo),
    DECLARE_NAPI_METHOD("versionStrings", versionStrings),
    DECLARE_NAPI_METHOD("configurations", configurations),
    DECLARE_NAPI_METHOD("licenses", licenses),
    DECLARE_NAPI_METHOD("governor", governor),
    DECLARE_NAPI_METHOD("format", format),
    DECLARE_NAPI_METHOD("decoder", decoder),
    DECLARE_NAPI_METHOD("encoder", encoder),
    DECLARE_NAPI_METHOD("testSetProps", testSetProps)
   };
  status = napi_define_properties(env, exports, 9, desc);
  CHECK_STATUS;
  return exports;
}

NAPI_MODULE(beamcoder, Init)
