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

  if ((ret = avformat_open_input(&c->fmt_ctx, c->filename, NULL, NULL))) {
    printf("Could not read.\n");
    c->status = BEAMCODER_ERROR_START;
    c->errorMsg = "Failed to open file.";
    return;
  }

  // Do the stream info thing?
}

void metadataComplete(napi_env env,  napi_status asyncStatus, void* data) {
  metadataCarrier* c = (metadataCarrier*) data;
  napi_value result;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Async capture creator failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_uint32(env, c->fmt_ctx->nb_streams, &result);
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
    c->status = napi_get_value_string_utf8(env, args[0], NULL, 0, &strLen);
    REJECT_RETURN;
    c->filename = (const char *) malloc((strLen + 1) * sizeof(char));
    c->status = napi_get_value_string_utf8(env, args[0], (char *) c->filename, strLen + 1, &strLen);
    REJECT_RETURN;
  }

  c->status = napi_create_string_utf8(env, "Metadata", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, NULL, resourceName, metadataExecute,
    metadataComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}
