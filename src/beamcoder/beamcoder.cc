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

#include "node_api.h"
#include "beamcoder_util.h"
#include "governor.h"
#include "format.h"
#include "decode.h"
#include "filter.h"
#include "encode.h"
#include "packet.h"
#include "codec_par.h"
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
  bool encoding = false;

  size_t argc = 2;
  napi_value args[2];
  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;

  if (argc < 1) NAPI_THROW_ERROR("Arguments object must be provided.");

  if (argc == 2) {
    status = napi_get_value_bool(env, args[1], &encoding);
    // If it fails, PA!
  }

  codec = avcodec_find_encoder_by_name("libx264");
  codecCtx = avcodec_alloc_context3(codec);

  // av_opt_set_int(codecCtx->priv_data, "motion-est", 1, 0);

  status = setCodecFromProps(env, codecCtx, args[0], encoding);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  status = getPropsFromCodec(env, result, codecCtx, encoding);
  return result;
}

napi_value codecs(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value, array, element;
  void* opaque = nullptr;
  const AVCodec* codec;
  const AVProfile* profile;
  const AVCodecDescriptor* codecDesc;
  int32_t index = 0;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  codec = av_codec_iterate(&opaque);
  while (codec != nullptr) {
    status = napi_create_object(env, &value);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, value, "type", "Codec");
    CHECK_STATUS;
    status = beam_set_string_utf8(env, value, "name", (char*) codec->name);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, value, "long_name", (char*) codec->long_name);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, value, "codec_type",
      (char*) av_get_media_type_string(codec->type));
    CHECK_STATUS;
    status = beam_set_int32(env, value, "id", (int32_t) codec->id);
    CHECK_STATUS;
    status = beam_set_bool(env, value, "decoder", av_codec_is_decoder(codec));
    CHECK_STATUS;
    status = beam_set_bool(env, value, "encoder", av_codec_is_encoder(codec));
    CHECK_STATUS;

    if (codec->profiles != nullptr) {
      status = napi_create_array(env, &array);
      CHECK_STATUS;
      profile = codec->profiles;
      index = 0;
      printf("Profiles for %s %s\n", codec->name, profile->name);
      while (profile->profile != FF_PROFILE_UNKNOWN) {
        status = napi_create_string_utf8(env, profile->name, NAPI_AUTO_LENGTH, &element);
        CHECK_STATUS;
        status = napi_set_element(env, array, index++, element);
        CHECK_STATUS;
        profile = profile + 1;
      }
      status = napi_set_named_property(env, value, "profiles", array);
      CHECK_STATUS;
    }

    status = beam_set_bool(env, value, "frame-level_multithreading",
      codec->capabilities & AV_CODEC_CAP_FRAME_THREADS);
    CHECK_STATUS;
    status = beam_set_bool(env, value, "slice-level_multithreading",
      codec->capabilities & AV_CODEC_CAP_SLICE_THREADS);
    CHECK_STATUS;
    status = beam_set_bool(env, value, "experimental",
      codec->capabilities & AV_CODEC_CAP_EXPERIMENTAL);
    CHECK_STATUS;
    status = beam_set_bool(env, value, "supports_draw-horiz-band",
      codec->capabilities & AV_CODEC_CAP_DRAW_HORIZ_BAND);
    CHECK_STATUS;
    status = beam_set_bool(env, value, "supports_direct_rendering_1",
      codec->capabilities & AV_CODEC_CAP_DR1);
    CHECK_STATUS;

    codecDesc = avcodec_descriptor_get(codec->id);
    if (codecDesc != nullptr) {
      status = beam_set_string_utf8(env, value, "descriptor_name", (char*) codecDesc->name);
      CHECK_STATUS;
      status = beam_set_bool(env, value, "intra-frame_only",
        codecDesc->props & AV_CODEC_PROP_INTRA_ONLY);
      CHECK_STATUS;
      status = beam_set_bool(env, value, "lossy",
        codecDesc->props & AV_CODEC_PROP_LOSSY);
      CHECK_STATUS;
      status = beam_set_bool(env, value, "lossless",
        codecDesc->props & AV_CODEC_PROP_LOSSLESS);
      CHECK_STATUS;
    }

    status = napi_set_named_property(env, result, codec->name, value);
    CHECK_STATUS;
    codec = av_codec_iterate(&opaque);
  }

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
    DECLARE_NAPI_METHOD("format", format), // deprecated - so young!
    DECLARE_NAPI_METHOD("decoder", decoder),
    DECLARE_NAPI_METHOD("filterer", filterer),
    DECLARE_NAPI_METHOD("encoder", encoder), // 10
    DECLARE_NAPI_METHOD("codecs", codecs),
    DECLARE_NAPI_METHOD("makePacket", makePacket),
    DECLARE_NAPI_METHOD("makeFrame", makeFrame),
    DECLARE_NAPI_METHOD("makeCodecParameters", makeCodecParameters),
    DECLARE_NAPI_METHOD("demuxer", format),
    DECLARE_NAPI_METHOD("muxer", format)
   };
  status = napi_define_properties(env, exports, 16, desc);
  CHECK_STATUS;

  // Iterate over all codecs to makes sure they are registered
  void* opaque = nullptr;
  const AVCodec* codec;
  do {
    codec = av_codec_iterate(&opaque);
    // printf("Registered '%s'\n", (codec != nullptr) ? codec->name : "(null)");
  } while (codec != nullptr);
  return exports;
}

NAPI_MODULE(beamcoder, Init)
