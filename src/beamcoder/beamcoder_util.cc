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

  status = beam_set_int32(env, target, "codec_id", codec->codec_id);
  PASS_STATUS;
  status = beam_set_string_utf8(env, target, "name", (char*) codec->codec->name);
  PASS_STATUS;
  status = beam_set_string_utf8(env, target, "long_name", (char*) codec->codec->long_name);
  PASS_STATUS;
  status = beam_set_string_utf8(env, target, "codec_type",
    (char*) av_get_media_type_string(codec->codec_type));
  PASS_STATUS;
  char codecTag[64];
  av_get_codec_tag_string(codecTag, 64, codec->codec_tag);
  status = beam_set_string_utf8(env, target, "codec_tag", codecTag);
  PASS_STATUS;
  // TODO consider priv_data
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


  return napi_ok;
};

napi_status setCodecFromProps(napi_env env, AVCodecContext* codec,
    napi_value props, bool encoding) {
  napi_status status;
  napi_value value;

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

  return napi_ok;
};

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
  status = napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &prop);
  PASS_STATUS;
  return napi_set_named_property(env, target, name, prop);
}

napi_status beam_get_string_utf8(napi_env env, napi_value target, char* name, char** value) {
  napi_status status;
  napi_value prop;
  char* str;
  size_t len;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_string_utf8(env, prop, nullptr, 0, &len);
  if (status == napi_string_expected) return napi_ok;
  PASS_STATUS;
  str = (char*) malloc((len + 1) * sizeof(char));
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

napi_status beam_get_bool(napi_env env, napi_value target, char* name, bool* value) {
  napi_status status;
  napi_value prop;
  status = napi_get_named_property(env, target, name, &prop);
  PASS_STATUS;
  status = napi_get_value_bool(env, prop, value);
  ACCEPT_STATUS(napi_boolean_expected);
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

const char* beam_field_order_name(AVFieldOrder field_order) {
  switch (field_order) {
    case AV_FIELD_PROGRESSIVE:
      return "progressive";
    case AV_FIELD_TT:
      return "top coded_first, top displayed first";
    case AV_FIELD_BB:
      return "bottom coded first, bottom displayed first";
    case AV_FIELD_TB:
      return "top coded first, bottom displayed first";
    case AV_FIELD_BT:
      return "bottom coded first, top displayed first";
    default:
    case AV_FIELD_UNKNOWN:
      return "unknown";
  }
};
