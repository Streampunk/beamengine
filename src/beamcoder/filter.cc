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
#include <map>
#include <deque>

extern "C" {
  #include <libavfilter/avfilter.h>
  #include <libavformat/avformat.h>
  #include <libavcodec/avcodec.h>
  #include <libavfilter/buffersink.h>
  #include <libavfilter/buffersrc.h>
}

class srcContexts {
public:
  srcContexts() {}
  ~srcContexts() {}

  bool add(const std::string &name, AVFilterContext *context) {
    auto result = mSrcContexts.emplace(name, context);
    return result.second;
  }

  AVFilterContext *getContext(const std::string &name) const {
    AVFilterContext *result = nullptr;
    auto &c = mSrcContexts.find(name);
    if (c != mSrcContexts.end())
      result = c->second;
    return result;
  }

private:
  std::map<std::string, AVFilterContext *> mSrcContexts;
};

struct filtererCarrier : carrier {
  std::vector<std::string> inName;
  std::vector<std::string> inParams;
  std::string filterSpec;

  srcContexts *srcCtxs;
  AVFilterContext *sinkCtx = nullptr;
  AVFilterGraph *filterGraph = nullptr;
  ~filtererCarrier() {}
};

void graphFinalizer(napi_env env, void* data, void* hint) {
  AVFilterGraph* graph = (AVFilterGraph*)data;
  avfilter_graph_free(&graph);
}

void srcCtxsFinalizer(napi_env env, void* data, void* hint) {
  srcContexts* srcCtxs = (srcContexts*)data;
  delete srcCtxs;
}

void filtererExecute(napi_env env, void* data) {
  filtererCarrier* c = (filtererCarrier*) data;
  int ret = 0;

  c->filterGraph = avfilter_graph_alloc();
  AVFilterInOut *inputs  = avfilter_inout_alloc();

  AVFilterInOut **outputs = new AVFilterInOut*[c->inParams.size()];
  bool opAlloc = true;
  for (size_t i = 0; i < c->inParams.size(); ++i)
    if (!(opAlloc = (outputs[i] = avfilter_inout_alloc()) != NULL)) break;

  if (!(opAlloc && inputs && c->filterGraph)) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to allocate filter resources.";
    goto end;
  }

  const AVFilter *buffersink = avfilter_get_by_name("buffersink");
  ret = avfilter_graph_create_filter(&c->sinkCtx, buffersink, "out", NULL, NULL, c->filterGraph);
  if (ret < 0) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to allocate sink filter graph.";
    goto end;
  }
  inputs->name       = av_strdup("out");
  inputs->filter_ctx = c->sinkCtx;
  inputs->pad_idx    = 0;
  inputs->next       = NULL;

  c->srcCtxs = new srcContexts;
  for (size_t i = 0; i < c->inParams.size(); ++i) {
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilterContext *srcCtx = nullptr;
    ret = avfilter_graph_create_filter(&srcCtx, buffersrc, "in", c->inParams[i].c_str(), NULL, c->filterGraph);
    if (ret < 0) {
      c->status = BEAMCODER_ERROR_ENOMEM;
      c->errorMsg = "Failed to allocate source filter graph.";
      goto end;
    }
    outputs[i]->name       = av_strdup(c->inName[i].c_str());
    outputs[i]->filter_ctx = srcCtx;
    outputs[i]->pad_idx    = 0;
    outputs[i]->next       = i < c->inParams.size() - 1 ? outputs[i + 1] : NULL;

    if (!c->srcCtxs->add(c->inName[i], srcCtx)) {
      c->status = BEAMCODER_ERROR_EINVAL;
      c->errorMsg = "Filter sources must have unique names.";
      goto end;
    }
  }

  if ((ret = avfilter_graph_parse_ptr(c->filterGraph, c->filterSpec.c_str(), &inputs, outputs, NULL)) < 0) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to parse filter graph.";
    goto end;
  }

  if ((ret = avfilter_graph_config(c->filterGraph, NULL)) < 0) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to configure filter graph.";
    goto end;
  }

end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(outputs);
  delete outputs;
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

  c->status = napi_create_external(env, c->srcCtxs, srcCtxsFinalizer, nullptr, &value);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "sourceContexts", value);
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

  bool hasInParams, hasFilterSpec;
  c->status = napi_has_named_property(env, args[0], "inputParams", &hasInParams);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "filterSpec", &hasFilterSpec);
  REJECT_RETURN;

  if (!(hasInParams && hasFilterSpec)) {
    REJECT_ERROR_RETURN("Filterer parameter object requires inputParams and filterSpec to be defined.",
      BEAMCODER_INVALID_ARGS);
  }

  napi_value paramsArrayVal;
  c->status = napi_get_named_property(env, args[0], "inputParams", &paramsArrayVal);
  REJECT_RETURN;
  c->status = napi_is_array(env, paramsArrayVal, &isArray);
  REJECT_RETURN;
  if (!isArray) {
    REJECT_ERROR_RETURN("Filterer inputParams must be an array.",
      BEAMCODER_INVALID_ARGS);
  }
  uint32_t paramsArrayLen;
  c->status = napi_get_array_length(env, paramsArrayVal, &paramsArrayLen);
  REJECT_RETURN;

  for (uint32_t i = 0; i < paramsArrayLen; ++i) {
    napi_value inParamsVal;
    c->status = napi_get_element(env, paramsArrayVal, i, &inParamsVal);
    REJECT_RETURN;

    std::string name;
    uint32_t width;
    uint32_t height;
    std::string pixFmt;
    AVRational timeBase;
    AVRational pixelAspect;

    bool hasNameVal;
    c->status = napi_has_named_property(env, inParamsVal, "name", &hasNameVal);
    REJECT_RETURN;
    if (!hasNameVal && (i > 0)) {
      REJECT_ERROR_RETURN("Filterer inputParams must include a name value if there is more than one input.",
        BEAMCODER_INVALID_ARGS);
    }
    if (hasNameVal) {
      napi_value nameVal;
      c->status = napi_get_named_property(env, inParamsVal, "name", &nameVal);
      REJECT_RETURN;
      size_t nameLen;
      c->status = napi_get_value_string_utf8(env, nameVal, nullptr, 0, &nameLen);
      REJECT_RETURN;
      name.resize(nameLen+1);
      c->status = napi_get_value_string_utf8(env, nameVal, (char *)name.data(), nameLen+1, nullptr);
      REJECT_RETURN;
      c->inName.push_back(name);
    } else
      c->inName.push_back("in");

    napi_value widthVal;
    c->status = napi_get_named_property(env, inParamsVal, "width", &widthVal);
    REJECT_RETURN;
    c->status = napi_get_value_uint32(env, widthVal, &width);
    REJECT_RETURN;
    napi_value heightVal;
    c->status = napi_get_named_property(env, inParamsVal, "height", &heightVal);
    REJECT_RETURN;
    c->status = napi_get_value_uint32(env, heightVal, &height);
    REJECT_RETURN;

    napi_value pixFmtVal;
    c->status = napi_get_named_property(env, inParamsVal, "pixFmt", &pixFmtVal);
    REJECT_RETURN;
    size_t pixFmtLen;
    c->status = napi_get_value_string_utf8(env, pixFmtVal, nullptr, 0, &pixFmtLen);
    REJECT_RETURN;
    pixFmt.resize(pixFmtLen+1);
    c->status = napi_get_value_string_utf8(env, pixFmtVal, (char *)pixFmt.data(), pixFmtLen+1, nullptr);
    REJECT_RETURN;

    napi_value timeBaseVal;
    c->status = napi_get_named_property(env, inParamsVal, "timeBase", &timeBaseVal);
    REJECT_RETURN;
    c->status = napi_is_array(env, timeBaseVal, &isArray);
    REJECT_RETURN;
    uint32_t arrayLen;
    if (isArray) {
      c->status = napi_get_array_length(env, timeBaseVal, &arrayLen);
      REJECT_RETURN;
    }
    if (!(isArray && (2 == arrayLen))) {
      REJECT_ERROR_RETURN("Filterer inputParams timeBase must be an array with 2 values representing a rational number.",
        BEAMCODER_INVALID_ARGS);
    }
    for (uint32_t i = 0; i < arrayLen; ++i) {
      napi_value arrayVal;
      c->status = napi_get_element(env, timeBaseVal, i, &arrayVal);
      REJECT_RETURN;
      c->status = napi_get_value_int32(env, arrayVal, (0==i)?&timeBase.num:&timeBase.den);
      REJECT_RETURN;
    }

    napi_value pixelAspectVal;
    c->status = napi_get_named_property(env, inParamsVal, "pixelAspect", &pixelAspectVal);
    REJECT_RETURN;
    c->status = napi_is_array(env, pixelAspectVal, &isArray);
    REJECT_RETURN;
    if (isArray) {
      c->status = napi_get_array_length(env, pixelAspectVal, &arrayLen);
      REJECT_RETURN;
    }
    if (!(isArray && (2 == arrayLen))) {
      REJECT_ERROR_RETURN("Filterer inputParams pixelAspect must be an array with 2 values representing a rational number.",
        BEAMCODER_INVALID_ARGS);
    }
    for (uint32_t i = 0; i < arrayLen; ++i) {
      napi_value arrayVal;
      c->status = napi_get_element(env, pixelAspectVal, i, &arrayVal);
      REJECT_RETURN;
      c->status = napi_get_value_int32(env, arrayVal, (0==i)?&pixelAspect.num:&pixelAspect.den);
      REJECT_RETURN;
    }

    char args[512];
    snprintf(args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            width, height, av_get_pix_fmt(pixFmt.c_str()),
            timeBase.num, timeBase.den, pixelAspect.num, pixelAspect.den);
    c->inParams.push_back(args);
  }

  napi_value filterSpecJS;
  c->status = napi_get_named_property(env, args[0], "filterSpec", &filterSpecJS);
  REJECT_RETURN;
  size_t specLen;
  c->status = napi_get_value_string_utf8(env, filterSpecJS, nullptr, 0, &specLen);
  REJECT_RETURN;
  c->filterSpec.resize(specLen+1);
  c->status = napi_get_value_string_utf8(env, filterSpecJS, (char *)c->filterSpec.data(), specLen+1, nullptr);
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
  srcContexts *srcCtxs;
  AVFilterContext *sinkCtx = nullptr;
  std::map<std::string, std::deque<AVFrame *> > srcFrames;
  std::vector<AVFrame *> dstFrames;
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

  for (auto it = c->srcFrames.begin(); it != c->srcFrames.end(); ++it) {
    AVFilterContext *srcCtx = c->srcCtxs->getContext(it->first);
    if (!srcCtx) {
      c->status = BEAMCODER_INVALID_ARGS;
      c->errorMsg = "Frame name not found in source contexts.";
      return;
    }

    std::deque<AVFrame *> frames = it->second;
    while (frames.size() > 0) {
      ret = av_buffersrc_add_frame_flags(srcCtx, frames.front(), AV_BUFFERSRC_FLAG_KEEP_REF);
      if (ret < 0) {
        c->status = BEAMCODER_ERROR_FILTER_ADD_FRAME;
        c->errorMsg = "Error while feeding the filtergraph.";
        return;
      }
      frames.pop_front();
    }
  }

  while (1) {
    AVFrame *filtFrame = av_frame_alloc();
    ret = av_buffersink_get_frame(c->sinkCtx, filtFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      break;
    if (ret < 0) {
      c->status = BEAMCODER_ERROR_FILTER_GET_FRAME;
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

    c->status = fromAVFrame(env, f, &frame);
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

  napi_value srcCtxsExt, sinkCtxExt;
  c->status = napi_get_named_property(env, filtererJS, "sourceContexts", &srcCtxsExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, srcCtxsExt, (void**)&c->srcCtxs);
  REJECT_RETURN;

  c->status = napi_get_named_property(env, filtererJS, "sinkContext", &sinkCtxExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, sinkCtxExt, (void**)&c->sinkCtx);
  REJECT_RETURN;

  if (argc != 1) {
    REJECT_ERROR_RETURN("Filter requires source frame object.",
      BEAMCODER_INVALID_ARGS);
  }

  bool isArray;
  c->status = napi_is_array(env, args[0], &isArray);
  REJECT_RETURN;
  if (!isArray)
    REJECT_ERROR_RETURN("Expected an array of source frame objects.",
      BEAMCODER_INVALID_ARGS);

  uint32_t srcsLen;
  c->status = napi_get_array_length(env, args[0], &srcsLen);
  REJECT_RETURN;
  for (uint32_t i = 0; i < srcsLen; ++i) {
    napi_value item;
    c->status = napi_get_element(env, args[0], i, &item);
    REJECT_RETURN;

    std::string name;
    bool hasName;
    c->status = napi_has_named_property(env, item, "name", &hasName);
    REJECT_RETURN;
    if (hasName) {
      napi_value nameVal;
      c->status = napi_get_named_property(env, item, "name", &nameVal);
      REJECT_RETURN;
      size_t nameLen;
      c->status = napi_get_value_string_utf8(env, nameVal, nullptr, 0, &nameLen);
      REJECT_RETURN;
      name.resize(nameLen+1);
      c->status = napi_get_value_string_utf8(env, nameVal, (char *)name.data(), nameLen+1, nullptr);
      REJECT_RETURN;
    } else if (0 == i) {
      name = "in";
    } else {
      REJECT_ERROR_RETURN("Source frame object requires a name.",
        BEAMCODER_INVALID_ARGS);
    }
      
    napi_value framesVal;
    c->status = napi_get_named_property(env, item, "frames", &framesVal);
    REJECT_RETURN;

    napi_value framesArrVal;
    c->status = napi_get_named_property(env, framesVal, "frames", &framesArrVal);
    REJECT_RETURN;

    bool isArray;
    c->status = napi_is_array(env, framesArrVal, &isArray);
    REJECT_RETURN;
    if (!isArray)
      REJECT_ERROR_RETURN("Expected an array of frame objects.",
        BEAMCODER_INVALID_ARGS);

    uint32_t framesLen;
    c->status = napi_get_array_length(env, framesArrVal, &framesLen);
    REJECT_RETURN;
    std::deque<AVFrame *> frames;
    for (uint32_t f = 0; f < framesLen; ++f) {
      napi_value item;
      c->status = napi_get_element(env, framesArrVal, f, &item);
      REJECT_RETURN;
      c->status = isFrame(env, item);
      if (c->status != napi_ok) {
        REJECT_ERROR_RETURN("Values in array must by of type frame.",
          BEAMCODER_INVALID_ARGS);
      }
      frames.push_back(getFrame(env, item));
    }
    auto result = c->srcFrames.emplace(name, frames);
    if (!result.second) {
      REJECT_ERROR_RETURN("Frame names must be unique.",
        BEAMCODER_INVALID_ARGS);
    }
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
