const NodeResque = require('node-resque');

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

async function boot() {
  const jobs = {
    'add': {
      plugins: ['JobLock'],
      pluginOptions: {
        JobLock: {}
      },
      perform: async (a, b) => {
        console.log(`Doing something ${Date.now()}`);
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

  const worker = new NodeResque.Worker({
    connection: connectionDetails,
    queues: ['math'],
    timeout: 5000
  }, jobs);
  await worker.connect();
  worker.start();

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
}

boot();
