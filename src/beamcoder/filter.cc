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

class filtContexts {
public:
  filtContexts() {}
  ~filtContexts() {}

  bool add(const std::string &name, AVFilterContext *context) {
    mContextNames.push_back(name);
    auto result = mFiltContexts.emplace(name, context);
    return result.second;
  }

  AVFilterContext *getContext(const std::string &name) const {
    AVFilterContext *result = nullptr;
    auto &c = mFiltContexts.find(name);
    if (c != mFiltContexts.end())
      result = c->second;
    return result;
  }

  const std::vector<std::string>& getNames() const  { return mContextNames; }

private:
  std::map<std::string, AVFilterContext *> mFiltContexts;
  std::vector<std::string> mContextNames;
};

struct filtererCarrier : carrier {
  std::string filterType;
  std::vector<std::string> inNames;
  std::vector<std::string> outNames;
  std::vector<std::string> inParams;
  std::string filterSpec;

  filtContexts *srcCtxs = nullptr;
  filtContexts *sinkCtxs = nullptr;
  AVFilterGraph *filterGraph = nullptr;
  ~filtererCarrier() {}
};

void graphFinalizer(napi_env env, void* data, void* hint) {
  AVFilterGraph* graph = (AVFilterGraph*)data;
  avfilter_graph_free(&graph);
}

void ctxsFinalizer(napi_env env, void* data, void* hint) {
  filtContexts* ctxs = (filtContexts*)data;
  delete ctxs;
}

void filtererExecute(napi_env env, void* data) {
  filtererCarrier* c = (filtererCarrier*) data;
  int ret = 0;

  c->filterGraph = avfilter_graph_alloc();

  AVFilterInOut **inputs = new AVFilterInOut*[c->outNames.size()];
  bool inAlloc = true;
  for (size_t i = 0; i < c->outNames.size(); ++i)
    if (!(inAlloc = (inputs[i] = avfilter_inout_alloc()) != NULL)) break;

  AVFilterInOut **outputs = new AVFilterInOut*[c->inParams.size()];
  bool opAlloc = true;
  for (size_t i = 0; i < c->inParams.size(); ++i)
    if (!(opAlloc = (outputs[i] = avfilter_inout_alloc()) != NULL)) break;

  if (!(inAlloc && opAlloc && c->filterGraph)) {
    c->status = BEAMCODER_ERROR_ENOMEM;
    c->errorMsg = "Failed to allocate filter resources.";
    goto end;
  }

  c->srcCtxs = new filtContexts;
  for (size_t i = 0; i < c->inParams.size(); ++i) {
    const AVFilter *buffersrc  = avfilter_get_by_name(0 == c->filterType.compare("audio")?"abuffer":"buffer");
    AVFilterContext *srcCtx = nullptr;
    ret = avfilter_graph_create_filter(&srcCtx, buffersrc, c->inNames[i].c_str(), c->inParams[i].c_str(), NULL, c->filterGraph);
    if (ret < 0) {
      c->status = BEAMCODER_ERROR_ENOMEM;
      c->errorMsg = "Failed to allocate source filter graph.";
      goto end;
    }
    outputs[i]->name       = av_strdup(c->inNames[i].c_str());
    outputs[i]->filter_ctx = srcCtx;
    outputs[i]->pad_idx    = 0;
    outputs[i]->next       = i < c->inParams.size() - 1 ? outputs[i + 1] : NULL;

    if (!c->srcCtxs->add(c->inNames[i], srcCtx)) {
      c->status = BEAMCODER_ERROR_EINVAL;
      c->errorMsg = "Filter sources must have unique names.";
      goto end;
    }
  }

  c->sinkCtxs = new filtContexts;
  for (size_t i = 0; i < c->outNames.size(); ++i) {
    const AVFilter *buffersink = avfilter_get_by_name(0 == c->filterType.compare("audio")?"abuffersink":"buffersink");
    AVFilterContext *sinkCtx = nullptr;
    ret = avfilter_graph_create_filter(&sinkCtx, buffersink, c->outNames[i].c_str(), NULL, NULL, c->filterGraph);
    if (ret < 0) {
      c->status = BEAMCODER_ERROR_ENOMEM;
      c->errorMsg = "Failed to allocate sink filter graph.";
      goto end;
    }
    inputs[i]->name       = av_strdup(c->outNames[i].c_str());
    inputs[i]->filter_ctx = sinkCtx;
    inputs[i]->pad_idx    = 0;
    inputs[i]->next       = i < c->outNames.size() - 1 ? inputs[i + 1] : NULL;

    if (!c->sinkCtxs->add(c->outNames[i], sinkCtx)) {
      c->status = BEAMCODER_ERROR_EINVAL;
      c->errorMsg = "Filter sinks must have unique names.";
      goto end;
    }
  }

  if ((ret = avfilter_graph_parse_ptr(c->filterGraph, c->filterSpec.c_str(), inputs, outputs, NULL)) < 0) {
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
  avfilter_inout_free(inputs);
  avfilter_inout_free(outputs);
  delete outputs;
}

napi_value getFilterName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilter* filter;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filter);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, filter->name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getFilterDesc(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilter* filter;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filter);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, filter->description, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getFilterPrivClass(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilter* filter;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filter);
  CHECK_STATUS;

  if (filter->priv_class != nullptr)
    status = fromAVClass(env, filter->priv_class, &result);
  else
    status = napi_get_null(env, &result);
  CHECK_STATUS;

  return result;
}

napi_status fromAVFilter(napi_env env, const AVFilter* filter, napi_value* result) {
  napi_status status;
  napi_value typeName;

  status = napi_create_object(env, result);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "Filter", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "name", nullptr, nullptr, getFilterName, nullptr, nullptr, napi_enumerable, (void*)filter },
    { "description", nullptr, nullptr, getFilterDesc, nullptr, nullptr, napi_enumerable, (void*)filter },
    { "priv_class", nullptr, nullptr, getFilterPrivClass, nullptr, nullptr, napi_enumerable, (void*)filter },
  };
  status = napi_define_properties(env, *result, 4, desc);
  PASS_STATUS;

  return napi_ok;
}

napi_value getFilter(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterContext* filterContext;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterContext);
  CHECK_STATUS;

  status = fromAVFilter(env, filterContext->filter, &result);
  CHECK_STATUS;

  return result;
}

napi_value getFilterContextName(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterContext* filterContext;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterContext);
  CHECK_STATUS;

  status = napi_create_string_utf8(env, filterContext->name, NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getNumInputs(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterContext* filterContext;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterContext);
  CHECK_STATUS;

  status = napi_create_uint32(env, filterContext->nb_inputs, &result);
  CHECK_STATUS;

  return result;
}

napi_value getNumOutputs(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterContext* filterContext;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterContext);
  CHECK_STATUS;

  status = napi_create_uint32(env, filterContext->nb_outputs, &result);
  CHECK_STATUS;

  return result;
}

napi_value getNumThreads(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterContext* filterContext;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterContext);
  CHECK_STATUS;

  status = napi_create_uint32(env, filterContext->nb_threads, &result);
  CHECK_STATUS;

  return result;
}

napi_value getReady(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterContext* filterContext;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterContext);
  CHECK_STATUS;

  status = napi_create_uint32(env, filterContext->ready, &result);
  CHECK_STATUS;

  return result;
}

napi_value getExtraHwFrames(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterContext* filterContext;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterContext);
  CHECK_STATUS;

  status = napi_create_int32(env, filterContext->extra_hw_frames, &result);
  CHECK_STATUS;

  return result;
}

napi_status fromAVFilterCtx(napi_env env, AVFilterContext* filtCtx, napi_value* result) {
  napi_status status;
  napi_value typeName;

  status = napi_create_object(env, result);
  PASS_STATUS;
  status = napi_create_string_utf8(env, "FilterContext", NAPI_AUTO_LENGTH, &typeName);
  PASS_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "filter", nullptr, nullptr, getFilter, nullptr, nullptr, napi_enumerable, filtCtx },
    { "name", nullptr, nullptr, getFilterContextName, nullptr, nullptr, napi_enumerable, filtCtx },
    { "nb_inputs", nullptr, nullptr, getNumInputs, nullptr, nullptr, napi_enumerable, filtCtx },
    { "nb_outputs", nullptr, nullptr, getNumOutputs, nullptr, nullptr, napi_enumerable, filtCtx },
    { "nb_threads", nullptr, nullptr, getNumThreads, nullptr, nullptr, napi_enumerable, filtCtx },
    { "ready", nullptr, nullptr, getReady, nullptr, nullptr, napi_enumerable, filtCtx },
    { "extra_hw_frames", nullptr, nullptr, getExtraHwFrames, nullptr, nullptr, napi_enumerable, filtCtx }
  };
  status = napi_define_properties(env, *result, 8, desc);
  PASS_STATUS;

  return napi_ok;
}

napi_value getGraphFilters(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, element;
  AVFilterGraph* filterGraph;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterGraph);
  CHECK_STATUS;

  status = napi_create_array(env, &result);
  CHECK_STATUS;

  for ( uint32_t f = 0 ; f < filterGraph->nb_filters ; ++f ) {
    if (filterGraph->filters[f] == nullptr) continue;
    status = fromAVFilterCtx(env, filterGraph->filters[f], &element);
    CHECK_STATUS;
    status = napi_set_element(env, result, f, element);
    CHECK_STATUS;
  }

  return result;
}

napi_value dumpGraph(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result;
  AVFilterGraph* filterGraph;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterGraph);
  CHECK_STATUS;

  status = napi_create_string_utf8(env,
    avfilter_graph_dump(filterGraph, nullptr),
    NAPI_AUTO_LENGTH, &result);
  CHECK_STATUS;

  return result;
}

napi_value getFilterGraph(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value result, typeName;
  AVFilterGraph* filterGraph;

  status = napi_get_cb_info(env, info, nullptr, nullptr, nullptr, (void**) &filterGraph);
  CHECK_STATUS;

  status = napi_create_object(env, &result);
  CHECK_STATUS;
  status = napi_create_string_utf8(env, "filterGraph", NAPI_AUTO_LENGTH, &typeName);
  CHECK_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "filters", nullptr, nullptr, getGraphFilters, nullptr, nullptr, napi_enumerable, filterGraph },
    { "dump", nullptr, dumpGraph, nullptr, nullptr, nullptr, napi_enumerable, filterGraph }
  };

  status = napi_define_properties(env, result, 3, desc);
  CHECK_STATUS;

  return result;
}

void filtererComplete(napi_env env, napi_status asyncStatus, void* data) {
  filtererCarrier* c = (filtererCarrier*) data;
  napi_value result, typeName, filterGraphValue, srcContextsValue, sinkContextsValue;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Filterer allocator failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_object(env, &result);
  REJECT_STATUS;
  c->status = napi_create_string_utf8(env, "filterer", NAPI_AUTO_LENGTH, &typeName);
  REJECT_STATUS;
  
  c->status = napi_create_external(env, c->filterGraph, graphFinalizer, nullptr, &filterGraphValue);
  REJECT_STATUS;
  c->status = napi_create_external(env, c->srcCtxs, ctxsFinalizer, nullptr, &srcContextsValue);
  REJECT_STATUS;
  c->status = napi_create_external(env, c->sinkCtxs, ctxsFinalizer, nullptr, &sinkContextsValue);
  REJECT_STATUS;

  napi_property_descriptor desc[] = {
    { "type", nullptr, nullptr, nullptr, nullptr, typeName, napi_enumerable, nullptr },
    { "graph", nullptr, nullptr, getFilterGraph, nullptr, nullptr, napi_enumerable, c->filterGraph },
    { "filter", nullptr, filter, nullptr, nullptr, nullptr, napi_enumerable, nullptr },
    { "_filterGraph", nullptr, nullptr, nullptr, nullptr, filterGraphValue, napi_default, nullptr },
    { "_sourceContexts", nullptr, nullptr, nullptr, nullptr, srcContextsValue, napi_default, nullptr },
    { "_sinkContexts", nullptr, nullptr, nullptr, nullptr, sinkContextsValue, napi_default, nullptr }
  };
  c->status = napi_define_properties(env, result, 6, desc);
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

  bool hasFilterType, hasInParams, hasOutNames, hasFilterSpec;
  c->status = napi_has_named_property(env, args[0], "filterType", &hasFilterType);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "inputParams", &hasInParams);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "outputNames", &hasOutNames);
  REJECT_RETURN;
  c->status = napi_has_named_property(env, args[0], "filterSpec", &hasFilterSpec);
  REJECT_RETURN;

  if (!(hasFilterType && hasInParams && hasFilterSpec)) {
    REJECT_ERROR_RETURN("Filterer parameter object requires type, inputParams and filterSpec to be defined.",
      BEAMCODER_INVALID_ARGS);
  }

  napi_value filterTypeVal;
  c->status = napi_get_named_property(env, args[0], "filterType", &filterTypeVal);
  REJECT_RETURN;
  size_t filterTypeLen;
  c->status = napi_get_value_string_utf8(env, filterTypeVal, nullptr, 0, &filterTypeLen);
  REJECT_RETURN;
  c->filterType.resize(filterTypeLen);
  c->status = napi_get_value_string_utf8(env, filterTypeVal, (char *)c->filterType.data(), filterTypeLen+1, nullptr);
  REJECT_RETURN;
  if ((0 != c->filterType.compare("audio")) && (0 != c->filterType.compare("video"))) {
    REJECT_ERROR_RETURN("Filterer expects filterType of audio or video.",
      BEAMCODER_INVALID_ARGS);
  }

  napi_value paramsArrayVal;
  uint32_t paramsArrayLen;
  c->status = napi_get_named_property(env, args[0], "inputParams", &paramsArrayVal);
  REJECT_RETURN;
  c->status = napi_is_array(env, paramsArrayVal, &isArray);
  REJECT_RETURN;
  if (!isArray) {
    REJECT_ERROR_RETURN("Filterer inputParams must be an array.",
      BEAMCODER_INVALID_ARGS);
  }
  c->status = napi_get_array_length(env, paramsArrayVal, &paramsArrayLen);
  REJECT_RETURN;

  for (uint32_t i = 0; i < paramsArrayLen; ++i) {
    napi_value inParamsVal;
    c->status = napi_get_element(env, paramsArrayVal, i, &inParamsVal);
    REJECT_RETURN;

    std::string name;
    uint32_t sampleRate;
    std::string sampleFormat;
    std::string channelLayout;
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
      name.resize(nameLen);
      c->status = napi_get_value_string_utf8(env, nameVal, (char *)name.data(), nameLen+1, nullptr);
      REJECT_RETURN;
      c->inNames.push_back(name);
    } else
      c->inNames.push_back("in");

    uint32_t arrayLen;
    if (0 == c->filterType.compare("audio")) {
      napi_value sampleRateVal;
      c->status = napi_get_named_property(env, inParamsVal, "sampleRate", &sampleRateVal);
      REJECT_RETURN;
      c->status = napi_get_value_uint32(env, sampleRateVal, &sampleRate);
      REJECT_RETURN;

      napi_value sampleFmtVal;
      c->status = napi_get_named_property(env, inParamsVal, "sampleFormat", &sampleFmtVal);
      REJECT_RETURN;
      size_t sampleFmtLen;
      c->status = napi_get_value_string_utf8(env, sampleFmtVal, nullptr, 0, &sampleFmtLen);
      REJECT_RETURN;
      pixFmt.resize(sampleFmtLen);
      c->status = napi_get_value_string_utf8(env, sampleFmtVal, (char *)sampleFormat.data(), sampleFmtLen+1, nullptr);
      REJECT_RETURN;

      napi_value channelLayoutVal;
      c->status = napi_get_named_property(env, inParamsVal, "channelLayout", &channelLayoutVal);
      REJECT_RETURN;
      size_t channelLayoutLen;
      c->status = napi_get_value_string_utf8(env, channelLayoutVal, nullptr, 0, &channelLayoutLen);
      REJECT_RETURN;
      channelLayout.resize(channelLayoutLen);
      c->status = napi_get_value_string_utf8(env, channelLayoutVal, (char *)channelLayout.data(), channelLayoutLen+1, nullptr);
      REJECT_RETURN;
    } else {
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
      c->status = napi_get_named_property(env, inParamsVal, "pixelFormat", &pixFmtVal);
      REJECT_RETURN;
      size_t pixFmtLen;
      c->status = napi_get_value_string_utf8(env, pixFmtVal, nullptr, 0, &pixFmtLen);
      REJECT_RETURN;
      pixFmt.resize(pixFmtLen);
      c->status = napi_get_value_string_utf8(env, pixFmtVal, (char *)pixFmt.data(), pixFmtLen+1, nullptr);
      REJECT_RETURN;

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
    }

    napi_value timeBaseVal;
    c->status = napi_get_named_property(env, inParamsVal, "timeBase", &timeBaseVal);
    REJECT_RETURN;
    c->status = napi_is_array(env, timeBaseVal, &isArray);
    REJECT_RETURN;
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

    char args[512];
    if (0 == c->filterType.compare("audio")) {
      snprintf(args, sizeof(args),
              "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%llx",
              timeBase.num, timeBase.den, sampleRate,
              sampleFormat.c_str(), av_get_channel_layout(channelLayout.c_str()));
    } else {
      snprintf(args, sizeof(args),
              "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
              width, height, av_get_pix_fmt(pixFmt.c_str()),
              timeBase.num, timeBase.den, pixelAspect.num, pixelAspect.den);
    }
    c->inParams.push_back(args);
  }

  if (hasOutNames) {
    napi_value namesArrayVal;
    uint32_t namesArrayLen;
    c->status = napi_get_named_property(env, args[0], "outputNames", &namesArrayVal);
    REJECT_RETURN;
    c->status = napi_is_array(env, namesArrayVal, &isArray);
    REJECT_RETURN;
    if (!isArray) {
      REJECT_ERROR_RETURN("Filterer outputNames must be an array.",
        BEAMCODER_INVALID_ARGS);
    }
    c->status = napi_get_array_length(env, namesArrayVal, &namesArrayLen);
    REJECT_RETURN;

    for (uint32_t i = 0; i < namesArrayLen; ++i) {
      napi_value outNameVal;
      c->status = napi_get_element(env, namesArrayVal, i, &outNameVal);
      REJECT_RETURN;

      std::string name;
      size_t nameLen;
      c->status = napi_get_value_string_utf8(env, outNameVal, nullptr, 0, &nameLen);
      REJECT_RETURN;
      name.resize(nameLen);
      c->status = napi_get_value_string_utf8(env, outNameVal, (char *)name.data(), nameLen+1, nullptr);
      REJECT_RETURN;
      c->outNames.push_back(name);
    }
  } else {
    c->outNames.push_back("out");
  }

  napi_value filterSpecJS;
  c->status = napi_get_named_property(env, args[0], "filterSpec", &filterSpecJS);
  REJECT_RETURN;
  size_t specLen;
  c->status = napi_get_value_string_utf8(env, filterSpecJS, nullptr, 0, &specLen);
  REJECT_RETURN;
  c->filterSpec.resize(specLen);
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
  filtContexts *srcCtxs = nullptr;
  filtContexts *sinkCtxs = nullptr;
  std::map<std::string, std::deque<AVFrame *> > srcFrames;
  std::map<std::string, std::vector<AVFrame *> > dstFrames;
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

  std::vector<std::string> sinkNames = c->sinkCtxs->getNames();
  for (auto it = sinkNames.begin(); it != sinkNames.end(); ++it) {
    std::vector<AVFrame *> frames;
    while (1) {
      AVFrame *filtFrame = av_frame_alloc();
      ret = av_buffersink_get_frame(c->sinkCtxs->getContext(*it), filtFrame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        break;
      if (ret < 0) {
        c->status = BEAMCODER_ERROR_FILTER_GET_FRAME;
        c->errorMsg = "Error while filtering.";
        break;
      }
      frames.push_back(filtFrame);
    }
    c->dstFrames.emplace(*it, frames);
  }
  c->totalTime = microTime(filterStart);
};

void filterComplete(napi_env env, napi_status asyncStatus, void* data) {
  filterCarrier* c = (filterCarrier*) data;
  napi_value result, dstFrame, nameVal, frames, frame, prop;

  if (asyncStatus != napi_ok) {
    c->status = asyncStatus;
    c->errorMsg = "Filter failed to complete.";
  }
  REJECT_STATUS;

  c->status = napi_create_array(env, &result);

  uint32_t outCount = 0;
  for (auto it = c->dstFrames.begin(); it != c->dstFrames.end(); it++) {
    c->status = napi_create_object(env, &dstFrame);
    REJECT_STATUS;

    c->status = napi_create_array(env, &frames);
    REJECT_STATUS;

    c->status = napi_create_string_utf8(env, it->first.c_str(), NAPI_AUTO_LENGTH, &nameVal);
    REJECT_STATUS;
    c->status = napi_set_named_property(env, dstFrame, "name", nameVal);
    REJECT_STATUS;

    uint32_t frameCount = 0;
    for (auto fit = it->second.begin(); fit != it->second.end(); ++fit) {
      frameData* f = new frameData;
      f->frame = *fit;

      c->status = fromAVFrame(env, f, &frame);
      REJECT_STATUS;

      c->status = napi_set_element(env, frames, frameCount++, frame);
      REJECT_STATUS;
    }
    c->status = napi_set_named_property(env, dstFrame, "frames", frames);
    REJECT_STATUS;

    c->status = napi_set_element(env, result, outCount++, dstFrame);
    REJECT_STATUS;
  }

  c->status = napi_create_int64(env, c->totalTime, &prop);
  REJECT_STATUS;
  c->status = napi_set_named_property(env, result, "total_time", prop);
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

  napi_value srcCtxsExt, sinkCtxsExt;
  c->status = napi_get_named_property(env, filtererJS, "_sourceContexts", &srcCtxsExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, srcCtxsExt, (void**)&c->srcCtxs);
  REJECT_RETURN;

  c->status = napi_get_named_property(env, filtererJS, "_sinkContexts", &sinkCtxsExt);
  REJECT_RETURN;
  c->status = napi_get_value_external(env, sinkCtxsExt, (void**)&c->sinkCtxs);
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
      name.resize(nameLen);
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
