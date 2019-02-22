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

/*
  mediaSpec is:

  - <pts> - Single PTS value in stream timebase units - exact match
  - <pts>f - Fuzzy match of pts, from half duration before to half duration after
  - <start>-<end> - Range of start pts to end pts, inclusive
  - <pts>(st|nd|rd|th)|first|last - pts is count rather than timebase units
  - <start>(st|nd|rd|th)|first|last-<end>(.*) - count-based range, inclusive
  - <pts>s - fuzzy match based on real time in seconds - fuzz is half duration
  - <start>s-<end>(s)? - fuzzy time match for a range.
  - <pts>(+|-)<dur> - find the frame <dur> before or after <pts>
*/

const tsRange = /(-?\d+)(f?)(-(-?\d+)(f?))?/;
const idxRange = /(\d+|fir|la)(st|nd|rd|th)(-(\d+|last)\w+)?/;
const secRange = /(-?\d+\.?\d*)s(-(-?\d+\.?\d*)s?)?/; // TODO add hour/min
const durRange = /(-?\d+)(f?)((\+|-)(\d+)d)/;

const [ DEFAULT, FUZZY, INDEX, REALTIME, DURATION ] = [ 0, 1, 2, 4, 8 ];

const parseMediaSpec = ms => {
  let match = ms.match(tsRange);
  if (match) {
    if (!match[2] && match[5]) return null; // All or nothing for fuzziness
    return {
      start: +match[1],
      end: match[4] ? +match[4] : +match[1],
      flags: match[2] === 'f' ? FUZZY : DEFAULT };
  }
  match = ms.match(idxRange);
  if (match) {
    let first = match[1] === 'fi' ? 0 : (match[1] == 'la' ? -1 : +match[1]);
    return {
      start: first,
      end: match[4] ? (match[4] === 'last' ? -1 : +match[4]) : first,
      flags: INDEX };
  }
  match = ms.match(secRange);
  if (match) {
    return {
      start: +match[1],
      end: match[3] ? +match[3] : +match[1],
      flags: REALTIME
    };
  }
  match = ms.match(durRange);
  if (match) {
    let first = +match[1];
    return {
      start: first,
      end: match[4] == '+' ? +match[5] : -(+match[5]),
      flags: DURATION | (match[2] === 'f' ? FUZZY : DEFAULT)
    };
  }
  return null;
};

async function mediaRoute (ctx, next) {
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
  let mediaIdx = isNaN(+ctx.params.mediaSpec) ?
    ctx.params.mediaSpec : +ctx.params.mediaSpec;
  if ((streamIdxType !== 'number') && (streamIdxType !== 'string')) {
    throw Boom.badRequest('Beam media specification - must be a number or a string.');
  }
  try {
    let media = await redisio.retrieveMediaMetadata(ctx.params.fmtSpec, streamIdx, mediaIdx);
    ctx.body = media;
  } catch (err) {
    throw Boom.notFound(`Media with name '${ctx.params.fmtSpec}:${streamIdxType === 'number' ? 'stream_' : ''}${streamIdx}:${mediaIdx}' was not found: ${err.message}`);
  }
  await next();
}

async function dataRoute (ctx, next) {
  console.log(ctx.params);
  await next();
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
  dataRoute,
  createBeam,
  formatUpdate,
  createRelated,
  mediaUpdate,
  dataUpdate,
  parseMediaSpec
};
