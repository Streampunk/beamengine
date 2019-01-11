/*
  Aerostat Beam Coder - Node.JS native mappings for FFmpeg.
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

#include "frame.h"

napi_value getFrameLinesize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value array, element;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_array(env, &array);
  CHECK_STATUS;

  for ( int x = 0 ; x < AV_NUM_DATA_POINTERS ; x++ ) {
    if (f->frame->linesize[x] == 0) break;
    status = napi_create_int32(env, f->frame->linesize[x], &element);
    CHECK_STATUS;
    status = napi_set_element(env, array, x, element);
    CHECK_STATUS;
  }

  return array;
}

napi_value setFrameLinesize(napi_env env, napi_callback_info info) {

  // TODO
  return nullptr;
}

napi_value getFrameWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->width, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameWidth(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame width must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame width property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->width);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->height, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameHeight(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame height must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame height property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->height);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameNbSamples(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->nb_samples, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameNbSamples(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame nb_samples must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame nb_samples property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->nb_samples);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;
  const char* name = nullptr;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  if (f->frame->nb_samples > 0) { // Assume audio data
    name = av_get_sample_fmt_name((AVSampleFormat) f->frame->format);
  }
  if (name == nullptr) { // Assume that it is video data
    name = av_get_pix_fmt_name((AVPixelFormat) f->frame->format);
  }
  if (name == nullptr) {
    status = napi_get_null(env, &result);
  } else {
    status = napi_create_string_utf8(env, (char*) name, NAPI_AUTO_LENGTH, &result);
  }
  CHECK_STATUS;

  return result;
}

napi_value setFrameFormat(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;
  char* name;
  size_t len;
  int format = (int) AV_PIX_FMT_NONE;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame format must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type == napi_null) {
    f->frame->format = -1;
    goto done;
  }
  if (type != napi_string) {
    NAPI_THROW_ERROR("Frame format property must be set with a string.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &len);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (len + 1));
  status = napi_get_value_string_utf8(env, args[0], name, len + 1, &len);
  CHECK_STATUS;

  // TODO this may give surprising results
  format = (int) av_get_pix_fmt((const char *) name);
  if (format == AV_PIX_FMT_NONE) {
    format = (int) av_get_sample_fmt((const char*) name);
    if ((format != AV_SAMPLE_FMT_NONE) && (f->frame->nb_samples == 0)) {
      f->frame->nb_samples = 1; // Cludge ... found a sample format ... force audio mode
    }
  }

  f->frame->format = format;

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameKeyFrame(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_get_boolean(env, (f->frame->key_frame == 1), &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameKeyFrame(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;
  bool keyFrame;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame key_frame must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_boolean) {
    NAPI_THROW_ERROR("Frame key_frame property must be set with a Boolean.");
  }
  status = napi_get_value_bool(env, args[0], &keyFrame);
  CHECK_STATUS;
  f->frame->key_frame = (keyFrame) ? 1 : 0;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFramePictType(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;
  const char* name;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  switch (f->frame->pict_type) {
    case AV_PICTURE_TYPE_I:
      name = "I";
      break;
    case AV_PICTURE_TYPE_P:
      name = "P";
      break;
    case AV_PICTURE_TYPE_B:
      name = "B";
      break;
    case AV_PICTURE_TYPE_S:
      name = "S";
      break;
    case AV_PICTURE_TYPE_SI:
      name = "SI";
      break;
    case AV_PICTURE_TYPE_SP:
      name = "SP";
      break;
    case AV_PICTURE_TYPE_BI:
      name = "BI";
      break;
    default:
      name = nullptr;
      break;
  }

  if (name == nullptr) {
    status = napi_get_undefined(env, &result);
  } else {
    status = napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &result);
  }
  CHECK_STATUS;
  return result;
}

napi_value setFramePictType(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;
  char* name;
  size_t len;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame pict_type must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_undefined) || (type == napi_null)) {
    f->frame->pict_type = AV_PICTURE_TYPE_NONE;
    goto done;
  }
  if (type != napi_string) {
    NAPI_THROW_ERROR("Frame pict_type property must be set with a string.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &len);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (len + 1));
  status = napi_get_value_string_utf8(env, args[0], name, len + 1, &len);
  CHECK_STATUS;

  switch (name[0]) {
    case 'I':
      f->frame->pict_type = AV_PICTURE_TYPE_I;
      break;
    case 'P':
      f->frame->pict_type = AV_PICTURE_TYPE_P;
      break;
    case 'B':
      f->frame->pict_type = (len == 1) ? AV_PICTURE_TYPE_B : AV_PICTURE_TYPE_BI;
      break;
    case 'S':
      if (len == 1) {
        f->frame->pict_type = AV_PICTURE_TYPE_S;
        break;
      }
      if (len == 2) {
        if (name[1] == 'I') {
          f->frame->pict_type = AV_PICTURE_TYPE_SI;
          break;
        }
        if (name[1] == 'P') {
          f->frame->pict_type = AV_PICTURE_TYPE_SP;
          break;
        }
      }
      f->frame->pict_type = AV_PICTURE_TYPE_NONE;
      break;
    default:
      f->frame->pict_type = AV_PICTURE_TYPE_NONE;
      break;
  }

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameSampleAR(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;
  status = napi_create_int32(env, f->frame->sample_aspect_ratio.num, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 0, element);
  CHECK_STATUS;
  status = napi_create_int32(env, f->frame->sample_aspect_ratio.den, &element);
  CHECK_STATUS;
  status = napi_set_element(env, result, 1, element);
  CHECK_STATUS;

  return result;
}

napi_value setFrameSampleAR(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  napi_valuetype type;
  frameData* f;
  bool isArray;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame sample_aspect_ratio must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if ((type == napi_undefined) || (type == napi_null)) {
    f->frame->sample_aspect_ratio = { 0, 1 };
    goto done;
  }
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (!isArray) {
    NAPI_THROW_ERROR("Frame sample_aspect_ratio property must be set with an array of two numbers.");
  }

  f->frame->sample_aspect_ratio = { 0, 1 };
  status = napi_get_element(env, args[0], 0, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &f->frame->sample_aspect_ratio.num);
  CHECK_STATUS;
  status = napi_get_element(env, args[0], 1, &element);
  CHECK_STATUS;
  status = napi_get_value_int32(env, element, &f->frame->sample_aspect_ratio.den);
  CHECK_STATUS;

done:
  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFramePTS(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int64(env, f->frame->pts, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFramePTS(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame PTS must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame PTS property must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &f->frame->pts);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFramePktDTS(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int64(env, f->frame->pkt_dts, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFramePktDTS(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame pkt_dts must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame pkt_dts property must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &f->frame->pkt_dts);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameCodecPicNum(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->coded_picture_number, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameCodecPicNum(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame coded_picture_number must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame coded_picture_number property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->coded_picture_number);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameDispPicNum(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->display_picture_number, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameDispPicNum(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame display_picture_number must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame display_picture_number property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->display_picture_number);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameQuality(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->quality, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameQuality(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame quality must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame quality property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->quality);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameRepeatPict(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->repeat_pict, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameRepeatPict(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame repeat_pict must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame repeat_pict property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->repeat_pict);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameInterlaced(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_get_boolean(env, f->frame->interlaced_frame == 1, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameInterlaced(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;
  bool interlaced;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame interlaced_frame must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_boolean) {
    NAPI_THROW_ERROR("Frame interlaced_frame property must be set with a Boolean.");
  }
  status = napi_get_value_bool(env, args[0], &interlaced);
  CHECK_STATUS;
  f->frame->interlaced_frame = (interlaced) ? 1 : 0;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameTopFieldFirst(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_get_boolean(env, f->frame->top_field_first == 1, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameTopFieldFirst(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;
  bool topFieldFirst;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame top_field_first must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_boolean) {
    NAPI_THROW_ERROR("Frame top_field_first property must be set with a Boolean.");
  }
  status = napi_get_value_bool(env, args[0], &topFieldFirst);
  CHECK_STATUS;
  f->frame->top_field_first = (topFieldFirst) ? 1 : 0;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFramePalHasChanged(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_get_boolean(env, f->frame->palette_has_changed == 1, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFramePalHasChanged(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;
  bool palHasChanged;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame palette_has_changed must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_boolean) {
    NAPI_THROW_ERROR("Frame palette_has_changed property must be set with a Boolean.");
  }
  status = napi_get_value_bool(env, args[0], &palHasChanged);
  CHECK_STATUS;
  f->frame->palette_has_changed = (palHasChanged) ? 1 : 0;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getFrameSampleRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  status = napi_create_int32(env, f->frame->sample_rate, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameSampleRate(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame sample_rate must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame sample_rate property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &f->frame->sample_rate);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}


napi_value getFrameChanLayout(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  frameData* f;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &f);
  CHECK_STATUS;

  char channelLayoutName[64];
  av_get_channel_layout_string(channelLayoutName, 64, -1, f->frame->channel_layout);

  status = napi_create_string_utf8(env, channelLayoutName, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;
  return result;
}

napi_value setFrameChanLayout(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  frameData* f;
  char* name;
  size_t len;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &f);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set frame sample_rate must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Frame sample_rate property must be set with a number.");
  }
  status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &len);
  CHECK_STATUS;
  name = (char*) malloc(sizeof(char) * (len + 1));
  status = napi_get_value_string_utf8(env, args[0], name, len + 1, &len);
  CHECK_STATUS;

  f->frame->channel_layout = av_get_channel_layout(name);

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value makeFrame(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, global, jsObject, assign;
  napi_valuetype type;
  bool isArray;
  frameData* f = new frameData;
  f->frame = av_frame_alloc();

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;
  if (argc > 1) {
    NAPI_THROW_ERROR("Frame may be created with zero or one options object argument.");
  }
  if (argc == 1) {
    status = napi_typeof(env, args[0], &type);
    CHECK_STATUS;
    status = napi_is_array(env, args[0], &isArray);
    CHECK_STATUS;
    if (isArray || (type != napi_object)) {
      NAPI_THROW_ERROR("Cannot create a frame without an options object.");
    }
  }

  status = frameFromAVFrame(env, f, &result);
  CHECK_STATUS;

  if (argc == 1) {
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

napi_status frameFromAVFrame(napi_env env, frameData* f, napi_value* result) {
  napi_status status;
  napi_value jsFrame, extFrame, typeName;
  int64_t externalMemory;

  status = napi_create_object(env, &jsFrame);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "Frame", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  status = napi_create_external(env, f, frameDataFinalizer, nullptr, &extFrame);
  PASS_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "linesize", nullptr, nullptr, getFrameLinesize, setFrameLinesize, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "width", nullptr, nullptr, getFrameWidth, setFrameWidth, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "height", nullptr, nullptr, getFrameHeight, setFrameHeight, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "nb_samples", nullptr, nullptr, getFrameNbSamples, setFrameNbSamples, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "format", nullptr, nullptr, getFrameFormat, setFrameFormat, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "key_frame", nullptr, nullptr, getFrameKeyFrame, setFrameKeyFrame, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "pict_type", nullptr, nullptr, getFramePictType, setFramePictType, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "sample_aspect_ratio", nullptr, nullptr, getFrameSampleAR, setFrameSampleAR, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "pts", nullptr, nullptr, getFramePTS, setFramePTS, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f }, // 10
    { "pkt_dts", nullptr, nullptr, getFramePktDTS, setFramePktDTS, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "codec_picture_number", nullptr, nullptr, getFrameCodecPicNum, setFrameCodecPicNum, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "display_picture_number", nullptr, nullptr, getFrameDispPicNum, setFrameDispPicNum, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "quality", nullptr, nullptr, getFrameQuality, setFrameQuality, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "repeat_pict", nullptr, nullptr, getFrameRepeatPict, setFrameRepeatPict, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "interlaced_frame", nullptr, nullptr, getFrameInterlaced, setFrameInterlaced, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "top_field_first", nullptr, nullptr, getFrameTopFieldFirst, setFrameTopFieldFirst, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "palette_has_changed", nullptr, nullptr, getFramePalHasChanged, setFramePalHasChanged, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "sample_rate", nullptr, nullptr, getFrameSampleRate, setFrameSampleRate, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f },
    { "channel_layout", nullptr, nullptr, getFrameChanLayout, setFrameChanLayout, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), f }, // 20
    { "_frame", nullptr, nullptr, nullptr, nullptr, extFrame, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsFrame, 21, desc);
  PASS_STATUS;

  // Add in external memory usage

  *result = jsFrame;
  return napi_ok;
}

void frameFinalizer(napi_env env, void* data, void* hint) {
  AVFrame* frame = (AVFrame*) data;
  av_frame_free(&frame);
}

void frameDataFinalizer(napi_env env, void* data, void* hint) {

}

void frameBufferFinalizer(napi_env env, void* data, void* hint) {
  AVBufferRef* hintRef = (AVBufferRef*) hint;
  // printf("Frame buffer finalizer called with %p.\n", hint);
  napi_status status;
  int64_t externalMemory;
  status = napi_adjust_external_memory(env, -hintRef->size, &externalMemory);
  // printf("External memory is %i\n", externalMemory);
  if (status != napi_ok) {
    printf("DEBUG: Napi failure to adjust external memory. In beamcoder decode.cc frameBufferFinalizer.");
  }
  av_buffer_unref(&hintRef);
}

void frameBufferFree(void* opaque, uint8_t* data) {

}
