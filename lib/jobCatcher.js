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

/* Basic idea - pattern match a job spec like /^.*\.(jpg|jpeg)$/, set up a job,
  wait for it to complete and return the result. May need a timeout.
*/

const config = require('../config.json');
const Queue = require('bull');
const redisio = require('./redisio.js');
const Boom = require('boom');

const queues = {};
Object.keys(config.jobs).forEach(k => {
  let job = config.jobs[k];
  if (job.pathPattern) {
    job.pathRE = new RegExp(job.pathPattern);
  }
  if (job.statusCode && !Array.isArray(job.statusCode)) {
    job.statusCode = [ job.statusCode ];
  }
  if (!queues[job.queue]) {
    queues[job.queue] = new Queue(job.queue, { redis: {
      port: config.redis.port, host: config.redis.host }});
  }
});
const [ preJobs, postJobs] = // eslint-disable-line no-unused-vars
  [ Object.values(config.jobs).filter(job => job.pathRE && !job.postRoute),
    Object.values(config.jobs).filter(job => job.pathRE && job.postRoute) ];

async function jobCatcher (ctx, next) {
  console.log('JOB?', ctx.path);

  for ( let x = 0 ; x < preJobs.length ; x++ ) {
    let job = preJobs[x];
    let match = job.pathRE.exec(ctx.path);
    // console.log(ctx.path, match);
    if (match) {
      let bullock = await queues[job.queue].add({
        rule: job,
        path: ctx.path,
        headers: ctx.request.headers,
        method: ctx.request.method
      });
      let result = await bullock.finished();
      // console.log(result);

      if (result.success && (0 === result.body.indexOf('beamengine:blob'))) {
        try {
          // Worker has returned a key to blob data in Redis
          result.body = await redisio.retrieveBlob(result.body);
          result.type = 'image/jpeg';
        } catch (err) {
          throw Boom.internal(`Failed to retrieve binary element ${result.body}: ${err.message}`);
        }
      }

      if (result.body) { ctx.body = result.body; }
      if (result.status) { ctx.status = result.status; }
      if (result.type) { ctx.type = result.type; }

      if (result.success) {
        return;
      }
    }
  }
  await next();

  /* postJobs.forEach(x => {
    if (x.statusCode.indexOf(ctx.status) < 0) return;
    let match = x.pathRE.exec(ctx.path);
    if (match) {
      // add post process job to bull
    }
  }); */
}

async function closeQueues() {
  await Promise.all(Object.keys(queues).map(q => queues[q].close()));
}

module.exports = {
  jobCatcher,
  closeQueues
};
