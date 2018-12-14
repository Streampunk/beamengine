const
  Queue = require('bull'),
  cluster = require('cluster'),
  beamcoder = require('bindings')('beamcoder');

var numWorkers = 8;
var queue = new Queue('my-first-queue');

if(cluster.isMaster){
  for (var i = 0; i < numWorkers; i++) {
    cluster.fork();
  }

  cluster.on('exit', (worker, code, signal) => {
    console.log('worker ' + worker.process.pid + ' died');
  });
}else{
  queue.process(async (job) => {
    console.log('Running job', job.id, cluster.worker.id);
    let r = Math.random();
    await new Promise(re => setTimeout(re, 10000 * r));
    job.data.version = beamcoder.version;
    console.log('Finished job', cluster.worker.id, await job.getState(), r * 10000);
    return job.data;
  });
}
