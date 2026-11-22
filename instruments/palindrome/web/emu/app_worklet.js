// SPDX-License-Identifier: MPL-2.0
// PALINDROME's own part of the emulator's worklet: what the engine says of its rings (dsp/palindrome.c
// pm_web_state), sent to the page when it changes, for the guide (scripts/tutorial/tutorial.js) to follow.
globalThis.fm1App = {
  process(fm1) {
    if (!fm1.ex || !fm1.ex.pm_web_state) return;
    this.n = (this.n || 0) + 1;
    if (this.n % 32) return;                         // about ten times a second
    const v = fm1.ex.pm_web_state() >>> 0;
    if (v !== this.last) {
      this.last = v;
      fm1.port.postMessage({ type: "app", pal: v });
    }
  },
};
