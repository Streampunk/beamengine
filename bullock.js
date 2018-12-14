const Bull = require('bull');
const consumer = new Bull('my-first-queue');

consumer.process(async job => {
  let jd = Date.now();
  console.log('Running job', job.data, jd - job.data.start);
  await new Promise(r => setTimeout(r, 100));
  console.log('Finished job');
  return job.data;
});
