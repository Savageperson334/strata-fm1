// SPDX-License-Identifier: MPL-2.0
// PALINDROME's own part of the emulator's page: the engine's state (app_worklet.js) kept where the guide reads it.
export function message(m) {
  if (m && m.pal !== undefined) {
    window.palState = {
      rings: m.pal & 15, ring: (m.pal >> 4) & 15, capturing: !!((m.pal >> 8) & 1), progress: (m.pal >> 9) & 127,
      grains: (m.pal >> 16) & 31,
    };
  }
}
