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
  AVDictionary* dict = nullptr;
  // AVDictionaryEntry* entry = nullptr;

  codec = avcodec_find_decoder_by_name(c->codecName);
  if (codec == nullptr) {
    c->status = BEAMCODER_ERROR_ALLOC_DECODER;
    c->errorMsg = "Failed to find a decoder from it's name.";
    return;
  }
  c->decoder = avcodec_alloc_context3(codec);
  if (c->decoder == nullptr) {
    c->status = BEAMCODER_ERROR_ALLOC_DECODER;
    c->errorMsg = "Problem allocating decoder context.";
    return;
  }
  if (c->params != nullptr) {
    if ((ret = avcodec_parameters_to_context(c->decoder, c->params))) {
      printf("DEBUG: Failed to set context parameters from those provided.");
    }
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

  setCodecFromProps(env, c->decoder, result);

  c->status = beam_set_string_utf8(env, result, "type", "decoder");
  REJECT_STATUS;

  c->status = napi_create_string_utf8(env, avcodec_get_name(c->decoder->codec_id),
    NAPI_AUTO_LENGTH, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "name", value);
  REJECT_STATUS;

  if (c->streamIdx != -1) {
    c->status = napi_create_int32(env, c->streamIdx, &value);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, result, "stream", value);
    REJECT_STATUS;
  }

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
  napi_value resourceName, promise, value, formatJS, formatExt;
  napi_valuetype type;
  decoderCarrier* c = new decoderCarrier;
  bool isArray, hasName, hasID, hasFormat, hasStream;
  AVCodecID id = AV_CODEC_ID_NONE;
  AVFormatContext* format = nullptr;

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
  c->status = napi_has_named_property(env, args[0], "format", &hasFormat);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "stream", &hasStream);
  REJECT_RETURN;

  if (hasFormat && hasStream) {
    c->status = napi_get_named_property(env, args[0], "format", &formatJS);
    REJECT_RETURN;
    c->status = napi_get_named_property(env, formatJS, "_formatContext", &formatExt);
    REJECT_RETURN;
    c->status = napi_get_value_external(env, formatExt, (void**) &format);
    REJECT_RETURN;

    c->status = napi_get_named_property(env, args[0], "stream", &value);
    REJECT_RETURN;
    c->status = napi_get_value_int32(env, value, &c->streamIdx);
    REJECT_RETURN;
    if (c->streamIdx < 0 || c->streamIdx >= format->nb_streams) {
      REJECT_ERROR_RETURN("Stream index is out of bounds for the given format.",
        BEAMCODER_ERROR_OUT_OF_BOUNDS);
    }
    c->params = format->streams[c->streamIdx]->codecpar;
    c->codecName = (char*) avcodec_get_name(c->params->codec_id);
    c->codecNameLen = strlen(c->codecName);
    goto create;
  }

  if (!(hasName || hasID)) {
    REJECT_ERROR_RETURN("Decoder must be identified with a 'codecID' or a 'name'.",
      BEAMCODER_INVALID_ARGS);
  }

  if (hasName) {
    c->status = napi_get_named_property(env, args[0], "name", &value);
    REJECT_RETURN;
    c->codecName = (char*) malloc(sizeof(char) * (c->codecNameLen + 1));
    c->status = napi_get_value_string_utf8(env, value, c->codecName,
      64, &c->codecNameLen);
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

create:
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
  HR_TIME_POINT decodeStart = NOW;

  for ( auto it = c->packets.cbegin() ; it != c->packets.cend() ; it++ ) {
  bump:
    ret = avcodec_send_packet(c->decoder, *it);
    switch (ret) {
      case AVERROR(EAGAIN):
        // printf("Input is not accepted in the current state - user must read output with avcodec_receive_frame().\n");
        frame = av_frame_alloc();
        avcodec_receive_frame(c->decoder, frame);
        c->frames.push_back(frame);
        goto bump;
      case AVERROR_EOF:
        c->status = BEAMCODER_ERROR_EOF;
        c->errorMsg = "The decoder has been flushed, and no new packets can be sent to it.";
        return;
      case AVERROR(EINVAL):
        if ((ret = avcodec_open2(c->decoder, c->decoder->codec, nullptr))) {
          c->status = BEAMCODER_ERROR_ALLOC_DECODER;
          c->errorMsg = avErrorMsg("Problem opening decoder: ", ret);
          return;
        }
        goto bump;
      case AVERROR(ENOMEM):
        c->status = BEAMCODER_ERROR_ENOMEM;
        c->errorMsg = "Failed to add packet to internal queue.";
        return;
      case 0:
        // printf("Successfully sent packet to codec.\n");
        break;
      default:
        c->status = BEAMCODER_ERROR_DECODE;
        c->errorMsg = avErrorMsg("Error sending packet: ", ret);
        return;
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

  c->totalTime = microTime(decodeStart);
};

void decodeComplete(napi_env env, napi_status asyncStatus, void* data) {
  decodeCarrier* c = (decodeCarrier*) data;
  napi_value result, value, frames, frame, prop, el;
  int64_t totalMem;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Decoder allocator failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_object(env, &result);
  REJECT_STATUS;
  c->status = beam_set_string_utf8(env, result, "type", "frames");
  REJECT_STATUS;

  c->status = napi_create_array(env, &frames);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "frames", frames);
  REJECT_STATUS;

  uint32_t frameCount = 0;
  for ( auto it = c->frames.begin() ; it != c->frames.end() ; it++ ) {
    AVFrame* item = *it;
    c->status = napi_create_object(env, &frame);
    REJECT_STATUS;

    c->status = beam_set_string_utf8(env, frame, "type", "frame");
    REJECT_STATUS;

    c->status = beam_set_string_utf8(env, frame,
      "media_type", (char*) av_get_media_type_string(c->decoder->codec_type));
    REJECT_STATUS;
    c->status = beam_set_int64(env, frame, "pts", item->pts);
    REJECT_STATUS;
    c->status = beam_set_int64(env, frame, "pkt_dts", item->pkt_dts);
    REJECT_STATUS;

    if (c->decoder->codec_type == AVMEDIA_TYPE_VIDEO) {
      c->status = beam_set_int32(env, frame, "width", item->width);
      REJECT_STATUS;
      c->status = beam_set_int32(env, frame, "height", item->height);
      REJECT_STATUS;
      c->status = beam_set_string_utf8(env, frame, "format",
        (char*) av_get_pix_fmt_name((AVPixelFormat) item->format));
      REJECT_STATUS;

      c->status = napi_create_array(env, &prop);
      REJECT_STATUS;
      for ( int x = 0 ; x < AV_NUM_DATA_POINTERS ; x++ ) {
        if (item->linesize[x] == 0) break;
        c->status = napi_create_int32(env, item->linesize[x], &el);
        REJECT_STATUS;
        c->status = napi_set_element(env, prop, x, el);
        REJECT_STATUS;
      }
      c->status = napi_set_named_property(env, frame, "linesize", prop);
      REJECT_STATUS;

      c->status = beam_set_bool(env, frame, "key_frame", item->key_frame == 1);
      REJECT_STATUS;

      char pt[2];
      pt[0] = av_get_picture_type_char(item->pict_type);
      pt[1] = '\0';
      c->status = beam_set_string_utf8(env, frame, "pict_type", pt);
      REJECT_STATUS;

      if (item->sample_aspect_ratio.num != 0) {
        c->status = beam_set_rational(env, frame, "sample_aspect_ratio", item->sample_aspect_ratio);
        REJECT_STATUS;
      }

      c->status = beam_set_bool(env, frame, "interlaced_frame", item->interlaced_frame != 0);
      REJECT_STATUS;
      if (item->interlaced_frame) {
        c->status = beam_set_bool(env, frame, "top_field_first", item->top_field_first != 0);
        REJECT_STATUS;
      }

      if (item->coded_picture_number > 0) {
        c->status = beam_set_int32(env,  frame, "coded_picture_number", item->coded_picture_number);
        REJECT_STATUS;
      }
      if (item->display_picture_number > 0) {
        c->status = beam_set_int32(env, frame, "display_picture_number", item->display_picture_number);
        REJECT_STATUS;
      }
      if (item->crop_top > 0) {
        c->status = beam_set_int64(env, frame, "crop_top", item->crop_top);
        REJECT_STATUS;
      }
      if (item->crop_bottom > 0) {
        c->status = beam_set_int64(env, frame, "crop_bottom", item->crop_bottom);
        REJECT_STATUS;
      }
      if (item->crop_left > 0) {
        c->status = beam_set_int64(env, frame, "crop_left", item->crop_left);
        REJECT_STATUS;
      }
      if (item->crop_right > 0) {
        c->status = beam_set_int64(env, frame, "crop_right", item->crop_right);
        REJECT_STATUS;
      }
    } // Media type video

    if (c->decoder->codec_type == AVMEDIA_TYPE_AUDIO) {
      c->status = beam_set_int32(env, frame, "nb_samples", item->nb_samples);
      REJECT_STATUS;
      c->status = beam_set_int32(env, frame, "sample_rate", item->sample_rate);
      REJECT_STATUS;

      c->status = beam_set_string_utf8(env, frame, "format",
        (char*) av_get_sample_fmt_name((AVSampleFormat) item->format));
      REJECT_STATUS;

      if (item->channel_layout != 0) {
        char cl[64];
        av_get_channel_layout_string(cl, 64, c->decoder->channels, item->channel_layout);
        c->status = beam_set_string_utf8(env, frame, "channel_layout", (char*) cl);
        REJECT_STATUS;
      }
    }

    if (item->quality != 0) {
      c->status = beam_set_int32(env, frame, "quality", item->quality);
      REJECT_STATUS;
    }

    c->status = napi_create_array(env, &prop);
    REJECT_STATUS;
    for ( int x = 0 ; x < AV_NUM_DATA_POINTERS ; x++ ) {
      if (item->buf[x] == nullptr) break;
      AVBufferRef* mybuf = av_buffer_ref(item->buf[x]);
      c->status = napi_create_external_buffer(env, mybuf->size, mybuf->data,
        frameBufferFinalize, mybuf, &el);
      REJECT_STATUS;
      c->status = napi_set_element(env, prop, x, el);
      REJECT_STATUS;
      c->status = napi_adjust_external_memory(env, mybuf->size, &totalMem);
      REJECT_STATUS;
    }
    c->status = napi_set_named_property(env, frame, "data", prop);
    REJECT_STATUS;

    c->status = napi_create_external(env, item, frameFinalizer, nullptr, &prop);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, frame, "_frame", prop);
    REJECT_STATUS;

    c->status = napi_set_element(env, frames, frameCount++, frame);
    REJECT_STATUS;
  }

  c->status = napi_create_int64(env, c->totalTime, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "totalTime", prop);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
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

void frameFinalizer(napi_env env, void* data, void* hint) {
  AVFrame* frame = (AVFrame*) data;
  // printf("Frame finalizer called.\n");
  av_frame_free(&frame);
}

void frameBufferFinalize(napi_env env, void* data, void* hint) {
  AVBufferRef* hintRef = (AVBufferRef*) hint;
  // printf("Frame buffer finalizer called with %p.\n", hint);
  napi_status status;
  int64_t externalMemory;
  status = napi_adjust_external_memory(env, -hintRef->size, &externalMemory);
  printf("External memory is %i\n", externalMemory);
  if (status != napi_ok) {
    printf("DEBUG: Napi failure to adjust external memory. In beamcoder decode.cc frameBufferFinalizer.");
  }
  av_buffer_unref(&hintRef);
}

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
  if (status != napi_ok) return nullptr;
  status = napi_get_value_external(env, value, (void**) &result);
  if (status != napi_ok) return nullptr;

  return result;
}
