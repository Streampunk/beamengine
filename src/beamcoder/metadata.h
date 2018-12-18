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

#ifndef METADATA_H
#define METADATA_H

extern "C" {
  #include <libavformat/avformat.h>
  #include <libavutil/opt.h>
  #include <libavutil/dict.h>
}

#include "beamcoder_util.h"
#include "node_api.h"

void metadataExecute(napi_env env, void* data);
void metadataComplete(napi_env env, napi_status asyncStatus, void* data);
napi_value metadata(napi_env env, napi_callback_info info);

struct metadataCarrier : carrier {
  const char* filename = nullptr;
  AVFormatContext* format = nullptr;
  ~metadataCarrier() {
    if (format != nullptr) { avformat_close_input(&format); }}
};

#endif //METADATA_H
