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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <string>
#include "beamcoder_util.h"
#include "node_api.h"

napi_status checkStatus(napi_env env, napi_status status,
  const char* file, uint32_t line) {

  napi_status infoStatus, throwStatus;
  const napi_extended_error_info *errorInfo;

  if (status == napi_ok) {
    // printf("Received status OK.\n");
    return status;
  }

  infoStatus = napi_get_last_error_info(env, &errorInfo);
  assert(infoStatus == napi_ok);
  printf("NAPI error in file %s on line %i. Error %i: %s\n", file, line,
    errorInfo->error_code, errorInfo->error_message);

  if (status == napi_pending_exception) {
    printf("NAPI pending exception. Engine error code: %i\n", errorInfo->engine_error_code);
    return status;
  }

  char errorCode[20];
  sprintf(errorCode, "%d", errorInfo->error_code);
  throwStatus = napi_throw_error(env, errorCode, errorInfo->error_message);
  assert(throwStatus == napi_ok);

  return napi_pending_exception; // Expect to be cast to void
}

long long microTime(std::chrono::high_resolution_clock::time_point start) {
  auto elapsed = std::chrono::high_resolution_clock::now() - start;
  return std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

const char* getNapiTypeName(napi_valuetype t) {
  switch (t) {
    case napi_undefined: return "undefined";
    case napi_null: return "null";
    case napi_boolean: return "boolean";
    case napi_number: return "number";
    case napi_string: return "string";
    case napi_symbol: return "symbol";
    case napi_object: return "object";
    case napi_function: return "function";
    case napi_external: return "external";
    default: return "unknown";
  }
}

napi_status checkArgs(napi_env env, napi_callback_info info, char* methodName,
  napi_value* args, size_t argc, napi_valuetype* types) {

  napi_status status;

  size_t realArgc = argc;
  status = napi_get_cb_info(env, info, &realArgc, args, nullptr, nullptr);
  PASS_STATUS;

  if (realArgc != argc) {
    char errorMsg[100];
    sprintf(errorMsg, "For method %s, expected %zi arguments and got %zi.",
      methodName, argc, realArgc);
    napi_throw_error(env, nullptr, errorMsg);
    return napi_pending_exception;
  }

  napi_valuetype t;
  for ( size_t x = 0 ; x < argc ; x++ ) {
    status = napi_typeof(env, args[x], &t);
    PASS_STATUS;
    if (t != types[x]) {
      char errorMsg[100];
      sprintf(errorMsg, "For method %s argument %zu, expected type %s and got %s.",
        methodName, x + 1, getNapiTypeName(types[x]), getNapiTypeName(t));
      napi_throw_error(env, nullptr, errorMsg);
      return napi_pending_exception;
    }
  }

  return napi_ok;
};


void tidyCarrier(napi_env env, carrier* c) {
  napi_status status;
  if (c->passthru != nullptr) {
    status = napi_delete_reference(env, c->passthru);
    FLOATING_STATUS;
  }
  if (c->_request != nullptr) {
    status = napi_delete_async_work(env, c->_request);
    FLOATING_STATUS;
  }
  // printf("Tidying carrier %p %p\n", c->passthru, c->_request);
  delete c;
}

int32_t rejectStatus(napi_env env, carrier* c, char* file, int32_t line) {
  if (c->status != BEAMCODER_SUCCESS) {
    napi_value errorValue, errorCode, errorMsg;
    napi_status status;
    if (c->status < BEAMCODER_ERROR_START) {
      const napi_extended_error_info *errorInfo;
      status = napi_get_last_error_info(env, &errorInfo);
      FLOATING_STATUS;
      c->errorMsg = std::string(
        (errorInfo->error_message != nullptr) ? errorInfo->error_message : "(no message)");
    }
    char* extMsg = (char *) malloc(sizeof(char) * c->errorMsg.length() + 200);
    sprintf(extMsg, "In file %s on line %i, found error: %s", file, line, c->errorMsg.c_str());
    char errorCodeChars[20];
    sprintf(errorCodeChars, "%d", c->status);
    status = napi_create_string_utf8(env, errorCodeChars,
      NAPI_AUTO_LENGTH, &errorCode);
    FLOATING_STATUS;
    status = napi_create_string_utf8(env, extMsg, NAPI_AUTO_LENGTH, &errorMsg);
    FLOATING_STATUS;
    status = napi_create_error(env, errorCode, errorMsg, &errorValue);
    FLOATING_STATUS;
    status = napi_reject_deferred(env, c->_deferred, errorValue);
    FLOATING_STATUS;

    delete[] extMsg;
    tidyCarrier(env, c);
  }
  return c->status;
}

// Should never get called
napi_value nop(napi_env env, napi_callback_info info) {
  napi_value value;
  napi_status status;
  status = napi_get_undefined(env, &value);
  if (status != napi_ok) NAPI_THROW_ERROR("Failed to retrieve undefined in nop.");
  return value;
}

char* avErrorMsg(const char* base, int avError) {
  char errMsg[AV_ERROR_MAX_STRING_SIZE];
  char* both;
  int ret = av_strerror(avError, errMsg, AV_ERROR_MAX_STRING_SIZE);
  if (ret < 0) {
    strcpy(errMsg, "Unable to create AV error string.");
  }
  size_t len = strlen(base) + strlen(errMsg);
  both = (char*) malloc(sizeof(char) * (len + 1));
  both[0] = '\0';
  strcat(both, base);
  strcat(both, errMsg);
  return both;
}

napi_status getPropsFromCodec(napi_env env, napi_value target,
    AVCodecContext* codec, bool encoding) {
  napi_status status;
  napi_value array, element, sub;
  int64_t iValue;
  double dValue;
  uint8_t* data;
  AVRational qValue;
  int ret;
  const AVOption* option = nullptr;
  const AVOption* prev = nullptr;

  status = beam_set_int32(env, target, "codec_id", codec->codec_id);
  PASS_STATUS;
  status = beam_set_string_utf8(env, target, "name", (char*) codec->codec->name);
  PASS_STATUS;
  status = beam_set_string_utf8(env, target, "long_name", (char*) codec->codec->long_name);
  PASS_STATUS;
  status = beam_set_string_utf8(env, target, "codec_type",
    (char*) av_get_media_type_string(codec->codec_type));
  PASS_STATUS;
  char codecTag[AV_FOURCC_MAX_STRING_SIZE];
  status = beam_set_string_utf8(env, target, "codec_tag",
    av_fourcc_make_string(codecTag, codec->codec_tag));
  PASS_STATUS;

  if (codec->codec->priv_class != nullptr) {
    status = napi_create_object(env, &sub);
    PASS_STATUS;
    status = beam_set_string_utf8(env, sub, "type", (char*) codec->codec->priv_class->class_name);
    PASS_STATUS;
    while ((option = av_opt_next(codec->priv_data, option))) {
      switch (option->type) {
        case AV_OPT_TYPE_FLAGS:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: flags");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_INT: // based on x264 definitions
          ret = av_opt_get_int(codec->priv_data, option->name, 0, &iValue);
          if (option->unit == nullptr) {
            // printf("For %s, return is %i value %i\n", option->name, ret, iValue);
            status = beam_set_int32(env, sub, (char*) option->name, iValue);
            PASS_STATUS;
          } else {
            if (iValue < 0) {
              status = beam_set_string_utf8(env, sub, (char*) option->name, "unknown");
              PASS_STATUS;
            } else {
              data = (uint8_t*) option->name;
              prev = option;
              option = av_opt_next(codec->priv_data, option);
              while (option->type == AV_OPT_TYPE_CONST) {
                if (option->default_val.i64 == iValue) {
                  status = beam_set_string_utf8(env, sub, (char*) data, (char*) option->name);
                  PASS_STATUS;
                }
                prev = option;
                option = av_opt_next(codec->priv_data, option);
              }
              option = prev;
            }
          }
          break;
        case AV_OPT_TYPE_INT64:
        case AV_OPT_TYPE_UINT64:
          ret = av_opt_get_int(codec->priv_data, option->name, 0, &iValue);
          // printf("For %s, return is %i value %i\n", option->name, ret, iValue);
          status = beam_set_int64(env, sub, (char*) option->name, iValue);
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_DOUBLE:
        case AV_OPT_TYPE_FLOAT:
          av_opt_get_double(codec->priv_data, option->name, 0, &dValue);
          status = beam_set_double(env, sub, (char*) option->name, dValue);
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_STRING:
          av_opt_get(codec->priv_data, option->name, 0, &data);
          status = beam_set_string_utf8(env, sub, (char*) option->name, (char*) data);
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_RATIONAL:
          av_opt_get_q(codec->priv_data, option->name, 0, &qValue);
          status = beam_set_rational(env, sub, (char*) option->name, qValue);
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_BINARY:  ///< offset must point to a pointer immediately followed by an int for the length
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: binary");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_DICT:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: dict");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_CONST:
          // status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: const");
          // PASS_STATUS;
          break;
        case AV_OPT_TYPE_IMAGE_SIZE: ///< offset must point to two consecutive integers
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: image_size");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_PIXEL_FMT:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: pixel_fmt");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_SAMPLE_FMT:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: sample_fmt");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_VIDEO_RATE: ///< offset must point to AVRational
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: AVRational");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_DURATION:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: duration");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_COLOR:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: color");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_CHANNEL_LAYOUT:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unmapped type: channel_layout");
          PASS_STATUS;
          break;
        case AV_OPT_TYPE_BOOL:
          av_opt_get_int(codec->priv_data, option->name, 0, &iValue);
          status = beam_set_bool(env, sub, (char*) option->name, iValue);
          PASS_STATUS;
          break;
        default:
          status = beam_set_string_utf8(env, sub, (char*) option->name, "unknown type");
          PASS_STATUS;
          break;
      }

    }
    status = napi_set_named_property(env, target, "priv_data", sub);
    PASS_STATUS;
  }
  // Choosing to ignore internal
  // Choosing to ignore opaque
  status = beam_set_int64(env, target, "bit_rate", codec->bit_rate);
  PASS_STATUS;
  if (encoding) {
    status = beam_set_int32(env, target, "bit_rate_tolerance", codec->bit_rate_tolerance);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_set_int32(env, target, "global_quality", codec->global_quality);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_set_int32(env, target, "compression_level", codec->compression_level);
    PASS_STATUS;
  }
  status = napi_create_object(env, &sub);
  PASS_STATUS;
  /**
   * Allow decoders to produce frames with data planes that are not aligned
   * to CPU requirements (e.g. due to cropping).
   */
  status = beam_set_bool(env, sub, "UNALIGNED", (codec->flags & AV_CODEC_FLAG_UNALIGNED) != 0);
  PASS_STATUS;
  /**
   * Use fixed qscale.
   */
  status = beam_set_bool(env, sub, "QSCALE", (codec->flags & AV_CODEC_FLAG_QSCALE) != 0);
  PASS_STATUS;
  /**
   * 4 MV per MB allowed / advanced prediction for H.263.
   */
  status = beam_set_bool(env, sub, "4MV", (codec->flags & AV_CODEC_FLAG_4MV) != 0);
  PASS_STATUS;
  /**
   * Output even those frames that might be corrupted.
   */
  status = beam_set_bool(env, sub, "OUTPUT_CORRUPT", (codec->flags & AV_CODEC_FLAG_OUTPUT_CORRUPT) != 0);
  PASS_STATUS;
  /**
   * Use qpel MC.
   */
  status = beam_set_bool(env, sub, "QPEL", (codec->flags & AV_CODEC_FLAG_QPEL) != 0);
  PASS_STATUS;
  /**
   * Use internal 2pass ratecontrol in first pass mode.
   */
  status = beam_set_bool(env, sub, "PASS1", (codec->flags & AV_CODEC_FLAG_PASS1) != 0);
  PASS_STATUS;
  /**
   * Use internal 2pass ratecontrol in second pass mode.
   */
  status = beam_set_bool(env, sub, "PASS2", (codec->flags & AV_CODEC_FLAG_PASS2) != 0);
  PASS_STATUS;
  /**
   * loop filter.
   */
  status = beam_set_bool(env, sub, "LOOP_FILTER", (codec->flags & AV_CODEC_FLAG_LOOP_FILTER) != 0);
  PASS_STATUS;
  /**
   * Only decode/encode grayscale.
   */
  status = beam_set_bool(env, sub, "GRAY", (codec->flags & AV_CODEC_FLAG_GRAY) != 0);
  PASS_STATUS;
  /**
   * error[?] variables will be set during encoding.
   */
  status = beam_set_bool(env, sub, "PSNR", (codec->flags & AV_CODEC_FLAG_PSNR) != 0);
  PASS_STATUS;
  /**
   * Input bitstream might be truncated at a random location
   * instead of only at frame boundaries.
   */
  status = beam_set_bool(env, sub, "TRUNCATED", (codec->flags & AV_CODEC_FLAG_TRUNCATED) != 0);
  PASS_STATUS;
  /**
   * Use interlaced DCT.
   */
  status = beam_set_bool(env, sub, "INTERLACED_DCT", (codec->flags & AV_CODEC_FLAG_INTERLACED_DCT) != 0);
  PASS_STATUS;
  /**
   * Force low delay.
   */
  status = beam_set_bool(env, sub, "LOW_DELAY", (codec->flags & AV_CODEC_FLAG_LOW_DELAY) != 0);
  PASS_STATUS;
  /**
   * Place global headers in extradata instead of every keyframe.
   */
  status = beam_set_bool(env, sub, "GLOBAL_HEADER", (codec->flags & AV_CODEC_FLAG_GLOBAL_HEADER) != 0);
  PASS_STATUS;
  /**
   * Use only bitexact stuff (except (I)DCT).
   */
  status = beam_set_bool(env, sub, "BITEXACT", (codec->flags & AV_CODEC_FLAG_BITEXACT) != 0);
  PASS_STATUS;
  /* Fx : Flag for H.263+ extra options */
  /**
   * H.263 advanced intra coding / MPEG-4 AC prediction
   */
  status = beam_set_bool(env, sub, "AC_PRED", (codec->flags & AV_CODEC_FLAG_AC_PRED) != 0);
  PASS_STATUS;
  /**
   * interlaced motion estimation
   */
  status = beam_set_bool(env, sub, "INTERLACED_ME", (codec->flags & AV_CODEC_FLAG_INTERLACED_ME) != 0);
  PASS_STATUS;

  status = beam_set_bool(env, sub, "CLOSED_GOP", (codec->flags & AV_CODEC_FLAG_CLOSED_GOP) != 0);
  PASS_STATUS;
  status = napi_set_named_property(env, target, "flags", sub);
  PASS_STATUS;

  status = napi_create_object(env, &sub);
  PASS_STATUS;
  /**
   * Allow non spec compliant speedup tricks.
   */
  status = beam_set_bool(env, sub, "FAST", (codec->flags2 & AV_CODEC_FLAG2_FAST) != 0);
  PASS_STATUS;
  /**
   * Skip bitstream encoding.
   */
  status = beam_set_bool(env, sub, "NO_OUTPUT", (codec->flags2 & AV_CODEC_FLAG2_NO_OUTPUT) != 0);
  PASS_STATUS;
  /**
   * Place global headers at every keyframe instead of in extradata.
   */
  status = beam_set_bool(env, sub, "LOCAL_HEADER", (codec->flags2 & AV_CODEC_FLAG2_LOCAL_HEADER) != 0);
  PASS_STATUS;

  /**
   * timecode is in drop frame format. DEPRECATED!!!!
   */
  status = beam_set_bool(env, sub, "DROP_FRAME_TIMECODE", (codec->flags2 & AV_CODEC_FLAG2_DROP_FRAME_TIMECODE) != 0);
  PASS_STATUS;

  /**
   * Input bitstream might be truncated at a packet boundaries
   * instead of only at frame boundaries.
   */
  status = beam_set_bool(env, sub, "CHUNKS", (codec->flags2 & AV_CODEC_FLAG2_CHUNKS) != 0);
  PASS_STATUS;
  /**
   * Discard cropping information from SPS.
   */
  status = beam_set_bool(env, sub, "IGNORE_CROP", (codec->flags2 & AV_CODEC_FLAG2_IGNORE_CROP) != 0);
  PASS_STATUS;

  /**
   * Show all frames before the first keyframe
   */
  status = beam_set_bool(env, sub, "SHOW_ALL", (codec->flags2 & AV_CODEC_FLAG2_SHOW_ALL) != 0);
  PASS_STATUS;
  /**
   * Export motion vectors through frame side data
   */
  status = beam_set_bool(env, sub, "EXPORT_MVS", (codec->flags2 & AV_CODEC_FLAG2_EXPORT_MVS) != 0);
  PASS_STATUS;
  /**
   * Do not skip samples and export skip information as frame side data
   */
  status = beam_set_bool(env, sub, "SKIP_MANUAL", (codec->flags2 & AV_CODEC_FLAG2_SKIP_MANUAL) != 0);
  PASS_STATUS;
  /**
   * Do not reset ASS ReadOrder field on flush (subtitles decoding)
   */
  status = beam_set_bool(env, sub, "RO_FLUSH_NOOP", (codec->flags2 & AV_CODEC_FLAG2_RO_FLUSH_NOOP) != 0);
  PASS_STATUS;
  status = napi_set_named_property(env, target, "flags2", sub);
  PASS_STATUS;

  if (codec->extradata_size > 0) {
    status = napi_create_external_buffer(env, codec->extradata_size, codec->extradata,
      nullptr, nullptr, &sub);
    PASS_STATUS;
    status = napi_set_named_property(env, target, "extradata", sub);
    PASS_STATUS;
  }

  if (encoding) {
    status = beam_set_rational(env, target, "time_base", codec->time_base);
    PASS_STATUS;
    status = beam_set_int32(env, target, "ticks_per_frame", codec->ticks_per_frame);
    PASS_STATUS;
  }

  if (!(encoding && (codec->codec_type == AVMEDIA_TYPE_AUDIO))) {
    status = beam_set_int32(env, target, "delay", codec->delay);
    PASS_STATUS;
  }

  if (codec->codec_type == AVMEDIA_TYPE_VIDEO) {
    status = beam_set_int32(env, target, "width", codec->width);
    PASS_STATUS;
    status = beam_set_int32(env, target, "height", codec->height);
    PASS_STATUS;
    if (!encoding) {
      status = beam_set_int32(env, target, "coded_width", codec->coded_width);
      PASS_STATUS;
      status = beam_set_int32(env, target, "coded_height", codec->coded_height);
      PASS_STATUS;
    }
    status = beam_set_string_utf8(env, target, "pix_fmt",
      (char*) av_get_pix_fmt_name(codec->pix_fmt));
    PASS_STATUS;
    if (encoding) {
      status = beam_set_int32(env, target, "max_b_frames", codec->max_b_frames);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "b_quant_factor", codec->b_quant_factor);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "b_quant_offset", codec->b_quant_offset);
      PASS_STATUS;
    }
    status = beam_set_int32(env, target, "has_b_frames", codec->has_b_frames);
    PASS_STATUS;
    if (encoding) {
      status = beam_set_double(env, target, "i_quant_factor", codec->i_quant_factor);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "i_quant_offset", codec->i_quant_offset);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "lumi_masking", codec->lumi_masking);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "temporal_cplx_masking", codec->temporal_cplx_masking);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "spatial_cplx_masking", codec->spatial_cplx_masking);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "p_masking", codec->p_masking);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_double(env, target, "dask_masking", codec->dark_masking);
      PASS_STATUS;
    }
    status = napi_create_array(env, &array);
    PASS_STATUS;
    for ( int x = 0 ; x < codec->slice_count ; x++ ) {
      status = napi_create_int32(env, codec->slice_offset[x], &element);
      PASS_STATUS;
      status = napi_set_element(env, array, x, element);
      PASS_STATUS;
    }
    status = napi_set_named_property(env, target, "slice_offset", array);
    PASS_STATUS;

    status = beam_set_rational(env, target, "sample_aspect_ratio", codec->sample_aspect_ratio);
    PASS_STATUS;

    if (encoding) {
      status = beam_set_enum(env, target, "me_cmp", beam_ff_cmp, codec->me_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_enum(env, target, "me_sub_cmp", beam_ff_cmp, codec->me_sub_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_enum(env, target, "mb_cmp", beam_ff_cmp, codec->mb_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_enum(env, target, "ildct_cmp", beam_ff_cmp, codec->ildct_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "dia_size", codec->dia_size);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "last_predictor_count",
        codec->last_predictor_count);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_enum(env, target, "me_pre_cmp", beam_ff_cmp, codec->me_pre_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "pre_dia_size", codec->pre_dia_size);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "me_subpel_quality", codec->me_subpel_quality);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "me_range", codec->me_range);
      PASS_STATUS;
    }

    if (!encoding) {
      // draw_horiz_band() is called in coded order instead of display
      status = beam_set_bool(env, target, "SLICE_FLAG_CODED_ORDER",
        (codec->slice_flags & SLICE_FLAG_CODED_ORDER) != 0);
      PASS_STATUS;
      // allow draw_horiz_band() with field slices (MPEG-2 field pics)
      status = beam_set_bool(env, target, "SLICE_FLAG_ALLOW_FIELD",
        (codec->slice_flags & SLICE_FLAG_ALLOW_FIELD) != 0);
      PASS_STATUS;
      // allow draw_horiz_band() with 1 component at a time (SVQ1)
      status = beam_set_bool(env, target, "SLICE_FLAG_ALLOW_PLANE",
        (codec->slice_flags & SLICE_FLAG_ALLOW_PLANE) != 0);
      PASS_STATUS;
    }

    if (encoding) {
      status = beam_set_enum(env, target, "mb_decision", beam_ff_mb_decision, codec->mb_decision);
      PASS_STATUS;
    }

    if (codec->intra_matrix != nullptr) {
      status = napi_create_array(env, &array);
      PASS_STATUS;
      for ( int x = 0 ; x < 64 ; x++ ) {
        status = napi_create_uint32(env, codec->intra_matrix[x], &element);
        PASS_STATUS;
        status = napi_set_element(env, array, x, element);
        PASS_STATUS;
      }
      status = napi_set_named_property(env, target, "intra_matrix", array);
      PASS_STATUS;
    }

    if (codec->inter_matrix != nullptr) {
      status = napi_create_array(env, &array);
      PASS_STATUS;
      for ( int x = 0 ; x < 64 ; x++ ) {
        status = napi_create_uint32(env, codec->inter_matrix[x], &element);
        PASS_STATUS;
        status = napi_set_element(env, array, x, element);
        PASS_STATUS;
      }
      status = napi_set_named_property(env, target, "inter_matrix", array);
      PASS_STATUS;
    }

    status = beam_set_int32(env, target, "intra_dc_precision", codec->intra_dc_precision);
    PASS_STATUS;
    if (!encoding) {
      status = beam_set_int32(env, target, "skip_top", codec->skip_top);
      PASS_STATUS;
    }
    if (!encoding) {
      status = beam_set_int32(env, target, "skip_bottom", codec->skip_bottom);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "mb_lmin", codec->mb_lmin);
      PASS_STATUS;
      status = beam_set_int32(env, target, "mb_lmax", codec->mb_lmax);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "bidir_refine", codec->bidir_refine);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_set_int32(env, target, "keyint_min", codec->keyint_min);
      PASS_STATUS;
    }
    status = beam_set_int32(env, target, "refs", codec->refs);
    PASS_STATUS;
    if (encoding) {
      status = beam_set_int32(env, target, "mv0_threshold", codec->mv0_threshold);
      PASS_STATUS;
    }
    status = beam_set_string_utf8(env, target, "color_primaries",
      (char*) av_color_primaries_name(codec->color_primaries));
    PASS_STATUS;
    status = beam_set_string_utf8(env, target, "color_trc",
      (char*) av_color_transfer_name(codec->color_trc));
    PASS_STATUS;
    status = beam_set_string_utf8(env, target, "colorspace",
      (char*) av_color_space_name(codec->colorspace));
    PASS_STATUS;
    status = beam_set_string_utf8(env, target, "color_range",
      (char*) av_color_range_name(codec->color_range));
    PASS_STATUS;
    status = beam_set_string_utf8(env, target, "chroma_sample_location",
      (char*) av_chroma_location_name(codec->chroma_sample_location));
    PASS_STATUS;

    if (encoding) {
      status = beam_set_int32(env, target, "slices", codec->slices);
      PASS_STATUS;
    }
    status = beam_set_enum(env, target, "field_order", beam_field_order,
      codec->field_order);
    PASS_STATUS;
  } // Video-only parameters

  if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    status = beam_set_int32(env, target, "sample_rate", codec->sample_rate);
    PASS_STATUS;
    status = beam_set_int32(env, target, "channels", codec->channels);
    PASS_STATUS;

    status = beam_set_string_utf8(env, target, "sample_fmt",
      (char*) av_get_sample_fmt_name(codec->sample_fmt));
    PASS_STATUS;
    status = beam_set_int32(env, target, "frame_size", codec->frame_size);
    PASS_STATUS;
    status = beam_set_int32(env, target, "frame_number", codec->frame_number);
    PASS_STATUS;
    status = beam_set_int32(env, target, "block_align", codec->block_align);
    PASS_STATUS;
    if (encoding) {
      status = beam_set_int32(env, target, "cutoff", codec->cutoff);
      PASS_STATUS;
    }
    char channelLayoutName[64];
    av_get_channel_layout_string(channelLayoutName, 64, codec->channels,
      codec->channel_layout);
    status = beam_set_string_utf8(env, target, "channel_layout", channelLayoutName);
    PASS_STATUS;
    if (!encoding) {
      char reqChanLayoutName[64];
      av_get_channel_layout_string(reqChanLayoutName, 64, codec->channels,
        codec->request_channel_layout);
      status = beam_set_string_utf8(env, target, "request_channel_layout",
        reqChanLayoutName);
      PASS_STATUS;
    }
    status = beam_set_enum(env, target, "audio_service_type",
      beam_av_audio_service_type, codec->audio_service_type);
    PASS_STATUS;
    if (!encoding) {
      status = beam_set_string_utf8(env, target, "request_sample_fmt",
        (char*) av_get_sample_fmt_name(codec->request_sample_fmt));
      PASS_STATUS;
    }
  } // Audio-only Parameters

  // Encoding parameters
  if (encoding) {
    status = beam_set_double(env, target, "qcompress", codec->qcompress);
    PASS_STATUS;
    status = beam_set_double(env, target, "qblur", codec->qblur);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_set_int32(env, target, "qmin", codec->qmin);
    PASS_STATUS;
    status = beam_set_int32(env, target, "qmax", codec->qmax);
    PASS_STATUS;
    status = beam_set_int32(env, target, "max_qdiff", codec->max_qdiff);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_set_int32(env, target, "rc_buffer_size", codec->rc_buffer_size);
    PASS_STATUS;
  }
  if (encoding) {
    status = napi_create_array(env, &array);
    PASS_STATUS;
    for ( int x = 0 ; x < codec->rc_override_count ; x++) {
      status = napi_create_object(env, &element);
      PASS_STATUS;
      status = beam_set_string_utf8(env, element, "type", "RcOverride");
      PASS_STATUS;
      status = beam_set_int32(env, element, "start_frame", codec->rc_override[x].start_frame);
      PASS_STATUS;
      status = beam_set_int32(env, element, "end_frame", codec->rc_override[x].end_frame);
      PASS_STATUS;
      status = beam_set_int32(env, element, "qscale", codec->rc_override[x].qscale);
      PASS_STATUS;
      status = beam_set_double(env, element, "quality_factor", codec->rc_override[x].quality_factor);
      PASS_STATUS;
      status = napi_set_element(env, array, x, element);
      PASS_STATUS;
    }
    status = napi_set_named_property(env, target, "rc_override", array);
    PASS_STATUS;
  }
  status = beam_set_int64(env, target, "rc_max_rate", codec->rc_max_rate);
  PASS_STATUS;
  if (encoding) {
    status = beam_set_int64(env, target, "rc_min_rate", codec->rc_min_rate);
    PASS_STATUS;
    status = beam_set_double(env, target, "rc_max_available_vbv_use",
      codec->rc_max_available_vbv_use);
    PASS_STATUS;
    status = beam_set_double(env, target, "rc_min_vbv_overflow_use",
      codec->rc_min_vbv_overflow_use);
    PASS_STATUS;
    status = beam_set_int32(env, target, "rc_initial_buffer_occupancy",
      codec->rc_initial_buffer_occupancy);
    PASS_STATUS;
  }

  if (encoding) {
    status = beam_set_int32(env, target, "trellis", codec->trellis);
    PASS_STATUS;
  }
  // TODO stats_out, stats_in
  status = napi_create_object(env, &sub);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "AUTODETECT", (codec->workaround_bugs & FF_BUG_AUTODETECT) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "XVID_ILACE", (codec->workaround_bugs & FF_BUG_XVID_ILACE) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "UMP4", (codec->workaround_bugs & FF_BUG_UMP4) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "NO_PADDING", (codec->workaround_bugs & FF_BUG_NO_PADDING) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "AMV", (codec->workaround_bugs & FF_BUG_AMV) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "QPEL_CHROMA", (codec->workaround_bugs & FF_BUG_QPEL_CHROMA) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "STD_QPEL", (codec->workaround_bugs & FF_BUG_STD_QPEL) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "QPEL_CHROMA2", (codec->workaround_bugs & FF_BUG_QPEL_CHROMA2) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "DIRECT_BLOCKSIZE", (codec->workaround_bugs & FF_BUG_DIRECT_BLOCKSIZE) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "EDGE", (codec->workaround_bugs & FF_BUG_EDGE) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "HPEL_CHROMA", (codec->workaround_bugs & FF_BUG_HPEL_CHROMA) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "DC_CLIP", (codec->workaround_bugs & FF_BUG_DC_CLIP) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "MS", (codec->workaround_bugs & FF_BUG_MS) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "TRUNCATED", (codec->workaround_bugs & FF_BUG_TRUNCATED) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "IEDGE", (codec->workaround_bugs & FF_BUG_IEDGE) != 0);
  PASS_STATUS;
  status = napi_set_named_property(env, target, "workaround_bugs", sub);
  PASS_STATUS;

  status = beam_set_enum(env, target, "strict_std_compliance", beam_ff_compliance,
    codec->strict_std_compliance);
  PASS_STATUS;

  if (!encoding) {
    status = napi_create_object(env, &sub);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "GUESS_MVS",
      (codec->error_concealment & FF_EC_GUESS_MVS) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "DEBLOCK",
      (codec->error_concealment & FF_EC_DEBLOCK) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "FAVOR_INTER",
      (codec->error_concealment & FF_EC_FAVOR_INTER) != 0);
    PASS_STATUS;
    status = napi_set_named_property(env, target, "error_concealment", sub);
    PASS_STATUS;
  }

  status = napi_create_object(env, &sub);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "PICT_INFO", (codec->debug & FF_DEBUG_PICT_INFO) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "RC", (codec->debug & FF_DEBUG_RC) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "BITSTREAM", (codec->debug & FF_DEBUG_BITSTREAM) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "MB_TYPE", (codec->debug & FF_DEBUG_MB_TYPE) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "QP", (codec->debug & FF_DEBUG_QP) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "DCT_COEFF", (codec->debug & FF_DEBUG_DCT_COEFF) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "SKIP", (codec->debug & FF_DEBUG_SKIP) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "STARTCODE", (codec->debug & FF_DEBUG_STARTCODE) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "ER", (codec->debug & FF_DEBUG_ER) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "MMCO", (codec->debug & FF_DEBUG_MMCO) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "BUGS", (codec->debug & FF_DEBUG_BUGS) != 0);
  PASS_STATUS;
  #if FF_API_DEBUG_MV
  status = beam_set_bool(env, sub, "VIS_QP", (codec->debug & FF_DEBUG_VIS_QP) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "VIS_MB_TYPE", (codec->debug & FF_DEBUG_VIS_MB_TYPE) != 0);
  PASS_STATUS;
  #endif
  status = beam_set_bool(env, sub, "BUFFERS", (codec->debug & FF_DEBUG_BUFFERS) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "THREADS", (codec->debug & FF_DEBUG_THREADS) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "GREEN_MD", (codec->debug & FF_DEBUG_GREEN_MD) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "NOMC", (codec->debug & FF_DEBUG_NOMC) != 0);
  PASS_STATUS;
  status = napi_set_named_property(env, target, "debug", sub);
  PASS_STATUS;

  if (!encoding) {
    status = napi_create_object(env, &sub);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "CRCCHECK", (codec->err_recognition & AV_EF_CRCCHECK) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "BITSTREAM", (codec->err_recognition & AV_EF_BITSTREAM) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "BUFFER", (codec->err_recognition & AV_EF_BUFFER) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "EXPLODE", (codec->err_recognition & AV_EF_EXPLODE) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "IGNORE_ERR", (codec->err_recognition & AV_EF_IGNORE_ERR) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "CAREFUL", (codec->err_recognition & AV_EF_CAREFUL) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "COMPLIANT", (codec->err_recognition & AV_EF_COMPLIANT) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "AGGRESSIVE", (codec->err_recognition & AV_EF_AGGRESSIVE) != 0);
    PASS_STATUS;
    status = napi_set_named_property(env, target, "err_recognition", sub);
    PASS_STATUS;
  } // err_recognition

  if (!encoding) {
    status = beam_set_int64(env, target, "reordered_opaque", codec->reordered_opaque);
    PASS_STATUS;
  }

  // TODO hardware accelerator Fields hwaccel and hwaccel_context - do when working on HW in general

  if (encoding && (codec->flags & AV_CODEC_FLAG_PSNR)) {
    status = napi_create_array(env, &array);
    PASS_STATUS;
    for ( int x = 0 ; x < AV_NUM_DATA_POINTERS ; x++ ) {
      // TODO consider using JS BigInt when not experimental
      status = napi_create_int64(env, (int64_t) codec->error[x], &element);
      PASS_STATUS;
      status = napi_set_element(env, array, x, element);
      PASS_STATUS;
    }
    status = napi_set_named_property(env, target, "error", array);
    PASS_STATUS;
  }

  if (encoding) {
    status = beam_set_enum(env, target, "dct_algo", beam_ff_dct, codec->dct_algo);
    PASS_STATUS;
  }
  status = beam_set_enum(env, target, "idct_algo", beam_ff_idct, codec->idct_algo);
  PASS_STATUS;
  status = beam_set_int32(env, target, "bits_per_coded_sample", codec->bits_per_coded_sample);
  PASS_STATUS;
  status = beam_set_int32(env, target, "bits_per_raw_sample", codec->bits_per_raw_sample);
  PASS_STATUS;
  status = beam_set_int32(env, target, "thread_count", codec->thread_count);
  PASS_STATUS;

  status = napi_create_object(env, &sub);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "FRAME", (codec->thread_type & FF_THREAD_FRAME) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "SLICE", (codec->thread_type & FF_THREAD_SLICE) != 0);
  PASS_STATUS;
  status = napi_set_named_property(env, target, "thread_type", sub);
  PASS_STATUS;

  status = napi_create_object(env, &sub);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "FRAME", (codec->active_thread_type & FF_THREAD_FRAME) != 0);
  PASS_STATUS;
  status = beam_set_bool(env, sub, "SLICE", (codec->active_thread_type & FF_THREAD_SLICE) != 0);
  PASS_STATUS;
  status = napi_set_named_property(env, target, "active_thread_type", sub);
  PASS_STATUS;

  // TODO not doing thread_safe_callbacks for now as not exposing custom get_buffer()

  if (encoding) {
    status = beam_set_int32(env, target, "nsse_weight", codec->nsse_weight);
    PASS_STATUS;
  }
  status = beam_set_string_utf8(env, target, "profile",
    (char*) av_get_profile_name(codec->codec, codec->profile));
  PASS_STATUS;

  status = beam_set_int32(env, target, "level", codec->level);
  PASS_STATUS;
  if (!encoding) {
    status = beam_set_enum(env, target, "skip_loop_filter", beam_avdiscard, codec->skip_loop_filter);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_set_enum(env, target, "skip_idct", beam_avdiscard, codec->skip_idct);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_set_enum(env, target, "skip_frame", beam_avdiscard, codec->skip_frame);
    PASS_STATUS;
  }

  if (codec->subtitle_header_size > 0) {
    status = napi_create_external_buffer(env, codec->subtitle_header_size,
      codec->subtitle_header, nullptr, nullptr, &sub);
    PASS_STATUS;
    status = napi_set_named_property(env, target, "subtitle_header", sub);
    PASS_STATUS;
  }

  if (encoding && (codec->codec_type == AVMEDIA_TYPE_AUDIO)) {
    status = beam_set_int32(env, target, "initial_padding", codec->initial_padding);
    PASS_STATUS;
  }
  status = beam_set_rational(env, target, "framerate", codec->framerate);
  PASS_STATUS;
  if (!encoding) {
    status = beam_set_string_utf8(env, target, "sw_pix_fmt",
      (char*) av_get_pix_fmt_name(codec->sw_pix_fmt));
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_set_rational(env, target, "pkt_timebase", codec->pkt_timebase);
    PASS_STATUS;
  }
  // TODO not supporing codec descriptor - yet
  if (!encoding) {
    status = beam_set_string_utf8(env, target, "sub_charenc", codec->sub_charenc);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_set_enum(env, target, "sub_charenc_mode",
      beam_ff_sub_charenc_mode, codec->sub_charenc_mode);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_set_int32(env, target, "skip_alpha", codec->skip_alpha);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_set_int32(env, target, "seek_preroll", codec->seek_preroll);
    PASS_STATUS;
  }
  if (encoding) {
    if (codec->chroma_intra_matrix != nullptr) {
      status = napi_create_array(env, &array);
      PASS_STATUS;
      for ( int x = 0 ; x < 64 ; x++ ) {
        status = napi_create_uint32(env, codec->chroma_intra_matrix[x], &element);
        PASS_STATUS;
        status = napi_set_element(env, array, x, element);
        PASS_STATUS;
      }
      status = napi_set_named_property(env, target, "chroma_intra_matrix", array);
      PASS_STATUS;
    }
  }

  status = beam_set_string_utf8(env, target, "dump_separator", (char*) codec->dump_separator);
  PASS_STATUS;
  if (!encoding) {
    status = napi_create_object(env, &sub);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "LOSSLESS", (codec->properties & FF_CODEC_PROPERTY_LOSSLESS) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "CLOSED_CAPTIONS", (codec->properties & FF_CODEC_PROPERTY_CLOSED_CAPTIONS) != 0);
    PASS_STATUS;
    status = napi_set_named_property(env, target, "properties", sub);
    PASS_STATUS;
  }

  // TODO hw_frames_ctx
  if (!encoding) { // Exposing as an integer - perhpas change to enum
    status = beam_set_int32(env, target, "sub_text_format", codec->sub_text_format);
    PASS_STATUS;
  }

  if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    status = beam_set_int32(env, target, "trailing_padding", codec->trailing_padding);
    PASS_STATUS;
  }
  status = beam_set_int64(env, target, "max_pixels", codec->max_pixels);
  PASS_STATUS;
  // TODO hw_device_ctx

  if (!encoding) {
    status = napi_create_object(env, &sub);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "IGNORE_LEVEL",
      (codec->hwaccel_flags & AV_HWACCEL_FLAG_IGNORE_LEVEL) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "ALLOW_HIGH_DEPTH",
      (codec->hwaccel_flags & AV_HWACCEL_FLAG_ALLOW_HIGH_DEPTH) != 0);
    PASS_STATUS;
    status = beam_set_bool(env, sub, "ALLOW_PROFILE_MISMATCH",
      (codec->hwaccel_flags & AV_HWACCEL_FLAG_ALLOW_PROFILE_MISMATCH) != 0);
    PASS_STATUS;
    status = napi_set_named_property(env, target, "hwaccel_flags", sub);
    PASS_STATUS;
  }

  // TODO coded side data

  if ((!encoding) && (codec->codec_type == AVMEDIA_TYPE_VIDEO)) {
    status = beam_set_bool(env, target, "apply_cropping", (codec->apply_cropping == 1));
    PASS_STATUS;
    status = beam_set_int32(env, target, "extra_hw_frames", codec->extra_hw_frames);
    PASS_STATUS;
  }

  return napi_ok;
};

napi_status setCodecFromProps(napi_env env, AVCodecContext* codec,
    napi_value props, bool encoding) {
  napi_status status;
  napi_value value, element, names;
  napi_valuetype type;
  bool present, flag, isArray;
  double dValue;
  uint32_t uThirtwo;
  char* sValue;
  const char* strProp;
  size_t sLen;
  const AVOption* option;
  int64_t iValue;
  uint8_t* data;
  int ret;

  if (codec->codec->priv_class != nullptr) {
    status = napi_get_named_property(env, props, "priv_data", &value);
    PASS_STATUS;
    status = napi_typeof(env, value, &type);
    PASS_STATUS;
    if (type == napi_object) {
      status = napi_get_property_names(env, value, &names);
      PASS_STATUS;
      status = napi_get_array_length(env, names, &uThirtwo);
      PASS_STATUS;
      for ( uint32_t x = 0 ; x < uThirtwo ; x++ ) {
        status = napi_get_element(env, names, x, &element);
        PASS_STATUS;
        status = napi_get_value_string_utf8(env, element, nullptr, 0, &sLen);
        PASS_STATUS;
        sValue = (char*) malloc(sizeof(char) * (sLen + 1));
        PASS_STATUS;
        status = napi_get_value_string_utf8(env, element, sValue, sLen + 1, &sLen);
        PASS_STATUS;
        option = av_opt_find(codec->priv_data, sValue, nullptr, 0, 0);
        if (option != nullptr) {
          status = napi_get_named_property(env, value, sValue, &element);
          PASS_STATUS;
          status = napi_typeof(env, element, &type);
          PASS_STATUS;
          switch (type) {
            case napi_boolean:
              status = napi_get_value_bool(env, element, &flag);
              PASS_STATUS;
              ret = av_opt_set_int(codec->priv_data, sValue, flag, 0);
              if (ret < 0) printf("DEBUG: Unable to set %s with a boolean value.\n", sValue);
              break;
            case napi_number:
              if ((option->type == AV_OPT_TYPE_DOUBLE) ||
                  (option->type == AV_OPT_TYPE_FLOAT)) {
                status = napi_get_value_double(env, element, &dValue);
                PASS_STATUS;
                ret = av_opt_set_double(codec->priv_data, sValue, dValue, 0);
                if (ret < 0) printf("DEBUG: Unable to set %s with a double value %f.\n", sValue, dValue);
                break;
              }
              status = napi_get_value_int64(env, element, &iValue);
              PASS_STATUS;
              ret = av_opt_set_int(codec->priv_data, sValue, iValue, 0);
              if (ret < 0) printf("DEBUG: Unable to set %s with an integer value %" PRId64 ".\n", sValue, iValue);
              break;
            case napi_string:
              status = napi_get_value_string_utf8(env, element, nullptr, 0, &sLen);
              PASS_STATUS;
              strProp = (const char*) malloc(sizeof(char) * (sLen + 1));
              PASS_STATUS;
              status = napi_get_value_string_utf8(env, element, (char*) strProp, sLen + 1, &sLen);
              PASS_STATUS;
              ret = av_opt_set(codec->priv_data, sValue, strProp, 0);
              if (ret < 0) printf("DEBUG: Uable to set %s with a string value %s.\n", sValue, strProp);
              // TODO where does strProp get freed
              break;
            default:
              printf("DEBUG: Failed to set a private data value %s\n", sValue);
              break;
          }
        } else {
          printf("DEBUG: Option %s not found.\n", sValue);
        }
        free(sValue);
      }
    }
  }

  status = beam_get_int64(env, props, "bit_rate", &codec->bit_rate);
  PASS_STATUS;
  if (encoding) {
    status = beam_get_int32(env, props, "bit_rate_tolerance", &codec->bit_rate_tolerance);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_get_int32(env, props, "global_quality", &codec->global_quality);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_get_int32(env, props, "compression_level", &codec->compression_level);
    PASS_STATUS;
  }

  status = napi_get_named_property(env, props, "flags", &value);
  PASS_STATUS;
  status = napi_typeof(env, value, &type);
  PASS_STATUS;
  if (type == napi_object) {
    /**
     * Allow decoders to produce frames with data planes that are not aligned
     * to CPU requirements (e.g. due to cropping).
     */
    status = beam_get_bool(env, value, "UNALIGNED", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_UNALIGNED :
      codec->flags & ~AV_CODEC_FLAG_UNALIGNED; }
    /**
     * Use fixed qscale.
     */
    status = beam_get_bool(env, value, "QSCALE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_QSCALE :
      codec->flags & ~AV_CODEC_FLAG_QSCALE; }
    /**
     * 4 MV per MB allowed / advanced prediction for H.263.
     */
    status = beam_get_bool(env, value, "4MV", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_4MV :
      codec->flags & ~AV_CODEC_FLAG_4MV; }
    /**
     * Output even those frames that might be corrupted.
     */
    status = beam_get_bool(env, value, "OUTPUT_CORRUPT", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_OUTPUT_CORRUPT :
      codec->flags & ~AV_CODEC_FLAG_OUTPUT_CORRUPT; }
    /**
     * Use qpel MC.
     */
    status = beam_get_bool(env, value, "QPEL", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_QPEL :
      codec->flags & ~AV_CODEC_FLAG_QPEL; }
    /**
     * Use internal 2pass ratecontrol in first pass mode.
     */
    status = beam_get_bool(env, value, "PASS1", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_PASS1 :
      codec->flags & ~AV_CODEC_FLAG_PASS1; }
    /**
     * Use internal 2pass ratecontrol in second pass mode.
     */
    status = beam_get_bool(env, value, "PASS2", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_PASS2 :
      codec->flags & ~AV_CODEC_FLAG_PASS2; }
    /**
     * loop filter.
     */
    status = beam_get_bool(env, value, "LOOP_FILTER", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_LOOP_FILTER :
      codec->flags & ~AV_CODEC_FLAG_LOOP_FILTER; }
    /**
     * Only decode/encode grayscale.
     */
    status = beam_get_bool(env, value, "GRAY", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_GRAY :
      codec->flags & ~AV_CODEC_FLAG_GRAY; }
    /**
     * error[?] variables will be set during encoding.
     */
    status = beam_get_bool(env, value, "PSNR", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_PSNR:
      codec->flags & ~AV_CODEC_FLAG_PSNR; }
    /**
     * Input bitstream might be truncated at a random location
     * instead of only at frame boundaries.
     */
    status = beam_get_bool(env, value, "TRUNCATED", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_TRUNCATED :
      codec->flags & ~AV_CODEC_FLAG_TRUNCATED; }
    /**
     * Use interlaced DCT.
     */
    status = beam_get_bool(env, value, "INTERLACED_DCT", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_INTERLACED_DCT :
      codec->flags & ~AV_CODEC_FLAG_INTERLACED_DCT; }
    /**
     * Force low delay.
     */
    status = beam_get_bool(env, value, "LOW_DELAY", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_LOW_DELAY :
      codec->flags & ~AV_CODEC_FLAG_LOW_DELAY; }
    /**
     * Place global headers in extradata instead of every keyframe.
     */
    status = beam_get_bool(env, value, "GLOBAL_HEADER", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_GLOBAL_HEADER :
      codec->flags & ~AV_CODEC_FLAG_GLOBAL_HEADER; }
    /**
     * Use only bitexact stuff (except (I)DCT).
     */
    status = beam_get_bool(env, value, "BITEXACT", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_BITEXACT :
      codec->flags & ~AV_CODEC_FLAG_BITEXACT; }
    /* Fx : Flag for H.263+ extra options */
    /**
     * H.263 advanced intra coding / MPEG-4 AC prediction
     */
    status = beam_get_bool(env, value, "AC_PRED", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_AC_PRED :
      codec->flags & ~AV_CODEC_FLAG_AC_PRED; }
    /**
     * interlaced motion estimation
     */
    status = beam_get_bool(env, value, "INTERLACED_ME", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_INTERLACED_ME :
      codec->flags & ~AV_CODEC_FLAG_INTERLACED_ME; }

    status = beam_get_bool(env, value, "CLOSED_GOP", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags = (flag) ?
      codec->flags | AV_CODEC_FLAG_CLOSED_GOP :
      codec->flags & ~AV_CODEC_FLAG_CLOSED_GOP; }
  } // flags

  status = napi_get_named_property(env, props, "flags2", &value);
  PASS_STATUS;
  status = napi_typeof(env, value, &type);
  PASS_STATUS;
  if (type == napi_object) {
    /**
     * Allow non spec compliant speedup tricks.
     */
    status = beam_get_bool(env, value, "FAST", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_FAST :
      codec->flags2 & ~AV_CODEC_FLAG2_FAST; }
    /**
     * Skip bitstream encoding.
     */
    status = beam_get_bool(env, value, "NO_OUTPUT", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_NO_OUTPUT :
      codec->flags2 & ~AV_CODEC_FLAG2_NO_OUTPUT; }
    /**
     * Place global headers at every keyframe instead of in extradata.
     */
    status = beam_get_bool(env, value, "LOCAL_HEADER", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_LOCAL_HEADER :
      codec->flags2 & ~AV_CODEC_FLAG2_LOCAL_HEADER; }
    /**
     * timecode is in drop frame format. DEPRECATED!!!!
     */
    status = beam_get_bool(env, value, "DROP_FRAME_TIMECODE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_DROP_FRAME_TIMECODE :
      codec->flags2 & ~AV_CODEC_FLAG2_DROP_FRAME_TIMECODE; }
    /**
     * Input bitstream might be truncated at a packet boundaries
     * instead of only at frame boundaries.
     */
    status = beam_get_bool(env, value, "CHUNKS", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_CHUNKS :
      codec->flags2 & ~AV_CODEC_FLAG2_CHUNKS; }
    /**
     * Discard cropping information from SPS.
     */
    status = beam_get_bool(env, value, "IGNORE_CROP", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_IGNORE_CROP :
      codec->flags2 & ~AV_CODEC_FLAG2_IGNORE_CROP; }
    /**
     * Show all frames before the first keyframe
     */
    status = beam_get_bool(env, value, "SHOW_ALL", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_SHOW_ALL :
      codec->flags2 & ~AV_CODEC_FLAG2_SHOW_ALL; }
    /**
     * Export motion vectors through frame side data
     */
    status = beam_get_bool(env, value, "EXPORT_MVS", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_EXPORT_MVS :
      codec->flags2 & ~AV_CODEC_FLAG2_EXPORT_MVS; }
    /**
     * Do not skip samples and export skip information as frame side data
     */
    status = beam_get_bool(env, value, "SKIP_MANUAL", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_SKIP_MANUAL :
      codec->flags2 & ~AV_CODEC_FLAG2_SKIP_MANUAL; }
    /**
     * Do not reset ASS ReadOrder field on flush (subtitles decoding)
     */
    status = beam_get_bool(env, value, "RO_FLUSH_NOOP", &present, &flag);
    PASS_STATUS;
    if (present) { codec->flags2 = (flag) ?
      codec->flags2 | AV_CODEC_FLAG2_RO_FLUSH_NOOP :
      codec->flags2 & ~AV_CODEC_FLAG2_RO_FLUSH_NOOP; }
  } // flags2

  if (!encoding) { // TODO memory leak
    status = napi_get_named_property(env, props, "extradata", &value);
    PASS_STATUS;
    status = napi_is_buffer(env, value, &isArray);
    PASS_STATUS;
    if (isArray) {
      status = napi_get_buffer_info(env, value, (void**) &data, &sLen);
      PASS_STATUS;
      codec->extradata_size = sLen;
      codec->extradata = (uint8_t*) malloc(sLen);
      memcpy(codec->extradata, data, sLen);
    }
  }

  if (encoding) {
    status = beam_get_rational(env, props, "time_base", &codec->time_base);
    PASS_STATUS;
    status = beam_get_int32(env, props, "ticks_per_frame", &codec->ticks_per_frame);
    PASS_STATUS;
  }

  if (codec->codec_type == AVMEDIA_TYPE_VIDEO) {
    status = beam_get_int32(env, props, "width", &codec->width);
    PASS_STATUS;
    status = beam_get_int32(env, props, "height", &codec->height);
    PASS_STATUS;
    if (!encoding) {
      status = beam_get_int32(env, props, "coded_width", &codec->coded_width);
      PASS_STATUS;
      status = beam_get_int32(env, props, "coded_height", &codec->coded_height);
      PASS_STATUS;
    }
    status = beam_get_string_utf8(env, props, "pix_fmt", &sValue);
    PASS_STATUS;
    if (sValue != nullptr) {
      codec->pix_fmt =  av_get_pix_fmt((const char *) sValue);
    }
    if (encoding) {
      status = beam_get_int32(env, props, "max_b_frames", &codec->max_b_frames);
      PASS_STATUS;
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "b_quant_factor", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->b_quant_factor = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "b_quant_offset", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->b_quant_offset = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "i_quant_factor", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->i_quant_factor = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "i_quant_offset", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->i_quant_offset = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "lumi_masking", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->lumi_masking = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "temporal_cplx_masking", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->temporal_cplx_masking = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "spatial_cplx_masking", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->spatial_cplx_masking = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "p_masking", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->p_masking = (float) dValue;
      }
    }
    if (encoding) {
      dValue = -87654321.0;
      status = beam_get_double(env, props, "dark_masking", &dValue);
      PASS_STATUS;
      if (dValue != -87654321.0) {
        codec->dark_masking = (float) dValue;
      }
    }
    if (!encoding) {
      status = napi_get_named_property(env, props, "slice_offset", &value);
      PASS_STATUS;
      status = napi_is_array(env, value, &isArray);
      PASS_STATUS;
      if (isArray) {
        status = napi_get_array_length(env, value, (uint32_t*) &codec->slice_count);
        PASS_STATUS;
        codec->slice_offset = (int*) malloc(sizeof(int) * codec->slice_count);
        for ( int x = 0 ; x < codec->slice_count ; x++ ) {
          int sliceOff = 0;
          status = napi_get_element(env, value, x, &element);
          PASS_STATUS;
          status = napi_get_value_int32(env, element, &sliceOff);
          PASS_STATUS;
          codec->slice_offset[x] = sliceOff;
        }
      }
      else {
        status = napi_has_named_property(env, props, "slice_offset", &present);
        PASS_STATUS;
        if (present) {
          codec->slice_count = 0;
          codec->slice_offset = nullptr;
        }
      }
    }
    if (encoding) {
      status = beam_get_rational(env, props, "sample_aspect_ratio", &codec->sample_aspect_ratio);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_enum(env, props, "me_cmp", beam_ff_cmp, &codec->me_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_enum(env, props, "me_sub_cmp", beam_ff_cmp, &codec->me_sub_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_enum(env, props, "mb_cmp", beam_ff_cmp, &codec->mb_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_enum(env, props, "mb_cmp", beam_ff_cmp, &codec->mb_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_enum(env, props, "ildct_cmp", beam_ff_cmp, &codec->dia_size);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "last_predictor_count",
        &codec->last_predictor_count);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_enum(env, props, "me_pre_cmp", beam_ff_cmp, &codec->me_pre_cmp);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "pre_dia_size", &codec->pre_dia_size);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "me_subpel_quality", &codec->me_subpel_quality);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "me_range", &codec->me_range);
      PASS_STATUS;
    }
    if (!encoding) {
      // draw_horiz_band() is called in coded order instead of display
      status = beam_get_bool(env, props, "SLICE_FLAG_CODED_ORDER", &present, &flag);
      PASS_STATUS;
      if (present) { codec->slice_flags = (flag) ?
        codec->slice_flags | SLICE_FLAG_CODED_ORDER :
        codec->slice_flags & ~SLICE_FLAG_CODED_ORDER ; }
      // allow draw_horiz_band() with field slices (MPEG-2 field pics)
      status = beam_get_bool(env, props, "SLICE_FLAG_ALLOW_FIELD", &present, &flag);
      PASS_STATUS;
      if (present) { codec->slice_flags = (flag) ?
        codec->slice_flags | SLICE_FLAG_ALLOW_FIELD :
        codec->slice_flags & ~SLICE_FLAG_ALLOW_FIELD ; }
      // allow draw_horiz_band() with 1 component at a time (SVQ1)
      status = beam_get_bool(env, props, "SLICE_FLAG_ALLOW_PLANE", &present, &flag);
      PASS_STATUS;
      if (present) { codec->slice_flags = (flag) ?
        codec->slice_flags | SLICE_FLAG_ALLOW_PLANE :
        codec->slice_flags & ~SLICE_FLAG_ALLOW_PLANE ; }
    }

    if (encoding) {
      status = beam_get_enum(env, props, "mb_decision", beam_ff_mb_decision, &codec->mb_decision);
      PASS_STATUS;
    }

    if (encoding) {
      status = napi_get_named_property(env, props, "intra_matrix", &value);
      PASS_STATUS;
      status = napi_is_array(env, value, &isArray);
      PASS_STATUS;
      if (isArray) {
        codec->intra_matrix = (uint16_t*) av_mallocz(sizeof(uint16_t) * 64);
        for ( int x = 0 ; x < 64 ; x++ ) {
          status = napi_get_element(env, value, x, &element);
          PASS_STATUS;
          status = napi_typeof(env, element, &type);
          PASS_STATUS;
          if (type == napi_number) {
            status = napi_get_value_uint32(env, element, &uThirtwo);
            PASS_STATUS;
            codec->intra_matrix[x] = (uint16_t) uThirtwo;
          } else {
            codec->intra_matrix[x] = 0;
          }
        }
      }
    } else {
      status = napi_has_named_property(env, props, "intra_matrix", &present);
      PASS_STATUS;
      if (present) {
        codec->intra_matrix = nullptr;
      }
    }

    if (encoding) {
      status = napi_get_named_property(env, props, "inter_matrix", &value);
      PASS_STATUS;
      status = napi_is_array(env, value, &isArray);
      PASS_STATUS;
      if (isArray) {
        codec->inter_matrix = (uint16_t*) av_mallocz(sizeof(uint16_t) * 64);
        for ( int x = 0 ; x < 64 ; x++ ) {
          status = napi_get_element(env, value, x, &element);
          PASS_STATUS;
          status = napi_typeof(env, element, &type);
          PASS_STATUS;
          if (type == napi_number) {
            status = napi_get_value_uint32(env, element, &uThirtwo);
            PASS_STATUS;
            codec->inter_matrix[x] = (uint16_t) uThirtwo;
          } else {
            codec->inter_matrix[x] = 0;
          }
        }
      }
    } else {
      status = napi_has_named_property(env, props, "inter_matrix", &present);
      PASS_STATUS;
      if (present) {
        codec->inter_matrix = nullptr;
      }
    }

    if (encoding) {
      status = beam_get_int32(env, props, "intra_dc_precision", &codec->intra_dc_precision);
      PASS_STATUS;
    }
    if (!encoding) {
      status = beam_get_int32(env, props, "skip_top", &codec->skip_top);
      PASS_STATUS;
      status = beam_get_int32(env, props, "skip_bottom", &codec->skip_bottom);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "mb_lmin", &codec->mb_lmin);
      PASS_STATUS;
      status = beam_get_int32(env, props, "mb_lmax", &codec->mb_lmax);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "bidir_refine", &codec->bidir_refine);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "keyint_min", &codec->keyint_min);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "refs", &codec->refs);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_int32(env, props, "mv0_threshold", &codec->mv0_threshold);
      PASS_STATUS;
    }
    if (encoding) {
      status = beam_get_string_utf8(env, props, "color_primaries", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->color_primaries = (AVColorPrimaries) av_color_primaries_from_name(sValue);
      }
    }
    if (encoding) {
      status = beam_get_string_utf8(env, props, "colour_trc", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->color_trc = (AVColorTransferCharacteristic) av_color_transfer_from_name(sValue);
      }
    }
    if (encoding) {
      status = beam_get_string_utf8(env, props, "colorspace", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->colorspace = (AVColorSpace) av_color_space_from_name(sValue);
      }
    }
    if (encoding) {
      status = beam_get_string_utf8(env, props, "color_range", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->color_range = (AVColorRange) av_color_range_from_name(sValue);
      }
    }
    if (encoding) {
      status = beam_get_string_utf8(env, props, "chroma_sample_location", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->chroma_sample_location = (AVChromaLocation) av_chroma_location_from_name(sValue);
      }
    }
    if (encoding) {
      status = beam_get_int32(env, props, "slices", &codec->slices);
      PASS_STATUS;
    }
    status = beam_get_enum(env, props, "field_order", beam_field_order,
      (int*) &codec->field_order);
    PASS_STATUS;

  } // Video-only parameters

  if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    status = beam_get_int32(env, props, "sample_rate", &codec->sample_rate);
    PASS_STATUS;
    status = beam_get_int32(env, props, "channels", &codec->channels);
    PASS_STATUS;

    if (encoding) {
      status = beam_get_string_utf8(env, props, "sample_fmt", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->sample_fmt = av_get_sample_fmt(sValue);
      }
    }
    status = beam_get_int32(env, props, "block_align", &codec->block_align);
    PASS_STATUS;
    if (encoding) {
      status = beam_get_int32(env, props, "cutoff", &codec->cutoff);
      PASS_STATUS;
    }
    status = beam_get_string_utf8(env, props, "channel_layout", &sValue);
    PASS_STATUS;
    if (sValue != nullptr) {
      codec->channel_layout = av_get_channel_layout(sValue);
    }
    if (!encoding) {
      status = beam_get_string_utf8(env, props, "request_channel_layout", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->request_channel_layout = av_get_channel_layout(sValue);
      }
    }
    if (encoding) {
      status = beam_get_enum(env, props, "audio_service_type",
        beam_av_audio_service_type, (int*) &codec->audio_service_type);
      PASS_STATUS;
    }
    if (!encoding) {
      status = beam_get_string_utf8(env, props, "request_sample_fmt", &sValue);
      PASS_STATUS;
      if (sValue != nullptr) {
        codec->request_sample_fmt = av_get_sample_fmt(sValue);
      }
    }
  } // Audio-only parameters

  // Encoding Parameters
  if (encoding) {
    dValue = 8765432.1;
    status = beam_get_double(env, props, "qcompress", &dValue);
    PASS_STATUS;
    if (dValue != 8765432.1) {
      codec->qcompress = (float) dValue;
    }
    dValue = 8765432.1;
    status = beam_get_double(env, props, "qblur", &dValue);
    PASS_STATUS;
    if (dValue != 8765432.1) {
      codec->qblur = (float) dValue;
    }
  }
  if (encoding) {
    status = beam_get_int32(env, props, "qmin", &codec->qmin);
    PASS_STATUS;
    status = beam_get_int32(env, props, "qmax", &codec->qmax);
    PASS_STATUS;
    status = beam_get_int32(env, props, "max_qdiff", &codec->max_qdiff);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_get_int32(env, props, "rc_buffer_size", &codec->rc_buffer_size);
    PASS_STATUS;
  }
  status = beam_get_int64(env, props, "rc_max_rate", &codec->rc_max_rate);
  PASS_STATUS;
  if (encoding) {
    status = beam_get_int64(env, props, "rc_min_rate", &codec->rc_min_rate);
    PASS_STATUS;
    dValue = 8765432.1;
    status = beam_get_double(env, props, "rc_max_available_vbv_use", &dValue);
    PASS_STATUS;
    if (dValue != 8765432.1) {
      codec->rc_max_available_vbv_use = (float) dValue;
    }
    dValue = 8765432.1;
    status = beam_get_double(env, props, "rc_min_vbv_overflow_use", &dValue);
    PASS_STATUS;
    if (dValue != 8765432.1) {
      codec->rc_min_vbv_overflow_use = (float) dValue;
    }
    status = beam_get_int32(env, props, "rc_initial_buffer_occupancy",
      &codec->rc_initial_buffer_occupancy);
    PASS_STATUS;
  }
  if (encoding) {
    status = napi_get_named_property(env, props, "rc_override", &value);
    PASS_STATUS;
    status = napi_is_array(env, value, &isArray);
    PASS_STATUS;
    if (isArray) {
      status = napi_get_array_length(env, value, (uint32_t*) &codec->rc_override_count);
      PASS_STATUS;
      // TODO should find a way to free this
      codec->rc_override = (RcOverride *) av_realloc(codec->rc_override,
          sizeof(RcOverride) * codec->rc_override_count);
      for ( int x = 0 ; x < codec->rc_override_count ; x++ ) {
        status = napi_get_element(env, value, x, &element);
        PASS_STATUS;
        status = beam_get_int32(env, element, "start_frame",
          &codec->rc_override[x].start_frame);
        PASS_STATUS;
        status = beam_get_int32(env, element, "end_frame",
          &codec->rc_override[x].end_frame);
        PASS_STATUS;
        status = beam_get_int32(env, element, "qscale",
          &codec->rc_override[x].qscale);
        PASS_STATUS;
        dValue = 8765432.1;
        status = beam_get_double(env, element, "quality_factor", &dValue);
        PASS_STATUS;
        if (dValue != 8765432.1) {
          codec->rc_override[x].quality_factor = (float) dValue;
        }
      } // rc_override array
    } // encoding
  }

  if (encoding) {
    status = beam_get_int32(env, props, "trellis", &codec->trellis);
    PASS_STATUS;
  }

  status = napi_get_named_property(env, props, "workaround_bugs", &value);
  PASS_STATUS;
  status = napi_typeof(env, value, &type);
  PASS_STATUS;
  if (type == napi_object) {
    status = beam_get_bool(env, value, "AUTODETECT", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_AUTODETECT :
      codec->workaround_bugs & ~FF_BUG_AUTODETECT; }
    status = beam_get_bool(env, value, "XVID_ILACE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_XVID_ILACE :
      codec->workaround_bugs & ~FF_BUG_XVID_ILACE; }
    status = beam_get_bool(env, value, "UMP4", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_UMP4 :
      codec->workaround_bugs & ~FF_BUG_UMP4; }
    status = beam_get_bool(env, value, "NO_PADDING", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_NO_PADDING :
      codec->workaround_bugs & ~FF_BUG_NO_PADDING; }
    status = beam_get_bool(env, value, "AMV", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_AMV :
      codec->workaround_bugs & ~FF_BUG_AMV; }
    status = beam_get_bool(env, value, "QPEL_CHROMA", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_QPEL_CHROMA :
      codec->workaround_bugs & ~FF_BUG_QPEL_CHROMA; }
    status = beam_get_bool(env, value, "STD_QPEL", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_STD_QPEL :
      codec->workaround_bugs & ~FF_BUG_STD_QPEL; }
    status = beam_get_bool(env, value, "QPEL_CHROMA2", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_QPEL_CHROMA2 :
      codec->workaround_bugs & ~FF_BUG_QPEL_CHROMA2; }
    status = beam_get_bool(env, value, "DIRECT_BLOCKSIZE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_DIRECT_BLOCKSIZE :
      codec->workaround_bugs & ~FF_BUG_DIRECT_BLOCKSIZE; }
    status = beam_get_bool(env, value, "EDGE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_EDGE :
      codec->workaround_bugs & ~FF_BUG_EDGE; }
    status = beam_get_bool(env, value, "HPEL_CHROMA", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_HPEL_CHROMA :
      codec->workaround_bugs & ~FF_BUG_HPEL_CHROMA; }
    status = beam_get_bool(env, value, "DC_CLIP", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_DC_CLIP :
      codec->workaround_bugs & ~FF_BUG_DC_CLIP; }
    status = beam_get_bool(env, value, "MS", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_MS :
      codec->workaround_bugs & ~FF_BUG_MS; }
    status = beam_get_bool(env, value, "TRUNCATED", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_TRUNCATED :
      codec->workaround_bugs & ~FF_BUG_TRUNCATED; }
    status = beam_get_bool(env, value, "IEDGE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->workaround_bugs = (flag) ?
      codec->workaround_bugs | FF_BUG_IEDGE :
      codec->workaround_bugs & ~FF_BUG_IEDGE; }
  } // workaround_bugs sub object

  status = beam_get_enum(env, props, "strict_std_compliance", beam_ff_compliance,
    &codec->strict_std_compliance);
  PASS_STATUS;

  if (!encoding) {
    status = napi_get_named_property(env, props, "error_concealment", &value);
    PASS_STATUS;
    status = napi_typeof(env, value, &type);
    PASS_STATUS;
    if (type == napi_object) {
      status = beam_get_bool(env, value, "GUESS_MVS", &present, &flag);
      PASS_STATUS;
      if (present) { codec->error_concealment = (flag) ?
        codec->error_concealment | FF_EC_GUESS_MVS :
        codec->error_concealment & ~FF_EC_GUESS_MVS; }
      status = beam_get_bool(env, value, "DEBLOCK", &present, &flag);
      PASS_STATUS;
      if (present) { codec->error_concealment = (flag) ?
        codec->error_concealment | FF_EC_DEBLOCK :
        codec->error_concealment & ~FF_EC_DEBLOCK; }
      status = beam_get_bool(env, value, "FAVOR_INTER", &present, &flag);
      PASS_STATUS;
      if (present) { codec->error_concealment = (flag) ?
        codec->error_concealment | FF_EC_FAVOR_INTER :
        codec->error_concealment & ~FF_EC_FAVOR_INTER; }
    }
  } // error-concealment

  status = napi_get_named_property(env, props, "debug", &value);
  PASS_STATUS;
  status = napi_typeof(env, value, &type);
  PASS_STATUS;
  if (type == napi_object) {
    status = beam_get_bool(env, value, "PICT_INFO", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_PICT_INFO :
      codec->debug & ~FF_DEBUG_PICT_INFO; }
    status = beam_get_bool(env, value, "RC", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_RC :
      codec->debug & ~FF_DEBUG_RC; }
    status = beam_get_bool(env, value, "BITSTREAM", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_BITSTREAM :
      codec->debug & ~FF_DEBUG_BITSTREAM; }
    status = beam_get_bool(env, value, "MB_TYPE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_MB_TYPE :
      codec->debug & ~FF_DEBUG_MB_TYPE; }
    status = beam_get_bool(env, value, "QP", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_QP :
      codec->debug & ~FF_DEBUG_QP; }
    status = beam_get_bool(env, value, "DCT_COEFF", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_DCT_COEFF :
      codec->debug & ~FF_DEBUG_DCT_COEFF; }
    status = beam_get_bool(env, value, "SKIP", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_SKIP :
      codec->debug & ~FF_DEBUG_SKIP; }
    status = beam_get_bool(env, value, "STARTCODE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_STARTCODE :
      codec->debug & ~FF_DEBUG_STARTCODE; }
    status = beam_get_bool(env, value, "ER", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_ER :
      codec->debug & ~FF_DEBUG_ER; }
    status = beam_get_bool(env, value, "MMCO", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_MMCO :
      codec->debug & ~FF_DEBUG_MMCO; }
    status = beam_get_bool(env, value, "BUGS", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_BUGS :
      codec->debug & ~FF_DEBUG_BUGS; }
  #if FF_API_DEBUG_MV
    status = beam_get_bool(env, value, "VIS_QP", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_VIS_QP :
      codec->debug & ~FF_DEBUG_VIS_QP; }
    status = beam_get_bool(env, value, "VIS_MB_TYPE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_VIS_MB_TYPE :
      codec->debug & ~FF_DEBUG_VIS_MB_TYPE; }
  #endif
    status = beam_get_bool(env, value, "BUFFERS", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_BUFFERS :
      codec->debug & ~FF_DEBUG_BUFFERS; }
    status = beam_get_bool(env, value, "THREADS", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_THREADS :
      codec->debug & ~FF_DEBUG_THREADS; }
    status = beam_get_bool(env, value, "GREEN_MD", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_GREEN_MD :
      codec->debug & ~FF_DEBUG_GREEN_MD; }
    status = beam_get_bool(env, value, "NOMC", &present, &flag);
    PASS_STATUS;
    if (present) { codec->debug = (flag) ?
      codec->debug | FF_DEBUG_NOMC :
      codec->debug & ~FF_DEBUG_NOMC; }
  } // debug flags

  if (!encoding) {
    status = napi_get_named_property(env, props, "err_recognition", &value);
    PASS_STATUS;
    status = napi_typeof(env, value, &type);
    PASS_STATUS;
    if (type == napi_object) {
      status = beam_get_bool(env, value, "CRCCHECK", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_CRCCHECK :
        codec->err_recognition & ~AV_EF_CRCCHECK; }
      status = beam_get_bool(env, value, "BITSTREAM", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_BITSTREAM :
        codec->err_recognition & ~AV_EF_BITSTREAM; }
      status = beam_get_bool(env, value, "BUFFER", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_BUFFER :
        codec->err_recognition & ~AV_EF_BUFFER; }
      status = beam_get_bool(env, value, "EXPLODE", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_EXPLODE :
        codec->err_recognition & ~AV_EF_EXPLODE; }

      status = beam_get_bool(env, value, "IGNORE_ERR", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_IGNORE_ERR :
        codec->err_recognition & ~AV_EF_IGNORE_ERR; }
      status = beam_get_bool(env, value, "CAREFUL", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_CAREFUL :
        codec->err_recognition & ~AV_EF_CAREFUL; }
      status = beam_get_bool(env, value, "COMPLIANT", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_COMPLIANT :
        codec->err_recognition & ~AV_EF_COMPLIANT; }
      status = beam_get_bool(env, value, "AGGRESSIVE", &present, &flag);
      PASS_STATUS;
      if (present) { codec->err_recognition = (flag) ?
        codec->err_recognition | AV_EF_AGGRESSIVE :
        codec->err_recognition & ~AV_EF_AGGRESSIVE; }
    }
  } // err_recognition

  if (!encoding) {
    status = beam_get_int64(env, props, "reordered_opaque", &codec->reordered_opaque);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_get_enum(env, props, "dct_algo", beam_ff_dct, &codec->dct_algo);
    PASS_STATUS;
  }
  status = beam_get_enum(env, props, "idct_algo", beam_ff_idct, &codec->idct_algo);
  PASS_STATUS;
  if (!encoding) {
    status = beam_get_int32(env, props, "bits_per_coded_sample", &codec->bits_per_coded_sample);
    PASS_STATUS;
  }
  if (encoding) {
    status = beam_get_int32(env, props, "bits_per_raw_sample", &codec->bits_per_raw_sample);
    PASS_STATUS;
  }
  status = beam_get_int32(env, props, "thread_count", &codec->thread_count);
  PASS_STATUS;

  status = napi_get_named_property(env, props, "thread_type", &value);
  PASS_STATUS;
  status = napi_typeof(env, value, &type);
  PASS_STATUS;
  if (type == napi_object) {
    status = beam_get_bool(env, value, "FRAME", &present, &flag);
    PASS_STATUS;
    if (present) { codec->thread_type = (flag) ?
      codec->thread_type | FF_THREAD_FRAME :
      codec->thread_type & ~FF_THREAD_FRAME; }
    status = beam_get_bool(env, value, "SLICE", &present, &flag);
    PASS_STATUS;
    if (present) { codec->thread_type = (flag) ?
      codec->thread_type | FF_THREAD_SLICE :
      codec->thread_type & ~FF_THREAD_SLICE; }
  }

  if (encoding) {
    status = beam_get_int32(env, props, "nsse_weight", &codec->nsse_weight);
    PASS_STATUS;
  }

  status = beam_get_string_utf8(env, props, "profile", &sValue);
  PASS_STATUS;
  if (sValue != nullptr) {
    const AVProfile* profile = codec->codec->profiles;
    while (profile->profile != FF_PROFILE_UNKNOWN) {
      if (strcmp(sValue, profile->name) == 0) {
        codec->profile = profile->profile;
        break;
      }
      profile = profile + 1;
    }
  }

  if (encoding) {
    status = beam_get_int32(env, props, "level", &codec->level);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_get_enum(env, props, "skip_loop_filter", beam_avdiscard, (int*) &codec->skip_loop_filter);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_get_enum(env, props, "skip_idct", beam_avdiscard, (int*) &codec->skip_idct);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_get_enum(env, props, "skip_frame", beam_avdiscard, (int*) &codec->skip_frame);
    PASS_STATUS;
  }

  if (encoding) { // TODO memory leak - freeing the allocated header block
    status = napi_get_named_property(env, props, "subtitle_header", &value);
    PASS_STATUS;
    status = napi_is_buffer(env, value, &isArray);
    PASS_STATUS;
    if (isArray) {
      status = napi_get_buffer_info(env, value, (void**) &data, &sLen);
      PASS_STATUS;
      codec->subtitle_header_size = sLen;
      codec->subtitle_header = (uint8_t*) malloc(sLen);
      memcpy(codec->subtitle_header, data, sLen);
    }
  }

  status = beam_get_rational(env, props, "framerate", &codec->framerate);
  PASS_STATUS;
  if (!encoding) {
    status = beam_get_rational(env, props, "pkt_format", &codec->pkt_timebase);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_get_string_utf8(env, props, "sub_charenc", &codec->sub_charenc);
    PASS_STATUS;
  }
  if (!encoding) {
    status = beam_get_int32(env, props, "skip_alpha", &codec->skip_alpha);
    PASS_STATUS;
  }

  if (encoding) {
    status = napi_get_named_property(env, props, "chroma_intra_matrix", &value);
    PASS_STATUS;
    status = napi_is_array(env, value, &isArray);
    PASS_STATUS;
    if (isArray) {
      codec->chroma_intra_matrix = (uint16_t*) av_mallocz(sizeof(uint16_t) * 64);
      for ( int x = 0 ; x < 64 ; x++ ) {
        status = napi_get_element(env, value, x, &element);
        PASS_STATUS;
        status = napi_typeof(env, element, &type);
        PASS_STATUS;
        if (type == napi_number) {
          status = napi_get_value_uint32(env, element, &uThirtwo);
          PASS_STATUS;
          codec->chroma_intra_matrix[x] = (uint16_t) uThirtwo;
        } else {
          codec->chroma_intra_matrix[x] = 0;
        }
      }
    }
  }

  status = beam_get_string_utf8(env, props, "dump_separator", (char**) &codec->dump_separator);
  PASS_STATUS;
  // TODO hw_frames_ctx
  if (!encoding) { // Exposing as an integer - perhpas change to enum
    status = beam_get_int32(env, props, "sub_text_format", &codec->sub_text_format);
    PASS_STATUS;
  }
  if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    status = beam_get_int32(env, props, "trailing_padding", &codec->trailing_padding);
    PASS_STATUS;
  }
  status = beam_get_int64(env, props, "max_pixels", &codec->max_pixels);
  PASS_STATUS;
  // TODO hw_device_ctx

  if (!encoding) {
    status = napi_get_named_property(env, props, "hwaccel_flags", &value);
    PASS_STATUS;
    status = napi_typeof(env, value, &type);
    PASS_STATUS;
    if (type == napi_object) {
      status = beam_get_bool(env, value, "IGNORE_LEVEL", &present, &flag);
      PASS_STATUS;
      if (present) { codec->hwaccel_flags = (flag) ?
        codec->hwaccel_flags | AV_HWACCEL_FLAG_IGNORE_LEVEL :
        codec->hwaccel_flags & ~AV_HWACCEL_FLAG_IGNORE_LEVEL; }
      status = beam_get_bool(env, value, "ALLOW_HIGH_DEPTH", &present, &flag);
      PASS_STATUS;
      if (present) { codec->hwaccel_flags = (flag) ?
        codec->hwaccel_flags | AV_HWACCEL_FLAG_ALLOW_HIGH_DEPTH :
        codec->hwaccel_flags & ~AV_HWACCEL_FLAG_ALLOW_HIGH_DEPTH; }
      status = beam_get_bool(env, value, "ALLOW_PROFILE_MISMATCH", &present, &flag);
      PASS_STATUS;
      if (present) { codec->hwaccel_flags = (flag) ?
        codec->hwaccel_flags | AV_HWACCEL_FLAG_ALLOW_PROFILE_MISMATCH :
        codec->hwaccel_flags & ~AV_HWACCEL_FLAG_ALLOW_PROFILE_MISMATCH; }
    }
  } // hwaccel_flags
  // TODO coded side data

  if ((!encoding) && (codec->codec_type == AVMEDIA_TYPE_VIDEO)) {
    status = beam_get_bool(env, props, "apply_cropping", &present, &flag);
    PASS_STATUS;
    if (present) { codec->apply_cropping = (flag) ? 1 : 0; }
    status = beam_get_int32(env, props, "extra_hw_frames", &codec->extra_hw_frames);
    PASS_STATUS;
  }

  return napi_ok;
};

napi_status getPropsFromPacket(napi_env env, napi_value target, AVPacket* packet) {
  napi_status status;

  if (packet->pts != AV_NOPTS_VALUE) {
    status = beam_set_int64(env, target, "pts", packet->pts);
    PASS_STATUS;
  } else {
    status = beam_set_null(env, target, "pts");
    PASS_STATUS;
  }
  status = beam_set_int64(env, target, "dts", packet->dts);
  PASS_STATUS;
  // TODO DATA here
  status = beam_set_int32(env, target, "size", packet->size);
  PASS_STATUS;
  status = beam_set_int32(env, target, "stream_index", packet->size);
  PASS_STATUS;
  status = beam_set_int64(env, target, "duration", packet->duration);
  PASS_STATUS;
  status = beam_set_int64(env, target, "pos", packet->pos);
  PASS_STATUS;

  return napi_ok;
};

napi_status setPacketFromProps(napi_env env, AVPacket* packet, napi_value props) {
  napi_status status;
  bool isNull;

  status = beam_is_null(env, props, "pts", &isNull);
  PASS_STATUS;
  if (isNull) {
    packet->pts = AV_NOPTS_VALUE;
  } else {
    status = beam_get_int64(env, props, "pts", &packet->pts);
    PASS_STATUS;
  }
  status = beam_get_int64(env, props, "dts", &packet->dts);
  PASS_STATUS;
  status = beam_get_int32(env, props, "stream_index", &packet->stream_index);
  PASS_STATUS;
  status = beam_get_int64(env, props, "duration", &packet->duration);
  PASS_STATUS;
  status = beam_get_int64(env, props, "pos", &packet->pos);
  PASS_STATUS;

  return napi_ok;
};

napi_status getPropsFromFrame(napi_env env, napi_value value, AVFrame* frame) {
  napi_status status;

  return napi_ok;
};

napi_status setFrameFromProps(napi_env env, AVFrame* frame, napi_value params) {
  napi_status status;

  return napi_ok;
};

napi_value makePacket(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, value;
  napi_valuetype type;
  bool isArray;
  AVPacket* packet = av_packet_alloc();

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Packet must be created with exactly one options object.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Cannot create a packet without an options object.");
  }

  status = setPacketFromProps(env, packet, args[0]);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  status = beam_set_string_utf8(env, result, "type", "Packet");
  CHECK_STATUS;

  status = getPropsFromPacket(env, result, packet);
  CHECK_STATUS;

  status = napi_create_external(env, packet, packetFinalizer, nullptr, &value);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "_packet", value);
  CHECK_STATUS;

  return result;
}



void packetFinalizer(napi_env env, void* data, void* hint) {
  AVPacket* pkt = (AVPacket*) data;
  av_packet_free(&pkt);
}

napi_status beam_set_uint32(napi_env env, napi_value target, char* name, uint32_t value) {
  napi_status status;
  napi_value prop;
  status = napi_create_uint32(env, value, &prop);
  PASS_STATUS;
  return napi_set_named_property(env, target, name, prop);
};

napi_status beam_get_uint32(napi_env env, napi_value target, char* name, uint32_t* value) {
  napi_status status;
  napi_value prop;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_uint32(env, prop, value);
  ACCEPT_STATUS(napi_number_expected);
  return napi_ok;
}

napi_status beam_set_int32(napi_env env, napi_value target, char* name, int32_t value) {
  napi_status status;
  napi_value prop;
  status = napi_create_int32(env, value, &prop);
  PASS_STATUS;
  return napi_set_named_property(env, target, name, prop);
}

napi_status beam_get_int32(napi_env env, napi_value target, char* name, int32_t* value) {
  napi_status status;
  napi_value prop;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_int32(env, prop, value);
  ACCEPT_STATUS(napi_number_expected);
  return napi_ok;
}

napi_status beam_set_int64(napi_env env, napi_value target, char* name, int64_t value) {
  napi_status status;
  napi_value prop;
  status = napi_create_int64(env, value, &prop);
  PASS_STATUS;
  return napi_set_named_property(env, target, name, prop);
}

napi_status beam_get_int64(napi_env env, napi_value target, char* name, int64_t* value) {
  napi_status status;
  napi_value prop;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_int64(env, prop, value);
  ACCEPT_STATUS(napi_number_expected);
  return napi_ok;
}

napi_status beam_set_double(napi_env env, napi_value target, char* name, double value) {
  napi_status status;
  napi_value prop;
  status = napi_create_double(env, value, &prop);
  PASS_STATUS;
  return napi_set_named_property(env, target, name, prop);
}

napi_status beam_get_double(napi_env env, napi_value target, char* name, double* value) {
  napi_status status;
  napi_value prop;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_double(env, prop, value);
  ACCEPT_STATUS(napi_number_expected);
  return napi_ok;
}

napi_status beam_set_string_utf8(napi_env env, napi_value target, char* name, char* value) {
  napi_status status;
  napi_value prop;
  if (value == nullptr) {
    status = napi_get_null(env, &prop);
  }
  else {
    status = napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &prop);
  }
  ACCEPT_STATUS(napi_string_expected);
  return napi_set_named_property(env, target, name, prop);
}

// TODO improve memory management
napi_status beam_get_string_utf8(napi_env env, napi_value target, char* name, char** value) {
  napi_status status;
  napi_value prop;
  char* str;
  size_t len;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_string_utf8(env, prop, nullptr, 0, &len);
  if (status == napi_string_expected) {
    *value = nullptr;
    return napi_ok;
  }
  PASS_STATUS;
  str = (char*) malloc(sizeof(char) * (len + 1));
  status = napi_get_value_string_utf8(env, prop, str, len + 1, &len);
  PASS_STATUS;
  *value = str;
  return napi_ok;
}

napi_status beam_set_bool(napi_env env, napi_value target, char* name, bool value) {
  napi_status status;
  napi_value prop;
  status = napi_get_boolean(env, value, &prop);
  PASS_STATUS;
  return napi_set_named_property(env, target, name, prop);
}

napi_status beam_get_bool(napi_env env, napi_value target, char* name, bool* present, bool* value) {
  napi_status status;
  napi_value prop;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_bool(env, prop, value);
  if (status == napi_boolean_expected) {
    *present = false;
    status = napi_ok;
  }
  else {
    *present = true;
  }
  PASS_STATUS;
  return napi_ok;
}

napi_status beam_set_rational(napi_env env, napi_value target, char* name, AVRational value) {
  napi_status status;
  napi_value pair, element;
  status = napi_create_array(env, &pair);
  PASS_STATUS;
  status = napi_create_int32(env, value.num, &element);
  PASS_STATUS;
  status = napi_set_element(env, pair, 0, element);
  PASS_STATUS;
  status = napi_create_int32(env, value.den, &element);
  PASS_STATUS;
  status = napi_set_element(env, pair, 1, element);
  PASS_STATUS;
  return napi_set_named_property(env, target, name, pair);
}

napi_status beam_get_rational(napi_env env, napi_value target, char* name, AVRational* value) {
  napi_status status;
  napi_value prop, element;
  int32_t num = 0, den = 1;
  bool isArray;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_is_array(env, prop, &isArray);
  PASS_STATUS;
  if (isArray) {
    status = napi_get_element(env, prop, 0, &element);
    PASS_STATUS;
    status = napi_get_value_int32(env, element, &num);
    ACCEPT_STATUS(napi_number_expected);

    status = napi_get_element(env, prop, 1, &element);
    PASS_STATUS;
    status = napi_get_value_int32(env, element, &den);
    ACCEPT_STATUS(napi_number_expected);
  }
  *value = av_make_q(num, den);
  return napi_ok;
}

napi_status beam_set_null(napi_env env, napi_value target, char* name) {
  napi_status status;
  napi_value value;
  status = napi_get_null(env, &value);
  PASS_STATUS;
  status = napi_set_named_property(env, target, name, value);
  PASS_STATUS;

  return napi_ok;
};

napi_status beam_is_null(napi_env env, napi_value props, char* name, bool* isNull) {
  napi_status status;
  napi_value value;
  napi_valuetype type;
  status = napi_get_named_property(env, props, name, &value);
  PASS_STATUS;
  status = napi_typeof(env, props, &type);
  PASS_STATUS;
  *isNull = (type == napi_null);

  return napi_ok;
};

const char* beam_lookup_name(std::unordered_map<int, std::string> m, int value) {
  auto search = m.find(value);
  if (search != m.end()) {
    return strdup(search->second.c_str());
  } else {
    return "unknown";
  }
}

int beam_lookup_enum(std::unordered_map<std::string, int> m, char* value) {
  auto search = m.find(std::string(value));
  if (search != m.end()) {
    return search->second;
  } else {
    return BEAM_ENUM_UNKNOWN;
  }
}

napi_status beam_set_enum(napi_env env, napi_value target, char* name,
    const beamEnum* enumDesc, int value) {
  napi_status status;
  napi_value prop;
  auto search = enumDesc->forward.find(value);
  if (search != enumDesc->forward.end()) {
    status = napi_create_string_utf8(env, search->second.data(), NAPI_AUTO_LENGTH, &prop);
  } else {
    status = napi_create_string_utf8(env, "unknown", NAPI_AUTO_LENGTH, &prop);
  }
  PASS_STATUS;
  return napi_set_named_property(env, target, name, prop);
};

napi_status beam_get_enum(napi_env env, napi_value target, char* name,
    const beamEnum* enumDesc, int* value) {
  napi_status status;
  napi_value prop;
  napi_valuetype type;
  char* enumStr;
  size_t len;

  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_typeof(env, prop, &type);
  PASS_STATUS;
  if (type == napi_number) {
    status = napi_get_value_int32(env, prop, value);
    PASS_STATUS;
    return napi_ok;
  }
  if (type == napi_string) {
    status = napi_get_value_string_utf8(env, prop, nullptr, 0, &len);
    PASS_STATUS;
    enumStr = (char*) malloc((len + 1) * sizeof(char));
    status = napi_get_value_string_utf8(env, prop, enumStr, len + 1, &len);
    PASS_STATUS;
    auto search = enumDesc->inverse.find(std::string(enumStr));
    if (search != enumDesc->inverse.end()) {
      *value = search->second;
    } else {
      *value = BEAM_ENUM_UNKNOWN;
    }
  }
  return napi_ok;
};

std::unordered_map<int, std::string> beam_field_order_fmap = {
  { AV_FIELD_PROGRESSIVE, "progressive" },
  { AV_FIELD_TT, "top coded first, top displayed first" },
  { AV_FIELD_BB, "bottom coded first, bottom displayed first" },
  { AV_FIELD_TB, "top coded first, bottom displayed first" },
  { AV_FIELD_BT, "bottom coded first, top displayed first" },
  { AV_FIELD_UNKNOWN, "unknown" }
};
const beamEnum* beam_field_order = new beamEnum(beam_field_order_fmap);

std::unordered_map<int, std::string> beam_ff_cmp_fmap = {
  { FF_CMP_SAD, "sad" },
  { FF_CMP_SSE, "sse" },
  { FF_CMP_SATD, "satd" },
  { FF_CMP_DCT, "dct" },
  { FF_CMP_PSNR, "psnr" },
  { FF_CMP_BIT, "bit" },
  { FF_CMP_RD, "rd" },
  { FF_CMP_ZERO, "zero" },
  { FF_CMP_VSAD, "vsad" },
  { FF_CMP_VSSE, "vsse" },
  { FF_CMP_NSSE, "nsse" },
  { FF_CMP_W53, "w53" },
  { FF_CMP_W97, "w97" },
  { FF_CMP_DCTMAX, "dctmax" },
  { FF_CMP_DCT264, "dct264" },
  { FF_CMP_MEDIAN_SAD, "median_sad" },
  { FF_CMP_CHROMA, "chroma" },
};
const beamEnum* beam_ff_cmp = new beamEnum(beam_ff_cmp_fmap);

std::unordered_map<int, std::string> beam_ff_mb_decision_fmap = {
  { FF_MB_DECISION_SIMPLE, "simple" }, //  uses mb_cmp
  { FF_MB_DECISION_BITS, "bits" }, // chooses the one which needs the fewest bits
  { FF_MB_DECISION_RD, "rd" } // rate distortion
};
const beamEnum* beam_ff_mb_decision = new beamEnum(beam_ff_mb_decision_fmap);

std::unordered_map<int, std::string> beam_av_audio_service_type_fmap = {
  { AV_AUDIO_SERVICE_TYPE_MAIN, "main" },
  { AV_AUDIO_SERVICE_TYPE_EFFECTS, "effects" },
  { AV_AUDIO_SERVICE_TYPE_VISUALLY_IMPAIRED, "visually-impaired" },
  { AV_AUDIO_SERVICE_TYPE_HEARING_IMPAIRED, "hearing-impaired" },
  { AV_AUDIO_SERVICE_TYPE_DIALOGUE, "dialogue" },
  { AV_AUDIO_SERVICE_TYPE_COMMENTARY, "commentary" },
  { AV_AUDIO_SERVICE_TYPE_EMERGENCY, "emergency" },
  { AV_AUDIO_SERVICE_TYPE_VOICE_OVER, "voice-over" },
  { AV_AUDIO_SERVICE_TYPE_KARAOKE, "karaoke" },
  { AV_AUDIO_SERVICE_TYPE_NB, "nb" }
};
const beamEnum* beam_av_audio_service_type = new beamEnum(beam_av_audio_service_type_fmap);

std::unordered_map<int, std::string> beam_ff_compliance_fmap = {
  { FF_COMPLIANCE_VERY_STRICT, "very-strict" },
  { FF_COMPLIANCE_STRICT, "strict" },
  { FF_COMPLIANCE_NORMAL, "normal" },
  { FF_COMPLIANCE_UNOFFICIAL, "unofficial" },
  { FF_COMPLIANCE_EXPERIMENTAL, "experimental" }
};
const beamEnum* beam_ff_compliance = new beamEnum(beam_ff_compliance_fmap);

std::unordered_map<int, std::string> beam_ff_dct_fmap = {
  { FF_DCT_AUTO, "auto" },
  { FF_DCT_FASTINT, "fastint "},
  { FF_DCT_INT, "int" },
  { FF_DCT_MMX, "mmx" },
  { FF_DCT_ALTIVEC, "altivec" },
  { FF_DCT_FAAN, "faan" }
};
const beamEnum* beam_ff_dct = new beamEnum(beam_ff_dct_fmap);

std::unordered_map<int, std::string> beam_ff_idct_fmap = {
  { FF_IDCT_AUTO, "auto" },
  { FF_IDCT_INT, "int" },
  { FF_IDCT_SIMPLE, "simple" },
  { FF_IDCT_SIMPLEMMX, "simplemmx" },
  { FF_IDCT_ARM, "arm" },
  { FF_IDCT_ALTIVEC, "altivec" },
  { FF_IDCT_SIMPLEARM, "simplearm" },
  { FF_IDCT_XVID, "xvid" },
  { FF_IDCT_SIMPLEARMV5TE, "simplearmv5te" },
  { FF_IDCT_SIMPLEARMV6, "simplearmv6" },
  { FF_IDCT_FAAN, "faan" },
  { FF_IDCT_SIMPLENEON, "simpleneon" },
  { FF_IDCT_NONE, "none" },
  { FF_IDCT_SIMPLEAUTO, "simpleauto" },
};
const beamEnum* beam_ff_idct = new beamEnum(beam_ff_idct_fmap);

std::unordered_map<int, std::string> beam_avdiscard_fmap = {
  { AVDISCARD_NONE, "none" },
  { AVDISCARD_DEFAULT, "default" },
  { AVDISCARD_NONREF, "nonref" },
  { AVDISCARD_BIDIR, "bidir" },
  { AVDISCARD_NONINTRA, "nonintra" },
  { AVDISCARD_NONKEY, "nonkey" },
  { AVDISCARD_ALL, "all" }
};
const beamEnum* beam_avdiscard = new beamEnum(beam_avdiscard_fmap);

std::unordered_map<int, std::string> beam_ff_sub_charenc_mode_fmap = {
  // do nothing (demuxer outputs a stream supposed to be already in UTF-8, or the codec is bitmap for instance
  { FF_SUB_CHARENC_MODE_DO_NOTHING, "do-nothing" },
  // libavcodec will select the mode itself
  { FF_SUB_CHARENC_MODE_AUTOMATIC, "automatic" },
  // the AVPacket data needs to be recoded to UTF-8 before being fed to the decoder, requires iconv
  { FF_SUB_CHARENC_MODE_PRE_DECODER, "pre-decoder" },
  // neither convert the subtitles, nor check them for valid UTF-8
  { FF_SUB_CHARENC_MODE_IGNORE, "ignore" }
};
const beamEnum* beam_ff_sub_charenc_mode = new beamEnum(beam_ff_sub_charenc_mode_fmap);
