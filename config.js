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

const fs = require('fs');

const env = process.env.NODE_ENV || 'development';
let base = require(`./config/${env}.js`);

let loaded = false;

function loadOverride() {
  if (loaded) return base;
  if (process.argv.length > 2) {
    try {
      let argConfig = fs.readFileSync(process.argv[2]);
      let override = JSON.parse(argConfig);
      Object.keys(base).forEach(x => { // merge at second level
        Object.assign(base[x], override[x]);
      });
      Object.keys(base.rules ? base.rules : {})
        .filter(x => base.rules[x] === null)
        .forEach(x => { delete base.rules[x]; });
    } catch (err) {
      console.error(`Failed to open file ${process.argv[2]}: ${err.message}`);
    }
  }
  loaded = true;
  return base;
}

base.load = loadOverride;
Object.defineProperty(base, 'load', { enumerable: false });

module.exports = base;
