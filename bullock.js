const Bull = require('bull');
const consumer = new Bull('my-first-queue');

consumer.process(async job => {
  let jd = Date.now();
  console.log('Running job', job.id, jd);
  let r = Math.random();
  await new Promise(re => setTimeout(re, 10000 * r));
  console.log('Finished job', await job.getState(), r * 10000);
  if (r > 0.1) {
    return job.data;
  } else {
    throw new Error('Failed!');
  }
});
