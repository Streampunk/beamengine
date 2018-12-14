#include "node_api.h"

#include <stdio.h>

extern "C" {
  #include <libavutil/avutil.h>
}

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_value value;
  uint32_t v = avutil_version();

  status = napi_create_uint32(env, v, &value);
  status = napi_set_named_property(env, exports, "version", value);

  return exports;
}

NAPI_MODULE(beamcoder, Init)
