/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2018  Streampunk Media Ltd.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

  https://www.streampunk.media/ mailto:furnace@streampunk.media
  14 Ormiscaig, Aultbea, Achnasheen, IV22 2JJ  U.K.
*/

const Koa = require('koa');
const Router = require('koa-router');
const Redis = require('ioredis');
const NodeResque = require('node-resque');
const render = require('koa-ejs');
const path = require('path');

const app = new Koa();
const router = new Router();
const redis = new Redis();

render(app, {
  root: path.join(__dirname, 'views'),
  layout: 'template',
  viewExt: 'html',
  cache: false,
  debug: false
});

app.use(async (ctx, next) => {
  await redis.set('date', new Date());
  await redis.set('ip', ctx.ip);
  return next();
});

/* app.use(async (ctx) => {
  const users = [{ name: 'Dead Horse' }, { name: 'Jack' }, { name: 'Tom' }];
  await ctx.render('content', {
    users
  });
}); */

const timer = t => new Promise(f => {
  setTimeout(f, t);
});

router.get('/fred', async ctx => {
  await redis.set('bang', 'bomb');
  await timer(500);
  ctx.body = 'Hello Burp ' + await redis.get('bang');
});

router.get('/ginger.html', async (ctx) => {
  const users = [{ name: 'Dead Rabbit' }, { name: 'Jack' }, { name: 'Tom' }];
  await ctx.render('content', {
    users,
    ip: await redis.get('ip'),
    date: await redis.get('date')
  });
});

app
  .use(router.routes())
  .use(router.allowedMethods());

app.listen(3000);

app.on('error', (err) => {
  console.log(err.stack);
});

async function boot () {
  // ////////////////////////
  // SET UP THE CONNECTION //
  // ////////////////////////

  const connectionDetails = {
    pkg: 'ioredis',
    host: 'localhost',
    password: null,
    port: 6379,
    database: 0
    // namespace: 'resque',
    // looping: true,
    // options: {password: 'abc'},
  };

  // ///////////////////////////
  // DEFINE YOUR WORKER TASKS //
  // ///////////////////////////

  let jobsToComplete = 0;

  const jobs = {
    'add': {
      plugins: ['JobLock'],
      pluginOptions: {
        JobLock: {}
      },
      perform: async (a, b) => {
        console.log(typeof a, typeof b);
        let answer = a + b;
        await new Promise((resolve) => { setTimeout(resolve, 1000); });
        return answer;
      }
    },
    'subtract': {
      perform: (a, b) => {
        let answer = a - b;
        return answer;
      }
    }
  };

  // /////////////////
  // START A WORKER //
  // /////////////////

  const worker = new NodeResque.Worker({connection: connectionDetails, queues: ['math', 'otherQueue']}, jobs);
  await worker.connect();
  // worker.start();

  // ////////////////////
  // START A SCHEDULER //
  // ////////////////////

  const scheduler = new NodeResque.Scheduler({connection: connectionDetails});
  await scheduler.connect();
  scheduler.start();

  // //////////////////////
  // REGISTER FOR EVENTS //
  // //////////////////////

  worker.on('start', () => { console.log('worker started'); });
  worker.on('end', () => { console.log('worker ended'); });
  worker.on('cleaning_worker', (worker, pid) => { console.log(`cleaning old worker ${worker} ${pid}`); });
  worker.on('poll', (queue) => { console.log(`worker polling ${queue}`); });
  worker.on('ping', (time) => { console.log(`worker check in @ ${time}`); });
  worker.on('job', (queue, job) => { console.log(`working job ${queue} ${JSON.stringify(job)}`); });
  worker.on('reEnqueue', (queue, job, plugin) => { console.log(`reEnqueue job (${plugin}) ${queue} ${JSON.stringify(job)}`); });
  worker.on('success', (queue, job, result) => { console.log(`job success ${queue} ${JSON.stringify(job)} >> ${result}`); });
  worker.on('failure', (queue, job, failure) => { console.log(`job failure ${queue} ${JSON.stringify(job)} >> ${failure}`); });
  worker.on('error', (error, queue, job) => { console.log(`error ${queue} ${JSON.stringify(job)}  >> ${error}`); });
  worker.on('pause', () => { console.log('worker paused'); });

  scheduler.on('start', () => { console.log('scheduler started'); });
  scheduler.on('end', () => { console.log('scheduler ended'); });
  scheduler.on('poll', () => { console.log('scheduler polling'); });
  scheduler.on('master', (state) => { console.log(`scheduler became master ${state}`); });
  scheduler.on('cleanStuckWorker', (workerName, errorPayload, delta) => { console.log(`failing ${workerName} (stuck for ${delta}s) and failing job ${errorPayload}`); });
  scheduler.on('error', (error) => { console.log(`scheduler error >> ${error}`); });
  scheduler.on('workingTimestamp', (timestamp) => { console.log(`scheduler working timestamp ${timestamp}`); });
  scheduler.on('transferredJob', (timestamp, job) => { console.log(`scheduler enquing job ${timestamp} >> ${JSON.stringify(job)}`); });

  // //////////////////////
  // CONNECT TO A QUEUE //
  // //////////////////////

  const queue = new NodeResque.Queue({connection: connectionDetails}, jobs);
  queue.on('error', (error) => { console.log(error); });
  await queue.connect();
  console.log(`Sending something ${Date.now()}`);
  await queue.enqueue('math', 'add', [1, 2]);
  await queue.enqueue('math', 'add', [1, 2]);
  await queue.enqueue('math', 'add', [2, 3]);
  await queue.enqueueIn(3000, 'math', 'subtract', [2, 1]);
}

boot();
