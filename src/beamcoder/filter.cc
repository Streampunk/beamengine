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

#include "filter.h"
#include "beamcoder_util.h"
#include "frame.h"

extern "C" {
  #include <libavfilter/avfilter.h>
  #include <libavformat/avformat.h>
  #include <libavcodec/avcodec.h>
  #include <libavfilter/buffersink.h>
  #include <libavfilter/buffersrc.h>
}

struct filtererCarrier : carrier {
  AVFormatContext *fmtCtx;
  AVCodecContext *decCtx;
  int streamIdx = -1;
  AVFilterContext *srcCtx = nullptr;
  AVFilterContext *sinkCtx = nullptr;
  AVFilterGraph *filterGraph = nullptr;
  char *filterDescription = nullptr;
  ~filtererCarrier() {}
};

void graphFinalizer(napi_env env, void* data, void* hint) {
  AVFilterGraph* graph = (AVFilterGraph*)data;
  avfilter_graph_free(&graph);
}

void filtererExecute(napi_env env, void* data) {
  filtererCarrier* c = (filtererCarrier*) data;

  char args[512];
  int ret = 0;
  const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
  const AVFilter *buffersink = avfilter_get_by_name("buffersink");
  AVFilterInOut *outputs = avfilter_inout_alloc();
  AVFilterInOut *inputs  = avfilter_inout_alloc();

  c->filterGraph = avfilter_graph_alloc();
  if (!outputs || !inputs || !c->filterGraph) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to allocate filter resources.";
    goto end;
  }

  AVRational time_base = c->fmtCtx->streams[c->streamIdx]->time_base;
  snprintf(args, sizeof(args),
          "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
          c->decCtx->width, c->decCtx->height, c->decCtx->pix_fmt,
          time_base.num, time_base.den,
          c->decCtx->sample_aspect_ratio.num, c->decCtx->sample_aspect_ratio.den);
  ret = avfilter_graph_create_filter(&c->srcCtx, buffersrc, "in", args, NULL, c->filterGraph);
  if (ret < 0) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to allocate source filter graph.";
    goto end;
  }

  ret = avfilter_graph_create_filter(&c->sinkCtx, buffersink, "out", NULL, NULL, c->filterGraph);
  if (ret < 0) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to allocate sink filter graph.";
    goto end;
  }

  outputs->name       = av_strdup("in");
  outputs->filter_ctx = c->srcCtx;
  outputs->pad_idx    = 0;
  outputs->next       = NULL;

  inputs->name       = av_strdup("out");
  inputs->filter_ctx = c->sinkCtx;
  inputs->pad_idx    = 0;
  inputs->next       = NULL;

  if ((ret = avfilter_graph_parse_ptr(c->filterGraph, c->filterDescription, &inputs, &outputs, NULL)) < 0)
    goto end;

  if ((ret = avfilter_graph_config(c->filterGraph, NULL)) < 0)
    goto end;

end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);
}

void filtererComplete(napi_env env, napi_status asyncStatus, void* data) {
  filtererCarrier* c = (filtererCarrier*) data;
  napi_value result, value;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Filterer allocator failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_object(env, &result);
  REJECT_STATUS;

  c->status = beam_set_string_utf8(env, result, "type", "filterer");
  REJECT_STATUS;

  c->status = napi_create_external(env, c->filterGraph, graphFinalizer, nullptr, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "graph", value);
  REJECT_STATUS;

  c->status = napi_create_external(env, c->srcCtx, nullptr, nullptr, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "sourceContext", value);
  REJECT_STATUS;

  c->status = napi_create_external(env, c->sinkCtx, nullptr, nullptr, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "sinkContext", value);
  REJECT_STATUS;

  c->status = napi_create_function(env, "filter", NAPI_AUTO_LENGTH, filter, nullptr, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "filter", value);
  REJECT_STATUS;

  napi_status status;
  status = napi_resolve_deferred(env, c->_deferred, result);
  FLOATING_STATUS;

  tidyCarrier(env, c);
}

napi_value filterer(napi_env env, napi_callback_info info) {
  napi_value resourceName, promise;
  napi_valuetype type;
  filtererCarrier* c = new filtererCarrier;
  bool isArray;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];

  c->status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  REJECT_RETURN;

  if (argc != 1) {
    REJECT_ERROR_RETURN("Filterer requires a single options object.",
      BEAMCODER_INVALID_ARGS);
  }

  c->status = napi_typeof(env, args[0], &type);
  REJECT_RETURN;
  c->status = napi_is_array(env, args[0], &isArray);
  REJECT_RETURN;
  if ((type != napi_object) || (isArray == true)) {
    REJECT_ERROR_RETURN("Filterer must be configured with a single parameter, an options object.",
      BEAMCODER_INVALID_ARGS);
  }

  bool hasFormat, hasDecoder, hasDescription;
  c->status = napi_has_named_property(env, args[0], "format", &hasFormat);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "decoder", &hasDecoder);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "description", &hasDescription);
  REJECT_RETURN;

  if (!(hasFormat && hasDecoder && hasDescription)) {
    REJECT_ERROR_RETURN("Filterer requires a format, decoder and a description.",
      BEAMCODER_INVALID_ARGS);
  }

  napi_value formatJS, formatExt;
  c->status = napi_get_named_property(env, args[0], "format", &formatJS);
  REJECT_RETURN;
  c->status = napi_get_named_property(env, formatJS, "_formatContext", &formatExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, formatExt, (void**)&c->fmtCtx);
  REJECT_RETURN;

  napi_value decoderJS, decoderExt, streamExt;
  c->status = napi_get_named_property(env, args[0], "decoder", &decoderJS);
  REJECT_RETURN;
  c->status = napi_get_named_property(env, decoderJS, "_decoder", &decoderExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, decoderExt, (void**)&c->decCtx);
  REJECT_RETURN;
  c->status = napi_get_named_property(env, decoderJS, "stream", &streamExt);
  REJECT_RETURN;
  c->status = napi_get_value_int32(env, streamExt, &c->streamIdx);
  REJECT_RETURN;

  if (c->streamIdx < 0 || c->streamIdx >= (int)c->fmtCtx->nb_streams) {
    REJECT_ERROR_RETURN("Stream index is out of bounds for the given format.",
      BEAMCODER_ERROR_OUT_OF_BOUNDS);
  }

  napi_value descriptionJS;
  c->status = napi_get_named_property(env, args[0], "description", &descriptionJS);
  REJECT_RETURN;
  size_t descrLen;
  c->status = napi_get_value_string_utf8(env, descriptionJS, nullptr, 0, &descrLen);
  REJECT_RETURN;
  c->filterDescription = (char*)malloc(sizeof(char) * (descrLen + 1));
  c->status = napi_get_value_string_utf8(env, descriptionJS, c->filterDescription, descrLen + 1, nullptr);
  REJECT_RETURN;

  c->status = napi_create_string_utf8(env, "Filterer", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, filtererExecute,
    filtererComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
}


struct filterCarrier : carrier {
  AVFilterContext *srcCtx = nullptr;
  AVFilterContext *sinkCtx = nullptr;
  std::vector<AVFrame*> srcFrames;
  std::vector<AVFrame*> dstFrames;
  ~filterCarrier() {
    // printf("Filter carrier destructor.\n");
  }
};

namespace {
  napi_status isFrame(napi_env env, napi_value packet) {
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

    status = napi_has_named_property(env, packet, "_frame", &result);
    if ((status != napi_ok) || (result == false)) return napi_invalid_arg;

    status = napi_get_named_property(env, packet, "type", &value);
    if (status != napi_ok) return status;
    status = napi_get_value_string_utf8(env, value, objType, 10, &typeLen);
    if (status != napi_ok) return status;
    cmp = strcmp("Frame", objType);
    if (cmp != 0) return napi_invalid_arg;

    status = napi_get_named_property(env, packet, "_frame", &value);
    if (status != napi_ok) return status;
    status = napi_typeof(env, value, &type);
    if (status != napi_ok) return status;
    if (type != napi_external) return napi_invalid_arg;

    return napi_ok;
  }

  AVFrame* getFrame(napi_env env, napi_value frame) {
    napi_status status;
    napi_value value;
    frameData* result = nullptr;
    status = napi_get_named_property(env, frame, "_frame", &value);
    if (status != napi_ok) return nullptr;
    status = napi_get_value_external(env, value, (void**)&result);
    if (status != napi_ok) return nullptr;

    return result->frame;
  }
}

void filterExecute(napi_env env, void* data) {
  filterCarrier* c = (filterCarrier*) data;
  int ret = 0;
  HR_TIME_POINT filterStart = NOW;

  for (auto it = c->srcFrames.cbegin(); it != c->srcFrames.cend(); ++it) {

    if (av_buffersrc_add_frame_flags(c->srcCtx, *it, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
      c->status = BEAMCODER_ERROR_ENOMEM;
      c->errorMsg = "Error while feeding the filtergraph.";
      return;
    }
  }

  while (1) {
    AVFrame *filtFrame = av_frame_alloc();
    ret = av_buffersink_get_frame(c->sinkCtx, filtFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    if (ret < 0) {
      c->status = BEAMCODER_ERROR_ENOMEM;
      c->errorMsg = "Error while filtering.";
      break;
    }
    c->dstFrames.push_back(filtFrame);
  }
  c->totalTime = microTime(filterStart);
};

void filterComplete(napi_env env, napi_status asyncStatus, void* data) {
  filterCarrier* c = (filterCarrier*) data;
  napi_value result, frames, frame, prop;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Filter failed to complete.";
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
  for ( auto it = c->dstFrames.begin() ; it != c->dstFrames.end() ; it++ ) {
    frameData* f = new frameData;
    f->frame = *it;

    c->status = frameFromAVFrame(env, f, &frame);
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

napi_value filter(napi_env env, napi_callback_info info) {
  napi_value resourceName, promise, filtererJS;
  filterCarrier* c = new filterCarrier;

  c->status = napi_create_promise(env, &c->_deferred, &promise);
  REJECT_RETURN;

  size_t argc = 1;
  napi_value args[1];

  c->status = napi_get_cb_info(env, info, &argc, args, &filtererJS, nullptr);
  REJECT_RETURN;

  napi_value srcCtxExt, sinkCtxExt;
  c->status = napi_get_named_property(env, filtererJS, "sourceContext", &srcCtxExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, srcCtxExt, (void**)&c->srcCtx);
  REJECT_RETURN;
  c->status = napi_get_named_property(env, filtererJS, "sinkContext", &sinkCtxExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, sinkCtxExt, (void**)&c->sinkCtx);
  REJECT_RETURN;

  if (argc != 1) {
    REJECT_ERROR_RETURN("Filter requires frame object.",
      BEAMCODER_INVALID_ARGS);
  }

  napi_value frames;
  c->status = napi_get_named_property(env, args[0], "frames", &frames);
  REJECT_RETURN;

  bool isArray;
  c->status = napi_is_array(env, frames, &isArray);
  REJECT_RETURN;
  if (!isArray)
    REJECT_ERROR_RETURN("Expected an array of frames.",
      BEAMCODER_INVALID_ARGS);

  uint32_t framesLength;
  c->status = napi_get_array_length(env, frames, &framesLength);
  REJECT_RETURN;
  for (uint32_t x = 0 ; x < framesLength ; x++) {
    napi_value item;
    c->status = napi_get_element(env, frames, x, &item);
    REJECT_RETURN;
    c->status = isFrame(env, item);
    if (c->status != napi_ok) {
      REJECT_ERROR_RETURN("All passed frames in an array must be of type frame.",
        BEAMCODER_INVALID_ARGS);
    }
    c->srcFrames.push_back(getFrame(env, item));
  }

  c->status = napi_create_string_utf8(env, "Filter", NAPI_AUTO_LENGTH, &resourceName);
  REJECT_RETURN;
  c->status = napi_create_async_work(env, nullptr, resourceName, filterExecute,
    filterComplete, c, &c->_request);
  REJECT_RETURN;
  c->status = napi_queue_async_work(env, c->_request);
  REJECT_RETURN;

  return promise;
};
