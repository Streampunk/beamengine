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

#include "packet.h"

napi_value getPacketPts(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  packetData* p;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &p);
  CHECK_STATUS;

  if (p->packet->pts == AV_NOPTS_VALUE) {
    status = napi_get_null(env, &result);
  } else {
    status = napi_create_int64(env, p->packet->pts, &result);
  }
  CHECK_STATUS;
  return result;
}

napi_value setPacketPts(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  packetData* p;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &p);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set packet PTS must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  switch (type) {
    case napi_null:
      p->packet->pts = AV_NOPTS_VALUE;
      break;
    case napi_number:
      status = napi_get_value_int64(env, args[0], &p->packet->pts);
      CHECK_STATUS;
      break;
    default:
      NAPI_THROW_ERROR("Packet PTS property must be set with a number.");
      break;
  }

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getPacketDts(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  packetData* p;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &p);
  CHECK_STATUS;

  status = napi_create_int64(env, p->packet->dts, &result);
  CHECK_STATUS;
  return result;
}

napi_value setPacketDts(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  packetData* p;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &p);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set packet DTS must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Packet DTS property must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &p->packet->dts);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getPacketData(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  packetData* p;
  AVBufferRef* hintRef;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &p);
  CHECK_STATUS;

  if (p->packet->buf == nullptr) {
    status = napi_get_null(env, &result);
  } else {
    hintRef = av_buffer_ref(p->packet->buf);
    status = napi_create_external_buffer(env, hintRef->size, hintRef->data,
      packetBufferFinalizer, hintRef, &result);
    CHECK_STATUS;
  }

  CHECK_STATUS;
  return result;
}

napi_value setPacketData(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  bool isBuffer;
  packetData* p;
  uint8_t* data;
  size_t length;
  avBufRef* avr = new avBufRef;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &p);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set packet data must be provided with a buffer value.");
  }
  status = napi_is_buffer(env, args[0], &isBuffer);
  CHECK_STATUS;
  if (!isBuffer) {
    NAPI_THROW_ERROR("Packet data property must be set with a buffer.");
  }

  if (p->dataRef != nullptr) {
    status = napi_delete_reference(env, p->dataRef);
    CHECK_STATUS;
  }
  status = napi_create_reference(env, args[0], 1, &p->dataRef);
  CHECK_STATUS;
  status = napi_create_reference(env, args[0], 1, &avr->ref);
  CHECK_STATUS;
  avr->env = env;
  status = napi_get_buffer_info(env, args[0], (void**) &data, &length);
  CHECK_STATUS;
  if (p->packet->buf != nullptr) {
    av_buffer_unref(&p->packet->buf);
  }
  p->packet->buf = av_buffer_create(data, length, packetBufferFree, avr, 0);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

// TODO is this a smell? Can this actually run from a separate thread?
void packetBufferFree(void* opaque, uint8_t* data) {
  napi_status status;
  avBufRef* avr = (avBufRef*) opaque;
  status = napi_delete_reference(avr->env, (napi_ref) avr->ref);
  if (status != napi_ok)
    printf("DEBUG: Failed to delete buffer reference associated with an AVBufferRef.");
  delete avr;
}

napi_value getPacketSize(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  packetData *p;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &p);
  CHECK_STATUS;

  status = napi_create_int32(env,
    (p->packet->buf == nullptr) ? 0 : p->packet->buf->size, &result);
  CHECK_STATUS;

  return result;
}

napi_value setPacketSize(napi_env env, napi_callback_info info) {
  NAPI_THROW_ERROR("Cannot set packet size. It is automatically calculated.");
}

napi_value getPacketStreamIndex(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  packetData* p;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &p);
  CHECK_STATUS;

  status = napi_create_int32(env, p->packet->stream_index, &result);
  CHECK_STATUS;
  return result;
}

napi_value setPacketStreamIndex(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  packetData* p;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &p);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set packet stream_index must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Packet stream_index property must be set with a number.");
  }
  status = napi_get_value_int32(env, args[0], &p->packet->stream_index);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getPacketDuration(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  packetData* p;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &p);
  CHECK_STATUS;

  status = napi_create_int64(env, p->packet->duration, &result);
  CHECK_STATUS;
  return result;
}

napi_value setPacketDuration(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  packetData* p;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &p);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set packet duration must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Packet duration property must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &p->packet->duration);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value getPacketPos(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  packetData* p;

  status = napi_get_cb_info(env, info, 0, nullptr, nullptr, (void**) &p);
  CHECK_STATUS;

  status = napi_create_int64(env, p->packet->pos, &result);
  CHECK_STATUS;
  return result;
}

napi_value setPacketPos(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  napi_valuetype type;
  packetData* p;

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, (void**) &p);
  CHECK_STATUS;
  if (argc < 1) {
    NAPI_THROW_ERROR("Set packet pos must be provided with a value.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  if (type != napi_number) {
    NAPI_THROW_ERROR("Packet pos property must be set with a number.");
  }
  status = napi_get_value_int64(env, args[0], &p->packet->pos);
  CHECK_STATUS;

  status = napi_get_undefined(env, &result);
  CHECK_STATUS;
  return result;
}

napi_value makePacket(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, global, jsObject, assign;
  napi_valuetype type;
  bool isArray;
  packetData* p = new packetData;
  p->packet = av_packet_alloc();

  size_t argc = 1;
  napi_value args[1];

  status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  CHECK_STATUS;
  if (argc > 1) {
    NAPI_THROW_ERROR("Packet may be created with exactly one options object.");
  }
  status = napi_typeof(env, args[0], &type);
  CHECK_STATUS;
  status = napi_is_array(env, args[0], &isArray);
  CHECK_STATUS;
  if (isArray || (type != napi_object)) {
    NAPI_THROW_ERROR("Cannot create a packet without an options object.");
  }

  status = packetFromAVPacket(env, p, &result);
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

napi_status packetFromAVPacket(napi_env env, packetData* p, napi_value* result) {
  napi_status status;
  napi_value jsPacket, extPacket, typeName;

  status = napi_create_object(env, &jsPacket);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "Packet", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;
  status = napi_create_external(env, p, packetDataFinalizer, nullptr, &extPacket);
  PASS_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "pts", nullptr, nullptr, getPacketPts, setPacketPts, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), p },
    { "dts", nullptr, nullptr, getPacketDts, setPacketDts, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), p },
    { "data", nullptr, nullptr, getPacketData, setPacketData, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), p },
    { "size", nullptr, nullptr, getPacketSize, setPacketSize, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), p },
    { "stream_index", nullptr, nullptr, getPacketStreamIndex, setPacketStreamIndex, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), p },
    { "duration", nullptr, nullptr, getPacketDuration, setPacketDuration, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), p },
    { "pos", nullptr, nullptr, getPacketPos, setPacketPos, nullptr,
      (napi_property_attributes) (napi_writable | napi_enumerable), p },
    { "_packet", nullptr, nullptr, nullptr, nullptr, extPacket, napi_default, nullptr }
  };
  status = napi_define_properties(env, jsPacket, 9, desc);
  PASS_STATUS;

  if (p->packet->buf != nullptr) {
    p->extSize = p->packet->buf->size;
    status = napi_adjust_external_memory(env, p->extSize, nullptr);
    PASS_STATUS;
  }

  *result = jsPacket;
  return napi_ok;
}

void packetFinalizer(napi_env env, void* data, void* hint) {
  AVPacket* pkt = (AVPacket*) data;
  av_packet_free(&pkt);
}

void packetDataFinalizer(napi_env env, void* data, void* hint) {
  napi_status status;
  napi_ref dataRef;
  packetData* p = (packetData*) data;
  dataRef = p->dataRef;
  status = napi_adjust_external_memory(env, -p->extSize, nullptr);
  if (status != napi_ok) {
    printf("DEBUG: Failed to adjust external memory downwards on packet delete.");
  }
  delete p;
  if (dataRef != nullptr) {
    status = napi_delete_reference(env, dataRef);
    if (status != napi_ok) {
      printf("DEBUG: Failed to delete data reference for packet data, status %i.\n", status);
    }
  }
}

void packetBufferFinalizer(napi_env env, void* data, void* hint) {
  AVBufferRef* hintRef = (AVBufferRef*) hint;
  av_buffer_unref(&hintRef);
};
