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
const { parseMediaSpec } = require('../lib/routes.js');

const [ DEFAULT, FUZZY, INDEX, REALTIME, DURATION ] = [ 0, 1, 2, 4, 8 ];

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
  t.end();
});

test('Time range specification', t => {
  t.end();
});

test('Duration specification', t => {
  t.end();
});

test('Bad specifications', t => {
  t.notOk(parseMediaSpec('wibble'), 'unexpected string fails.');
  t.notOk(parseMediaSpec('1-2f'), 'fuzzy on end is bad.');
  t.end();
});
