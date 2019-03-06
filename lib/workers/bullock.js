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

const Queue = require('bull');
const consumer = new Queue('media_workers');
const beamcoder = require('beamcoder');

consumer.process(async job => {
  let jd = Date.now();
  console.log('Running job', job.id, jd);
  let r = Math.random();
  await new Promise(re => setTimeout(re, 10000 * r));
  console.log('Finished job', await job.getState(), r * 10000);
  if (r > 0.1) {
    job.data.success = true;
    job.data.body = Object.keys(beamcoder.demuxers());
    return job.data;
  } else {
    throw new Error('Failed!');
  }
});
