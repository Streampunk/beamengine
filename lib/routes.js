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
  if (typeof ctx.params.fmtSpec != 'string') {
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
  console.log(ctx.params);
  await next();
}

async function startRedirect (ctx, next) {
  console.log(ctx.params);
  await next();
}

async function mediaRoute (ctx, next) {
  console.log(ctx.params);
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
  dataUpdate
};
