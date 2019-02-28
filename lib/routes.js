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

const Boom = require('boom');
const redisio = require('./redisio.js');
const mediaSpec = require('./mediaSpec.js');

async function beamsRoute (ctx, next) {
  let offset = isNaN(ctx.request.query.offset) ? 0 : +ctx.request.query.offset;
  let limit = isNaN(ctx.request.query.limit) ? 10 : +ctx.request.query.limit;
  let beam = await redisio.listContent(offset, limit);
  ctx.body = beam;
  await next();
}

async function formatRoute (ctx, next) {
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification was not a string value.');
  }
  try {
    let fmt = await redisio.retrieveFormat(ctx.params.fmtSpec);
    ctx.body = fmt;
  } catch (err) {
    throw Boom.notFound(`Format with name '${ctx.params.fmtSpec}' was not found: ${err.message}`);
  }
  await next();
}

async function streamRoute (ctx, next) {
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification - must be a string value.');
  }
  let streamIdx = isNaN(+ctx.params.streamSpec) ?
    ctx.params.streamSpec : +ctx.params.streamSpec;
  let indexType = typeof streamIdx;
  if ((indexType !== 'number') && (indexType !== 'string')) {
    throw Boom.badRequest('Beam stream specification - must be an index or an alias.');
  }
  try {
    let str = await redisio.retrieveStream(ctx.params.fmtSpec, streamIdx);
    ctx.body = str;
  } catch (err) {
    throw Boom.notFound(`Stream with name '${ctx.params.fmtSpec}:${indexType === 'number' ? 'stream_' : ''}${streamIdx}' was not found: ${err.message}`);
  }
  await next();
}

async function startRedirect (ctx, next) {
  console.log(ctx.params);
  await next();
}

async function mediaRoute (ctx, next) {
  console.log(ctx.params);
  let offset = isNaN(ctx.request.query.offset) ? 0 : +ctx.request.query.offset;
  let limit = isNaN(ctx.request.query.limit) ? 10 : +ctx.request.query.limit;
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification - must be a string value.');
  }
  let streamIdx = isNaN(+ctx.params.streamSpec) ?
    ctx.params.streamSpec : +ctx.params.streamSpec;
  let streamIdxType = typeof streamIdx;
  if ((streamIdxType !== 'number') && (streamIdxType !== 'string')) {
    throw Boom.badRequest('Beam stream specification - must be an index or an alias.');
  }
  let mediaParams = mediaSpec.parseMediaSpec(ctx.params.mediaSpec);
  if (!mediaParams) {
    throw Boom.badRequest('Beam media specification could not be matched.');
  }
  try {
    let media = await redisio.retrieveMedia(ctx.params.fmtSpec, streamIdx,
      mediaParams.start, mediaParams.end, offset, limit, mediaParams.flags, true);
    ctx.body = media;
  } catch (err) {
    throw Boom.notFound(`Media with name '${ctx.params.fmtSpec}:${streamIdxType === 'number' ? 'stream_' : ''}${streamIdx}:${ctx.params.mediaSpec}' was not found: ${err.message}`);
  }
  await next();
}

async function packetRoute (ctx, next) {
  console.log('!!! Packet metadata route', ctx.params);
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification - must be a string value.');
  }
  let streamIdx = isNaN(+ctx.params.streamSpec) ?
    ctx.params.streamSpec : +ctx.params.streamSpec;
  let streamIdxType = typeof streamIdx;
  if ((streamIdxType !== 'number') && (streamIdxType !== 'string')) {
    throw Boom.badRequest('Beam stream specification - must be an index or an alias.');
  }
  try {
    let media = await redisio.retrievePacketMetadata(ctx.params.fmtSpec,
      ctx.params.streamSpec, ctx.params.pts);
    ctx.body = media;
  } catch (err) {
    throw Boom.notFound(`Packet with name '${ctx.params.fmtSpec}:${streamIdxType === 'number' ? 'stream_' : ''}${streamIdx}:packet_${ctx.params.pts}' was not found: ${err.message}`);
  }
  // await next();
}

async function frameRoute (ctx, next) {
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification - must be a string value.');
  }
  let streamIdx = isNaN(+ctx.params.streamSpec) ?
    ctx.params.streamSpec : +ctx.params.streamSpec;
  let streamIdxType = typeof streamIdx;
  if ((streamIdxType !== 'number') && (streamIdxType !== 'string')) {
    throw Boom.badRequest('Beam stream specification - must be an index or an alias.');
  }
  try {
    let media = await redisio.retrieveFrameMetadata(ctx.params.fmtSpec,
      ctx.params.streamSpec, ctx.params.pts);
    ctx.body = media;
  } catch (err) {
    throw Boom.notFound(`Frame with name '${ctx.params.fmtSpec}:${streamIdxType === 'number' ? 'stream_' : ''}${streamIdx}:frame_${ctx.params.pts}' was not found: ${err.message}`);
  }
  // await next();
}

async function packetDataRoute (ctx, next) {
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification - must be a string value.');
  }
  let streamIdx = isNaN(+ctx.params.streamSpec) ?
    ctx.params.streamSpec : +ctx.params.streamSpec;
  let streamIdxType = typeof streamIdx;
  if ((streamIdxType !== 'number') && (streamIdxType !== 'string')) {
    throw Boom.badRequest('Beam stream specification - must be an index or an alias.');
  }
  // Pull out data index for frames
  try {
    let media = await redisio.retrievePacketData(ctx.params.fmtSpec,
      ctx.params.streamSpec, ctx.params.pts);
    ctx.set('Beam-PTS', ctx.params.pts);
    ctx.body = media;
  } catch (err) {
    throw Boom.notFound(`Packet with name '${ctx.params.fmtSpec}:${streamIdxType === 'number' ? 'stream_' : ''}${streamIdx}:packet_${ctx.params.pts}' was not found: ${err.message}`);
  }
  // await next();
}

async function frameDataRoute (ctx, next) {
  console.log(ctx.params);
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification - must be a string value.');
  }
  let streamIdx = isNaN(+ctx.params.streamSpec) ?
    ctx.params.streamSpec : +ctx.params.streamSpec;
  let streamIdxType = typeof streamIdx;
  if ((streamIdxType !== 'number') && (streamIdxType !== 'string')) {
    throw Boom.badRequest('Beam stream specification - must be an index or an alias.');
  }
  // Pull out data index for frames
  try {
    let media = await redisio.retrieveFrameData(ctx.params.fmtSpec,
      ctx.params.streamSpec, ctx.params.pts, ctx.params.idx);
    ctx.set('Beam-PTS', ctx.params.pts);
    ctx.set('Beam-Data-Index', ctx.params.idx);
    ctx.set('Beam-Buf-Sizes', JSON.stringify(media.bufSizes));
    ctx.body = media.data;
  } catch (err) {
    throw Boom.notFound(`Frame data with name '${ctx.params.fmtSpec}:${streamIdxType === 'number' ? 'stream_' : ''}${streamIdx}:frame_${ctx.params.pts}' was not found: ${err.message}`);
  }
  // await next();

  // await next();
}

async function createBeam (ctx, next) {
  console.log(ctx.params);
  await next();
}
async function formatUpdate (ctx, next) {
  console.log(ctx.params);
  await next();
}

async function createRelated (ctx, next) {
  console.log(ctx.params);
  await next();
}

async function mediaUpdate (ctx, next) {
  console.log(ctx.params);
  await next();
}

async function dataUpdate (ctx, next) {
  console.log(ctx.params);
  await next();
}

module.exports = {
  beamsRoute,
  formatRoute,
  streamRoute,
  startRedirect,
  mediaRoute,
  packetRoute,
  frameRoute,
  packetDataRoute,
  frameDataRoute,
  createBeam,
  formatUpdate,
  createRelated,
  mediaUpdate,
  dataUpdate
};
