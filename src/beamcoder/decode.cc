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

#include "decode.h"

void decoderExecute(napi_env env, void* data) {
  decoderCarrier* c = (decoderCarrier*) data;
  const AVCodec* codec = nullptr;
  int ret;

  codec = avcodec_find_decoder_by_name(c->codecName);
  if (codec == nullptr) {
    c->status = BEAMCODER_ERROR_ALLOC_DECODER;
    c->errorMsg = "Failed to find a decoder from it's name.";
    return;
  }
  c->decoder = avcodec_alloc_context3(codec);
  printf("Allocate context is %p\n", c->decoder);
  if ((ret = avcodec_open2(c->decoder, codec, nullptr))) {
    c->status = BEAMCODER_ERROR_ALLOC_DECODER;
    c->errorMsg = avErrorMsg("Problem allocating decoder: ", ret);
    return;
  }
}

void decoderComplete(napi_env env, napi_status asyncStatus, void* data) {
  decoderCarrier* c = (decoderCarrier*) data;
  napi_value result, value;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Decoder allocator failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  c->status = napi_create_string_utf8(env, "decoder", NAPI_AUTO_LENGTH, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "type", value);
  REJECT_STATUS;

  c->status = napi_create_external(env, c->decoder, decoderFinalizer, nullptr, &value);
  REJECT_STATUS;
  c->decoder = nullptr;
  c->status = napi_set_named_property(env, result, "_decoder", value);
  REJECT_STATUS;

  c->status = napi_create_function(env, "decode", NAPI_AUTO_LENGTH, decode, nullptr, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "decode", value);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value decoder(napi_env env, napi_callback_info info) {
  napi_value resourceName, promise, value;
  napi_valuetype type;
  decoderCarrier* c = new decoderCarrier;
  bool isArray, hasName, hasID;
  AVCodecID id = AV_CODEC_ID_NONE;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];

  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;

  if (argc != 1) {
    REJECT_ERROR_RETURN("Decoder requires a single options object.",
      BEAMCODER_INVALID_ARGS);
  }

  c->status = napi_typeof(env, args[0], &type);
  REJECT_RETURN;
  c->status = napi_is_array(env, args[0], &isArray);
  REJECT_RETURN;
  if ((type != napi_object) || (isArray == true)) {
    REJECT_ERROR_RETURN("Decoder must be configured with a single parameter, an options object.",
      BEAMCODER_INVALID_ARGS);
  }

  c->status = napi_has_named_property(env, args[0], "name", &hasName);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "codecID", &hasID);
  REJECT_RETURN;
  if (!(hasName || hasID)) {
    REJECT_ERROR_RETURN("Decoder must be identified with a 'codecID' or a 'codecName'.",
      BEAMCODER_INVALID_ARGS);
  }

  if (hasName) {
    c->status = napi_get_named_property(env, args[0], "name", &value);
    REJECT_RETURN;
    c->codecName = (char*) malloc(sizeof(char) * (c->codecNameLen + 1));
    c->status = napi_get_value_string_utf8(env, value, c->codecName,
      64, &c->codecNameLen);
    printf("Codec name is %s AV_CODEC_ID_H264 = %i\n", c->codecName,  AV_CODEC_ID_H264);
    REJECT_RETURN;
  }
  else {
    c->status = napi_get_named_property(env, args[0], "codecID", &value);
    REJECT_RETURN;
    c->status = napi_get_value_int32(env, value, (int32_t*) &id);
    REJECT_RETURN;
    c->codecName = (char*) avcodec_get_name(id);
    c->codecNameLen = strlen(c->codecName);
  }

  c->status = napi_create_string_utf8(env, "Decoder", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, decoderExecute,
    decoderComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}

void decoderFinalizer(napi_env env, void* data, void* hint) {
  AVCodecContext* decoder = (AVCodecContext*) data;
  avcodec_close(decoder);
  avcodec_free_context(&decoder);
}

void decodeExecute(napi_env env, void* data) {
  decodeCarrier* c = (decodeCarrier*) data;
  int ret = 0;
  AVFrame* frame = nullptr;

  for ( auto it = c->packets.cbegin() ; it != c->packets.cend() ; it++ ) {
  bump:
    ret = avcodec_send_packet(c->decoder, *it);
    switch (ret) {
      case AVERROR(EAGAIN):
        printf("Input is not accepted in the current state - user must read output with avcodec_receive_frame().\n");
        frame = av_frame_alloc();
        avcodec_receive_frame(c->decoder, frame);
        c->frames.push_back(frame);
        goto bump;
      case AVERROR_EOF:
        printf("The decoder has been flushed, and no new packets can be sent to it.\n");
        break;
      case AVERROR(EINVAL):
        printf("Codec not opened.\n");
        break;
      case AVERROR(ENOMEM):
        printf("Failed to add packet to internal queue.\n");
        break;
      case 0:
        printf("Successfully sent packet to codec.\n");
        break;
      default:
        printf("Error sending packet %i.\n", ret);
        break;
    }
  }

  do {
    frame = av_frame_alloc();
    ret = avcodec_receive_frame(c->decoder, frame);
    if (ret == 0) {
      c->frames.push_back(frame);
      frame = av_frame_alloc();
    }
  } while (ret == 0);
  av_frame_free(&frame);
};

void decodeComplete(napi_env env, napi_status asyncStatus, void* data) {
  decodeCarrier* c = (decodeCarrier*) data;
  napi_value result, value, frame, prop;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Decoder allocator failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_array(env, &result);
  REJECT_STATUS;
  uint32_t frameCount = 0;
  for ( auto it = c->frames.begin() ; it != c->frames.end() ; it++ ) {
    c->status = napi_create_object(env, &frame);
    REJECT_STATUS;
    AVFrame* item = *it;

    c->status = napi_create_string_utf8(env, "frame", NAPI_AUTO_LENGTH, &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, frame, "type", prop);
    REJECT_STATUS;

    c->status = napi_create_int32(env, item->buf[0]->size, &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, frame, "size", prop);
    REJECT_STATUS;

    c->status = napi_create_int32(env, item->linesize[0], &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, frame, "linesize", prop);
    REJECT_STATUS;

    c->status = napi_create_int32(env, item->width, &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, frame, "width", prop);
    REJECT_STATUS;

    c->status = napi_create_int32(env, item->height, &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, frame, "height", prop);
    REJECT_STATUS;

    c->status = napi_set_element(env, result, frameCount++, frame);
    REJECT_STATUS;
  }

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
  printf("Completing and resolving.\n");
};

napi_value decode(napi_env env, napi_callback_info info) {
  napi_value resourceName, promise, decoderJS, decoderExt, value;
  napi_valuetype type;
  decodeCarrier* c = new decodeCarrier;
  bool isArray;
  uint32_t packetsLength;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 0;
  napi_value* args = nullptr;

  c->status = napi_get_cb_info(env, info, &argc, args, &decoderJS, nullptr);
  REJECT_RETURN;
  c->status = napi_typeof(env, decoderJS, &type);
  REJECT_RETURN;
  c->status = napi_get_named_property(env, decoderJS, "_decoder", &decoderExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, decoderExt, (void**) &c->decoder);
  REJECT_RETURN;

  if (argc == 0) {
    printf("Rejecting\n");
    REJECT_ERROR_RETURN("Decode call requires one or more packets.",
      BEAMCODER_INVALID_ARGS);
  }

  args = (napi_value*) malloc(sizeof(napi_value) * argc);
  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;

  c->status = napi_is_array(env, args[0], &isArray);
  REJECT_RETURN;
  if (isArray) {
    c->status = napi_get_array_length(env, args[0], &packetsLength);
    REJECT_RETURN;
    for ( uint32_t x = 0 ; x < packetsLength ; x++ ) {
      c->status = napi_get_element(env, args[0], x, &value);
      REJECT_RETURN;
      c->status = isPacket(env, value);
      if (c->status != napi_ok) {
        REJECT_ERROR_RETURN("All passed packets in an array must be of type packet.",
          BEAMCODER_INVALID_ARGS);
      }
      c->packets.push_back(getPacket(env, value));
    }
  }
  else {
    for ( uint32_t x = 0 ; x < argc ; x++ ) {
      c->status = isPacket(env, args[x]);
      if (c->status != napi_ok) {
        REJECT_ERROR_RETURN("All passed packets as arguments must be of type packet.",
          BEAMCODER_INVALID_ARGS);
      }
      c->packets.push_back(getPacket(env, args[x]));
    }
  }

  c->status = napi_create_string_utf8(env, "Decode", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, decodeExecute,
    decodeComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
};

napi_status isPacket(napi_env env, napi_value packet) {
  napi_status status;
  napi_value value;
  bool result;
  char objType[10];
  size_t typeLen;
  int cmp;
  napi_valuetype type;

  status = napi_typeof(env, packet, &type);
  if ((status != napi_ok) || (type != napi_object)) return napi_invalid_arg;
  status = napi_is_array(env, packet, &result);
  if ((status != napi_ok) || (result == true)) return napi_invalid_arg;

  status = napi_has_named_property(env, packet, "type", &result);
  if ((status != napi_ok) || (result == false)) return napi_invalid_arg;

  status = napi_has_named_property(env, packet, "_packet", &result);
  if ((status != napi_ok) || (result == false)) return napi_invalid_arg;

  status = napi_get_named_property(env, packet, "type", &value);
  if (status != napi_ok) return status;
  status = napi_get_value_string_utf8(env, value, objType, 10, &typeLen);
  if (status != napi_ok) return status;
  cmp = strcmp("packet", objType);
  if (cmp != 0) return napi_invalid_arg;

  status = napi_get_named_property(env, packet, "_packet", &value);
  if (status != napi_ok) return status;
  status = napi_typeof(env, value, &type);
  if (status != napi_ok) return status;
  if (type != napi_external) return napi_invalid_arg;

  return napi_ok;
}

AVPacket* getPacket(napi_env env, napi_value packet) {
  napi_status status;
  napi_value value;
  AVPacket* result = nullptr;
  status = napi_get_named_property(env, packet, "_packet", &value);
  printf("AVPacket result pointer 1 %p\n", result);
  if (status != napi_ok) return nullptr;
  status = napi_get_value_external(env, value, (void**) &result);
  printf("AVPacket result pointer 2 %p\n", result);
  if (status != napi_ok) return nullptr;

  printf("AVPacket result pointer 3 %p\n", result);

  return result;
}
