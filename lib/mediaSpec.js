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

/*
  mediaSpec is:

  - <pts> - Single PTS value in stream timebase units - exact match
  - <pts>f - Fuzzy match of pts, from half duration before to half duration after
  - <start>-<end> - Range of start pts to end pts, inclusive
  - <pts>(st|nd|rd|th)|first|last - pts is count rather than timebase units
  - <start>(st|nd|rd|th)|first|last-<end>(.*) - count-based range, inclusive
  - <pts>s - fuzzy match based on real time in seconds - fuzz is half duration
  - <start>s-<end>(s)? - fuzzy time match for a range.
  - <pts>(+|-)<dur> - find the frame <dur> before or after <pts>
*/

const tsRange = /^(-?\d+)(f?)(-(-?\d+)(f?))?$/;
const idxRange = /^(\d+|fir|la)(st|nd|rd|th)(-(\d+|last)\w*)?$/;
const secRange = /^(-?\d+\.?\d*)s(-(-?\d+\.?\d*)s?)?$/; // TODO add hour/min
const durRange = /^(-?\d+)(f?)((\+|-)(\d+)d)$/;

const [ DEFAULT, FUZZY, INDEX, REALTIME, DURATION ] = [ 0, 1, 2, 4, 8 ];

const parseMediaSpec = ms => {
  if (typeof ms !== 'string') return null;
  let match = ms.match(tsRange);
  if (match) {
    if (!match[2] && match[5]) return null; // All or nothing for fuzziness
    return {
      start: +match[1],
      end: match[4] ? +match[4] : +match[1],
      flags: match[2] === 'f' ? FUZZY : DEFAULT };
  }
  match = ms.match(idxRange);
  if (match) {
    let first = match[1] === 'fir' ? 0 : (match[1] == 'la' ? -1 : +match[1] - 1);
    if (match[1].startsWith('0')) return null;
    if (match[4] && match[1] === 'la') return null;
    return {
      start: first,
      end: match[4] ? (match[4] === 'last' ? -1 : +match[4] - 1) : first,
      flags: INDEX };
  }
  match = ms.match(secRange);
  if (match) {
    return {
      start: +match[1],
      end: match[3] ? +match[3] : +match[1],
      flags: REALTIME
    };
  }
  match = ms.match(durRange);
  if (match) {
    let first = +match[1];
    return {
      start: first,
      end: match[4] == '+' ? +match[5] : -(+match[5]),
      flags: DURATION | (match[2] === 'f' ? FUZZY : DEFAULT)
    };
  }
  return null;
};

module.exports = { parseMediaSpec, DEFAULT, FUZZY, INDEX, REALTIME, DURATION };
