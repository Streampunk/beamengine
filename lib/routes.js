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

async function contentRoute (ctx, next) {
  let offset = isNaN(ctx.request.query.offset) ? 0 : +ctx.request.query.offset;
  let limit = isNaN(ctx.request.query.limit) ? 10 : +ctx.request.query.limit;
  let content = await redisio.listContent(offset, limit);
  ctx.body = content;
  await next();
}

async function formatRoute (ctx, next) {

}

async function streamRoute (ctx, next) { }

async function startRedirect (ctx, next) { }

async function mediaRoute (ctx, next) { }

async function dataRoute (ctx, next) { }

async function createContent (ctx, next) { }

async function formatUpdate (ctx, next) { }

async function createRelated (ctx, next) { }

async function mediaUpdate (ctx, next) { }

async function dataUpdate (ctx, next) { }

module.exports = {
  contentRoute,
  formatRoute,
  streamRoute,
  startRedirect,
  mediaRoute,
  dataRoute,
  createContent,
  formatUpdate,
  createRelated,
  mediaUpdate,
  dataUpdate
};
