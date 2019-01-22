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
#include "demux.h"
#include "decode.h"
#include "filter.h"
#include "encode.h"
#include "mux.h"
#include "packet.h"
#include "codec_par.h"
#include <stdio.h>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavdevice/avdevice.h>
  #include <libavfilter/avfilter.h>
  #include <libavformat/avformat.h>
  #include <libavutil/avutil.h>
  #include <libavutil/pixdesc.h>
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

napi_value pix_fmts(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, pixfmt, flags, comps, comp;
  const AVPixFmtDescriptor* desc = nullptr;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  while ((desc = av_pix_fmt_desc_next(desc))) {
    status = napi_create_object(env, &pixfmt);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, pixfmt, "name", (char*) desc->name);
    CHECK_STATUS;
    status = beam_set_int32(env, pixfmt, "nb_components", desc->nb_components);
    CHECK_STATUS;
    status = beam_set_uint32(env, pixfmt, "log2_chroma_h", desc->log2_chroma_h);
    CHECK_STATUS;
    status = beam_set_uint32(env, pixfmt, "log2_chroma_w", desc->log2_chroma_w);
    CHECK_STATUS;
    status = napi_create_object(env, &flags);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "BE", desc->flags & AV_PIX_FMT_FLAG_BE);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "PAL", desc->flags & AV_PIX_FMT_FLAG_PAL);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "BITSTREAM", desc->flags & AV_PIX_FMT_FLAG_BITSTREAM);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "HWACCEL", desc->flags & AV_PIX_FMT_FLAG_HWACCEL);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "PLANAR", desc->flags & AV_PIX_FMT_FLAG_PLANAR);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "RGB", desc->flags & AV_PIX_FMT_FLAG_RGB);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "PSEUDOPAL", desc->flags & AV_PIX_FMT_FLAG_PSEUDOPAL);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "ALPHA", desc->flags & AV_PIX_FMT_FLAG_ALPHA);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "BAYER", desc->flags & AV_PIX_FMT_FLAG_BAYER);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "FLOAT", desc->flags & AV_PIX_FMT_FLAG_FLOAT);
    CHECK_STATUS;
    status = napi_set_named_property(env, pixfmt, "flags", flags);
    CHECK_STATUS;
    status = napi_create_array(env, &comps);
    CHECK_STATUS;
    for ( int x = 0 ; x < desc->nb_components ; x++ ) {
      status = napi_create_object(env, &comp);
      CHECK_STATUS;
      char letter[] = { 'X', '\0' };
      switch(x) {
        case 0:
          if (desc->nb_components < 3) { letter[0] = 'Y'; break; }
          letter[0] = (desc->flags & AV_PIX_FMT_FLAG_RGB) ? 'R' : 'Y';
          break;
        case 1:
          if (desc->nb_components < 3) { letter[0] = 'A'; break; }
          letter[0] = (desc->flags & AV_PIX_FMT_FLAG_RGB) ? 'G' : 'U';
          break;
        case 2:
          letter[0] = (desc->flags & AV_PIX_FMT_FLAG_RGB) ? 'B' : 'V';
          break;
        case 3:
          letter[0] = 'A';
          break;
        default:
          break;
      }
      status = beam_set_string_utf8(env, comp, "code", letter);
      CHECK_STATUS;

      status = beam_set_int32(env, comp, "plane", desc->comp[x].plane);
      CHECK_STATUS;
      status = beam_set_int32(env, comp, "step", desc->comp[x].step);
      CHECK_STATUS;
      status = beam_set_int32(env, comp, "offset", desc->comp[x].offset);
      CHECK_STATUS;
      status = beam_set_int32(env, comp, "shift", desc->comp[x].shift);
      CHECK_STATUS;
      status = beam_set_int32(env, comp, "depth", desc->comp[x].depth);
      CHECK_STATUS;

      status = napi_set_element(env, comps, x, comp);
      CHECK_STATUS;
    }
    status = napi_set_named_property(env, pixfmt, "comp", comps);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, pixfmt, "alias",
      (desc->alias != nullptr) ? (char*) desc->alias : "");
    CHECK_STATUS;
    status = napi_set_named_property(env, result, desc->name, pixfmt);
    CHECK_STATUS;
  }

  return result;
}

napi_value protocols(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, inputs, outputs, element;
  void* opaque = nullptr;
  int pos = 0;
  const char* protocol;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  status = napi_create_array(env, &inputs);
  CHECK_STATUS;
  status = napi_create_array(env, &outputs);
  CHECK_STATUS;

  while ((protocol = avio_enum_protocols(&opaque, 0))) {
    status = napi_create_string_utf8(env, (char*) protocol, NAPI_AUTO_LENGTH, &element);
    CHECK_STATUS;
    status = napi_set_element(env, inputs, pos++, element);
    CHECK_STATUS;
  }

  opaque = nullptr;
  pos = 0;
  while ((protocol = avio_enum_protocols(&opaque, 1))) {
    status = napi_create_string_utf8(env, (char*) protocol, NAPI_AUTO_LENGTH, &element);
    CHECK_STATUS;
    status = napi_set_element(env, outputs, pos++, element);
    CHECK_STATUS;
  }

  status = napi_set_named_property(env, result, "inputs", inputs);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "outputs", outputs);
  CHECK_STATUS;

  return result;
}

napi_value filters(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, desc, flags, pads, pad;
  void* opaque = nullptr;
  const AVFilter* filter;
  int padCount;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  while ((filter = av_filter_iterate(&opaque))) {
    status = napi_create_object(env, &desc);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, desc, "name", (char*) filter->name);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, desc, "description", (char*) filter->description);
    CHECK_STATUS;

    status = napi_create_array(env, &pads);
    CHECK_STATUS;
    padCount = avfilter_pad_count(filter->inputs);
    for ( int x = 0 ; x < padCount ; x++ ) {
      status = napi_create_object(env, &pad);
      CHECK_STATUS;
      status = beam_set_string_utf8(env, pad, "name",
        (char*) avfilter_pad_get_name(filter->inputs, x));
      CHECK_STATUS;
      status = beam_set_string_utf8(env, pad, "media_type",
        (char*) av_get_media_type_string(avfilter_pad_get_type(filter->inputs, x)));
      CHECK_STATUS;
      status = napi_set_element(env, pads, x, pad);
      CHECK_STATUS;
    }
    status = napi_set_named_property(env, desc, "inputs", pads);
    CHECK_STATUS;

    status = napi_create_array(env, &pads);
    CHECK_STATUS;
    padCount = avfilter_pad_count(filter->outputs);
    for ( int x = 0 ; x < padCount ; x++ ) {
      status = napi_create_object(env, &pad);
      CHECK_STATUS;
      status = beam_set_string_utf8(env, pad, "name",
        (char*) avfilter_pad_get_name(filter->outputs, x));
      CHECK_STATUS;
      status = beam_set_string_utf8(env, pad, "media_type",
        (char*) av_get_media_type_string(avfilter_pad_get_type(filter->outputs, x)));
      CHECK_STATUS;
      status = napi_set_element(env, pads, x, pad);
      CHECK_STATUS;
    }
    status = napi_set_named_property(env, desc, "outputs", pads);
    CHECK_STATUS;

    // TODO expand class details
    if (filter->priv_class != nullptr) {
      status = beam_set_string_utf8(env, desc, "priv_class",
        (char*) filter->priv_class->class_name);
      CHECK_STATUS;
    } else {
      status = napi_get_null(env, &pad);
      CHECK_STATUS;
      status = napi_set_named_property(env, desc, "priv_class", pad);
      CHECK_STATUS;
    }

    status = napi_create_object(env, &flags);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "DYNAMIC_INPUTS",
      filter->flags & AVFILTER_FLAG_DYNAMIC_INPUTS);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "DYNAMIC_OUTPUTS",
      filter->flags & AVFILTER_FLAG_DYNAMIC_OUTPUTS);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "SLICE_THREADS",
      filter->flags & AVFILTER_FLAG_SLICE_THREADS);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "SUPPORT_TIMELINE_GENERIC",
      filter->flags & AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC);
    CHECK_STATUS;
    status = beam_set_bool(env, flags, "SUPPORT_TIMELINE_INTERNAL",
      filter->flags & AVFILTER_FLAG_SUPPORT_TIMELINE_INTERNAL);
    CHECK_STATUS;
    status = napi_set_named_property(env, desc, "flags", flags);
    CHECK_STATUS;

    status = napi_set_named_property(env, result, filter->name, desc);
    CHECK_STATUS;
  }

  return result;
}

napi_value bsfs(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, desc, codecs, codec;
  void* opaque = nullptr;
  const AVBitStreamFilter* bsf;
  const AVCodecID* codecID;
  int pos = 0;

  status = napi_create_object(env, &result);
  CHECK_STATUS;

  while ((bsf = av_bsf_iterate(&opaque))) {
    status = napi_create_object(env, &desc);
    CHECK_STATUS;
    status = beam_set_string_utf8(env, desc, "name", (char*) bsf->name);
    CHECK_STATUS;

    status = napi_create_array(env, &codecs);
    CHECK_STATUS;
    codecID = bsf->codec_ids;
    if (codecID != nullptr) {
      pos = 0;
      while (*codecID != AV_CODEC_ID_NONE) {
        status = napi_create_string_utf8(env,
          (char*) avcodec_get_name(*codecID), NAPI_AUTO_LENGTH, &codec);
        CHECK_STATUS;
        status = napi_set_element(env, codecs, pos++, codec);
        CHECK_STATUS;
        codecID = codecID + 1;
      }
    }
    status = napi_set_named_property(env, desc, "codec_ids", codecs);
    CHECK_STATUS;

    // TODO expand class details
    if (bsf->priv_class != nullptr) {
      status = fromAVClass(env, bsf->priv_class, &codec);
      CHECK_STATUS;
    } else {
      status = napi_get_null(env, &codec);
      CHECK_STATUS;
    }
    status = napi_set_named_property(env, desc, "priv_class", codec);
    CHECK_STATUS;

    status = napi_set_named_property(env, result, bsf->name, desc);
    CHECK_STATUS;
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
    DECLARE_NAPI_METHOD("format", demuxer), // deprecated - so young!
    DECLARE_NAPI_METHOD("decoder", decoder),
    DECLARE_NAPI_METHOD("filterer", filterer),
    DECLARE_NAPI_METHOD("encoder", encoder), // 10
    DECLARE_NAPI_METHOD("codecs", codecs),
    DECLARE_NAPI_METHOD("muxers", muxers),
    DECLARE_NAPI_METHOD("demuxers", demuxers),
    DECLARE_NAPI_METHOD("pix_fmts", pix_fmts),
    DECLARE_NAPI_METHOD("protocols", protocols),
    DECLARE_NAPI_METHOD("filters", filters),
    DECLARE_NAPI_METHOD("bsfs", bsfs),
    DECLARE_NAPI_METHOD("makePacket", makePacket),
    DECLARE_NAPI_METHOD("makeFrame", makeFrame),
    DECLARE_NAPI_METHOD("makeCodecParameters", makeCodecParameters), // 20
    DECLARE_NAPI_METHOD("demuxer", demuxer),
    DECLARE_NAPI_METHOD("muxer", muxer),
    DECLARE_NAPI_METHOD("guessFormat", guessFormat)
  };
  status = napi_define_properties(env, exports, 23, desc);
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
