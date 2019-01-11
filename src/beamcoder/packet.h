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

#ifndef PACKET_H
#define PACKET_H

#include "node_api.h"
#include "beamcoder_util.h"

extern "C" {
  #include <libavcodec/avcodec.h>
}

napi_status getPropsFromPacket(napi_env env, napi_value target, AVPacket* packet);
napi_status setPacketFromProps(napi_env env, AVPacket* packet, napi_value props);

void packetFinalizer(napi_env env, void* data, void* hint);
void packetDataFinalizer(napi_env env, void* data, void* hint);

napi_value makePacket(napi_env env, napi_callback_info info);

struct packetData {
  AVPacket* packet = av_packet_alloc();
  napi_ref dataRef = nullptr;
  ~packetData() {
    av_packet_free(&packet);
  }
};

#endif // PACKET_H
