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

#include "mux.h"

napi_value muxer(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, prop, subprop;
  int ret;
  AVOutputFormat* oformat = nullptr;
  char* formatName = nullptr;
  char* filename = nullptr;
  size_t strLen;
  napi_valuetype type;
  bool isArray;
  AVFormatContext* fmtCtx = nullptr;
  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;
  if (argc != 1) {
    NAPI_THROW_ERROR("Muxer can only be allocated with a single options object.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Muxer must be allocated be a single options object.");
  }
  status = napi_get_named_property(env, args[0], "format_name", &prop);
  CHECK_STATUS;
  status = napi_typeof(env, prop, &type);
  CHECK_STATUS;
  if (type == napi_undefined) {
    status = napi_get_named_property(env, args[0], "name", &prop);
    CHECK_STATUS;
    status = napi_typeof(env, prop, &type);
    CHECK_STATUS;
  }
  if (type == napi_string) { // Found a name property
    status = napi_get_value_string_utf8(env, prop, nullptr, 0, &strLen);
    CHECK_STATUS;
    formatName = (char*) malloc(sizeof(char) * (strLen + 1));
    status = napi_get_value_string_utf8(env, prop, formatName, strLen + 1, &strLen);
    CHECK_STATUS;
  }

  status = napi_get_named_property(env, args[0], "filename", &prop);
  CHECK_STATUS;
  status = napi_typeof(env, prop, &type);
  CHECK_STATUS;
  if (type == napi_string) {
    status = napi_get_value_string_utf8(env, prop, nullptr, 0, &strLen);
    CHECK_STATUS;
    filename = (char*) malloc(sizeof(char) * (strLen + 1));
    status = napi_get_value_string_utf8(env, prop, filename, strLen + 1, &strLen);
    CHECK_STATUS;
  }

  status = napi_get_named_property(env, args[0], "oformat", &prop);
  CHECK_STATUS;
  status = napi_typeof(env, prop, &type);
  CHECK_STATUS;
  status = napi_is_array(env, prop, &isArray);
  CHECK_STATUS;
  if (!isArray && (type == napi_object)) {
    status = napi_get_named_property(env, prop, "_oformat", &subprop);
    CHECK_STATUS;
    status = napi_typeof(env, subprop, &type);
    CHECK_STATUS;
    if (type == napi_external) {
      status = napi_get_value_external(env, subprop, (void**) &oformat);
      CHECK_STATUS;
    }
  }

  ret = avformat_alloc_output_context2(&fmtCtx, oformat, formatName, filename);

  free(formatName);
  free(filename);

  if (ret < 0) {
    NAPI_THROW_ERROR(avErrorMsg("Error allocating muxer context: ", ret));
  }

  status = fromAVFormatContext(env, fmtCtx, &result, true);
  CHECK_STATUS;

  status = napi_create_function(env, "openIO", NAPI_AUTO_LENGTH,
    openIO, nullptr, &prop);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "openIO", prop);
  CHECK_STATUS;

  status = napi_create_function(env, "writeHeader", NAPI_AUTO_LENGTH,
    writeHeader, nullptr, &prop);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "writeHeader", prop);
  CHECK_STATUS;

  status = napi_create_function(env, "initOutput", NAPI_AUTO_LENGTH,
    initOutput, nullptr, &prop);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "initOutput", prop);
  CHECK_STATUS;

  status = napi_create_function(env, "writeFrame", NAPI_AUTO_LENGTH,
    writeFrame, nullptr, &prop);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "writeFrame", prop);
  CHECK_STATUS;

  status = napi_create_function(env, "writeTrailer", NAPI_AUTO_LENGTH,
    writeTrailer, nullptr, &prop);
  CHECK_STATUS;
  status = napi_set_named_property(env, result, "writeTrailer", prop);
  CHECK_STATUS;

  return result;
}

void openIOExecute(napi_env env, void* data) {
  openIOCarrier* c = (openIOCarrier*) data;
}

void openIOComplete(napi_env env, napi_status asyncStatus, void* data) {
  openIOCarrier* c = (openIOCarrier*) data;

  tidyCarrier(env, c);
}

napi_value openIO(napi_env env, napi_callback_info info) {
  napi_value promise;
  openIOCarrier* c = new openIOCarrier;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  return promise;
}

void writeHeaderExecute(napi_env env, void* data) {

}

void writeHeaderComplete(napi_env env, napi_status asyncStatus, void* data) {

}

napi_value writeHeader(napi_env env, napi_callback_info info) {

  return nullptr;
}

void initOutputExecute(napi_env env, void* data) {

}

void initOutputComplete(napi_env env, napi_status asyncStatus, void* data) {

}

napi_value initOutput(napi_env env, napi_callback_info info) {

  return nullptr;
}

void writeFrameExecute(napi_env env, void* data) {

}

void writeFrameComplete(napi_env env, napi_status asyncStatus, void* data) {

}

napi_value writeFrame(napi_env env, napi_callback_info info) {

  return nullptr;
}

void writeTrailerExecute(napi_env env, void* data) {

}

void writeTrailerComplete(napi_env env, napi_status asyncStatus, void* data) {

}

napi_value writeTrailer(napi_env env, napi_callback_info info) {

  return nullptr;
}
