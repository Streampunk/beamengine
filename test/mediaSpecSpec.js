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

const test = require('tape');
const { parseMediaSpec, DEFAULT, FUZZY, INDEX, REALTIME, DURATION } =
  require('../lib/mediaSpec.js');

test('Timestamp range specification', t => {
  t.deepEqual(parseMediaSpec('0'),
    { start: 0, end: 0, flags: DEFAULT }, 'zero parses.');
  t.deepEqual(parseMediaSpec('0-1'),
    { start: 0, end: 1, flags: DEFAULT }, 'simple range parses.');
  t.deepEqual(parseMediaSpec('-42'),
    { start: -42, end: -42, flags: DEFAULT }, 'negative number parses.');
  t.deepEqual(parseMediaSpec('-42--39'),
    { start: -42, end: -39, flags: DEFAULT }, 'negative range parses.');
  t.deepEqual(parseMediaSpec('42f'),
    { start: 42, end: 42, flags: FUZZY }, 'fuzzy value parses.');
  t.deepEqual(parseMediaSpec('42f-53'),
    { start: 42, end: 53, flags: FUZZY }, 'fuzzy range parses.');
  t.deepEqual(parseMediaSpec('000003-9876543210'),
    { start: 3, end: 9876543210, flags: DEFAULT },
    'zero padding and 64-bit values parse.');
  t.end();
});

test('Index range specification', t => {
  t.deepEqual(parseMediaSpec('1st'),
    { start: 0, end: 0, flags: INDEX }, 'first frame index numerical.');
  t.deepEqual(parseMediaSpec('first'),
    { start: 0, end: 0, flags: INDEX }, 'first frame index lexical.');
  t.deepEqual(parseMediaSpec('2nd'),
    { start: 1, end: 1, flags: INDEX }, 'second frame index numerical.');
  t.deepEqual(parseMediaSpec('2nd-3rd'),
    { start: 1, end: 2, flags: INDEX }, 'short range numerical.');
  t.deepEqual(parseMediaSpec('last'),
    { start: -1, end: -1, flags: INDEX }, 'last frame index lexical.');
  t.deepEqual(parseMediaSpec('100th-last'),
    { start: 99, end: -1, flags: INDEX }, '100th to last OK.');
  t.deepEqual(parseMediaSpec('123456789th-9876543210st'),
    { start: 123456788, end: 9876543209, flags: INDEX }, 'big number range.');
  t.deepEqual(parseMediaSpec('9876543210st-123456789th'),
    { start: 9876543209, end: 123456788, flags: INDEX }, 'backward range OK.');
  t.end();
});

test('Time range specification', t => {
  t.deepEqual(parseMediaSpec('0s'),
    { start: 0, end: 0, flags: REALTIME }, 'zero seconds single');
  t.deepEqual(parseMediaSpec('1.0s'),
    { start: 1, end: 1, flags: REALTIME }, 'one point zero seconds single.');
  t.deepEqual(parseMediaSpec('1.5s'),
    { start: 1.5, end: 1.5, flags: REALTIME }, 'one point five seconds single.');
  t.deepEqual(parseMediaSpec('0s-1s'),
    { start: 0, end: 1, flags: REALTIME }, 'simple range.');
  t.deepEqual(parseMediaSpec('0s-1'),
    { start: 0, end: 1, flags: REALTIME }, 'simple range - no 2nd s.');
  t.deepEqual(parseMediaSpec('-42.42s'),
    { start: -42.42, end: -42.42, flags: REALTIME }, 'negative single stamp.');
  t.deepEqual(parseMediaSpec('-42.42s-24.24s'),
    { start: -42.42, end: 24.24, flags: REALTIME }, 'negative-positive range.');
  t.deepEqual(parseMediaSpec('-42.42s--24.24'),
    { start: -42.42, end: -24.24, flags: REALTIME }, 'negative-negative range.');
  t.end();
});

test('Duration specification', t => {
  t.deepEqual(parseMediaSpec('0+1d'),
    { start: 0, end: 1, flags: DURATION }, 'simplest duration jump spec.');
  t.deepEqual(parseMediaSpec('1+0d'),
    { start: 1, end: 0, flags: DURATION }, 'no duration jump spec.');
  t.deepEqual(parseMediaSpec('42-13d'),
    { start: 42, end: -13, flags: DURATION }, 'backwards looking.');
  t.deepEqual(parseMediaSpec('-42+13d'),
    { start: -42, end: 13, flags: DURATION }, 'negative range start.');
  t.deepEqual(parseMediaSpec('0f+1d'),
    { start: 0, end: 1, flags: DURATION | FUZZY }, 'fuzzy simplest duration jump spec.');
  t.deepEqual(parseMediaSpec('1f+0d'),
    { start: 1, end: 0, flags: DURATION | FUZZY }, 'fuzzy no duration jump spec.');
  t.deepEqual(parseMediaSpec('42f-13d'),
    { start: 42, end: -13, flags: DURATION | FUZZY }, 'fuzzy backwards looking.');
  t.deepEqual(parseMediaSpec('-42f+13d'),
    { start: -42, end: 13, flags: DURATION | FUZZY }, 'fuzzy negative range start.');
  t.end();
});

test('Bad specifications', t => {
  t.notOk(parseMediaSpec('wibble'), 'unexpected string fails.');
  t.notOk(parseMediaSpec('1-2f'), 'fuzzy on end is bad.');
  t.notOk(parseMediaSpec('0th'), 'zeroth index does not exist.');
  t.notOk(parseMediaSpec('last-3rd'), 'cannot start with last and go backward.');
  t.notOk(parseMediaSpec('last-last'), 'cannot range from last to last.');
  t.notOk(parseMediaSpec('42-43s'), 'cannot put time marker on second param.');
  t.notOk(parseMediaSpec(undefined), 'cannot parse undefined.');
  t.notOk(parseMediaSpec(null), 'cannot parse null.');
  t.notOk(parseMediaSpec({}), 'cannot parse an object.');
  t.end();
});
