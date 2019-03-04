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
const beamcoder = require('beamcoder');
const pathToRE = require('path-to-regexp');

const beamPath = pathToRE.compile('/beams/:fmtSpec');

async function beamsRoute (ctx) {
  let offset = isNaN(ctx.request.query.offset) ? 0 : +ctx.request.query.offset;
  let limit = isNaN(ctx.request.query.limit) ? 10 : +ctx.request.query.limit;
  let beam = await redisio.listContent(offset, limit);
  ctx.body = beam;
}

async function formatRoute (ctx) {
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification was not a string value.');
  }
  try {
    let fmt = await redisio.retrieveFormat(ctx.params.fmtSpec);
    ctx.body = fmt;
  } catch (err) {
    throw Boom.notFound(`Format with name '${ctx.params.fmtSpec}' was not found: ${err.message}`);
  }
}

async function streamRoute (ctx) {
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
}

async function startRedirect (ctx) {
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
    let first = await redisio.retrieveMedia(ctx.params.fmtSpec, streamIdx,
      Number.MIN_SAFE_INTEGER, Number.MAX_SAFE_INTEGER, 0, 1, mediaSpec.DEFAULT, true);
    if (first.length !== 1) {
      throw new Error('No result returned.');
    }
    ctx.redirect(`/beams/${ctx.params.fmtSpec}/stream_${first[0].stream_index}/${first[0].type.toLowerCase()}_${first[0].pts}`);
  } catch (err) {
    throw Boom.notFound(`Could not find start frame for stream '${ctx.params.fmtSpec}:${typeof streamIdx === 'number' ? 'stream_' : ''}${streamIdx}': ${err.message}`);
  }
}

async function endRedirect (ctx) {
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
    let first = await redisio.retrieveMedia(ctx.params.fmtSpec, streamIdx,
      -1, -1, 0, 1, mediaSpec.INDEX, true);
    if (first.length !== 1) {
      throw new Error('No result returned.');
    }
    ctx.redirect(`/beams/${ctx.params.fmtSpec}/stream_${first[0].stream_index}/${first[0].type.toLowerCase()}_${first[0].pts}`);
  } catch (err) {
    throw Boom.notFound(`Could not find end frame for stream '${ctx.params.fmtSpec}:${typeof streamIdx === 'number' ? 'stream_' : ''}${streamIdx}': ${err.message}`);
  }
}

async function mediaRoute (ctx) {
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
}

async function packetRoute (ctx) {
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

async function frameRoute (ctx) {
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

async function packetDataRoute (ctx) {
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

async function frameDataRoute (ctx) {
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
}

async function createBeam (ctx) {
  if ((!ctx.request.body) || (ctx.request.type != 'application/json') ||
    (typeof ctx.request.body !== 'object')) {
    throw Boom.badRequest('Request body must be a JSON document describing a format.');
  }
  if ((ctx.request.body.type !== 'format') && (ctx.request.body.type !== 'muxer') &&
     (ctx.request.body.type !== 'demuxer')) {
    throw Boom.badRequest('Request body must be of format type.');
  }
  let fmt = beamcoder.format(ctx.request.body);
  try {
    let storeResult = await redisio.storeFormat(fmt, false);
    if (!storeResult.every(x => x && x.startsWith('OK'))) {
      throw Boom.badGateway('Failure to store format into Redis.');
    }
  } catch (err) {
    if (err.message.indexOf('exists') >= 0)
      throw Boom.conflict(err.message);
    throw Boom.badImplementation(err.message);
  }
  ctx.set('Location', beamPath({ fmtSpec: fmt.url }));
  ctx.status = 201;
  ctx.body = fmt;
}

// TODO
async function formatUpdate (ctx) {
  console.log(ctx.params);
  throw Boom.notImplemented('Updating formats is not yet implemented.');
}

// TODO
async function createRelated (ctx) {
  console.log(ctx.params);
  throw Boom.notImplemented('Creating related foramts is not yet implemented.');
}

const extractPTS = /(\d+)$/;

async function mediaUpdate (ctx) {
  if (typeof ctx.params.fmtSpec !== 'string') {
    throw Boom.badRequest('Beam format specification - must be a string value.');
  }
  let streamIdx = isNaN(+ctx.params.streamSpec) ?
    ctx.params.streamSpec : +ctx.params.streamSpec;
  let streamIdxType = typeof streamIdx;
  if ((streamIdxType !== 'number') && (streamIdxType !== 'string')) {
    throw Boom.badRequest('Beam stream specification - must be an index or an alias.');
  }
  if (streamIdxType === 'string' && streamIdx.startsWith('stream_')) {
    streamIdx = +streamIdx.slice(7);
    [streamIdx, streamIdxType] = isNaN(streamIdx) ?
      [ctx.params.streamSpec, 'string'] : [streamIdx, 'number'];
  }

  let isPacket = ctx.params.mediaSpec.startsWith('packet_');
  let isFrame = ctx.params.mediaSpec.startsWith('frame_');
  if (!ctx.request.body || (ctx.request.type !== 'application/json') ||
    (typeof ctx.request.body !== 'object') || (!ctx.request.body.type)) {
    throw Boom.badRequest('Media element must be described with JSON.');
  }

  if (isPacket && ctx.request.body.type !== 'Packet') {
    throw Boom.badRequest(`Media element type '${ctx.request.body.type}' does not match URL parameter '${ctx.params.mediaSpec}'.`);
  }
  if (isFrame && ctx.request.body.type !== 'Frame') {
    throw Boom.badRequest(`Media element type '${ctx.request.body.type}' does not match URL parameter '${ctx.params.mediaSpec}'.`);
  }

  if (!isPacket && !isFrame) {
    isPacket = ctx.request.body.type === 'Packet';
    isFrame = ctx.request.body.type === 'Frame';
  }

  if (!isPacket && !isFrame) {
    throw Boom.badRequst('Media element is not a packet or a frame.');
  }

  if (isPacket && streamIdxType === 'number' &&
    ctx.request.body.stream_index !== streamIdx) {
    throw Boom.badRequest(`Packet stream index '${ctx.request.body.stream_index}' does not match URL stream index '${streamIdx}'.`);
  }

  let ptsMatch = extractPTS.exec(ctx.params.mediaSpec);
  console.log(ctx.request.body.pts, +ptsMatch[1]);
  if (!ptsMatch) {
    throw Boom.badRequest('Cannot extact PTS from URL.');
  }
  if (ctx.request.body.pts !== +ptsMatch[1]) {
    throw Boom.badRequest(`URL PTS of '${ptsMatch[1]}' does not match media element PTS of '${ctx.request.body.pts}'.`);
  }

  try {
    if (isPacket) {
      ctx.request.body.buf_size = ctx.request.body.size;
      delete ctx.request.body.size;
      let pkt = beamcoder.packet(ctx.request.body);
      let result = await redisio.storePacket(ctx.params.fmtSpec, pkt);
      if (!result.every(x => x && x.startsWith('OK'))) {
        throw new Error(`Failed to store packet. Result is [${result}].`);
      }
      ctx.body = pkt;
      ctx.status = result[0].endsWith('ovw') ? 200 : 201;
    }
    if (isFrame) {
      let frm = beamcoder.frame(ctx.request.body);
      let result = await redisio.storeFrame(ctx.params.fmtSpec, frm, streamIdx);
      if (!result.every(x => x && x.startsWith('OK'))) {
        throw new Error(`Failed to store frame. Result is [${result}].`);
      }
      ctx.body = frm;
      ctx.status = result[0].endsWith('ovw') ? 200 : 201;
    }
  } catch (err) {
    throw Boom.internal(`Failed to store media element: ${err.message}`);
  }
}

async function dataUpdate (ctx) {
  console.log(ctx.params, ctx.request);
  let reader = new Promise((resolve, reject) => {
    let data = Buffer.alloc(ctx.request.length);
    let pos = 0;
    ctx.req.on('data', d => {
      pos += d.copy(data, pos);
    });
    ctx.req.on('error', reject);
    ctx.req.on('end', () => {
      resolve(data);
    });
  });
  let data = await reader;
  console.log(data.length);
  ctx.body = null;
}

module.exports = {
  beamsRoute,
  formatRoute,
  streamRoute,
  startRedirect,
  endRedirect,
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
