/*
  Aerostat Beam Engine - Redis-backed highly-scale-able and cloud-fit media beam engine.
  Copyright (C) 2019 Streampunk Media Ltd.

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

const test = require('tape');
const config = require('../config.js');

test('Check that this is a test', t => {
  t.equal(process.env.NODE_ENV, 'test', 'this is a test.');
  t.end();
});

test('Default parameters as expected', t => {
  t.equal(config.redis.host, '127.0.0.1', 'redis host as expected.');
  t.equal(config.redis.port, 6379, 'redis port as expectd.');
  t.equal(config.redis.db, 15, 'redis test database selected.');
  t.equal(config.app.port, 13131, 'application test port as expected.');
  t.end();
});

test('Override some parameters with env variables', t => {
  delete require.cache[require.resolve('../config.js')];
  process.env.NODE_ENV = 'development';
  process.env.BEAM_DEV_APP_PORT = '54321';
  process.env.BEAM_DEV_REDIS_PASSWORD = 'flunky';
  let reloadConfig = require('../config.js');
  t.equal(reloadConfig.app.port, 54321, 'port number overridden.');
  t.equal(reloadConfig.redis.password, process.env.BEAM_DEV_REDIS_PASSWORD,
    'password overridden expected.');
  t.end();
});

test('Loading external config', t => {
  process.argv[2] = __dirname + '/test_config.json';
  let updatedConfig = config.load();
  t.equal(updatedConfig, config, 'updated config changes original.');
  t.equal(config.redis.port, 9876, 'redis port overridden by change.');
  t.equal(config.redis.host, '192.168.1.100', 'redis host overriden by change.');
  t.equal(config.redis.password, 'auth', 'redis password has been set.');
  t.end();
});

test('Merging rules', t => {
  t.deepEqual(config.rules.mp4.method, 'GET', 'mp4 method pre-check OK.');
  t.ok(config.rules.jpeg, 'pre-test that JPEG rules exists.');
  process.argv[2] = __dirname + '/test_rules.json';
  delete require.cache[require.resolve('../config.js')];
  let reloadConfig = require('../config.js').load();
  t.deepEqual(reloadConfig.rules.mp4.method, [ 'GET', 'POST' ], 'mp4 method pre-check OK.');
  t.notOk(reloadConfig.rules.jpeg, 'post-test that JPEG rule is deleted.');
  t.end();
});
