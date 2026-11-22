// SPDX-License-Identifier: MPL-2.0
// The emulator's Play / Tutorial switch (scripts/site_nav.py puts this, tutorial.css and one firmware's
// tutorial JSON into the emulator's page).
//
//   Play      the panel as it is: buttons, knobs and keys play the instrument.
//   Guide     (a firmware whose JSON has "guide") the panel still plays; a card says what to do next and the control
//             to use is ringed. A step is done when the instrument itself says so: its lights (the page's frames:
//             window.emuLast) and, for PALINDROME, its rings (window.palState, from its web/emu/app.js). Back, Next
//             and Exit are always there: nothing depends on a step being noticed.
//   Tutorial  ("Reference" where the JSON says so) a blueprint drawn over the panel: every control outlined, a line
//             from each one to a box that says what it does in this firmware. Tapping a control here does not play
//             it: it shows what it does in the card over the panel. A button that changes what the knobs or keys do
//             (a page; in STRATA a layer held: one tap for held, another to let go) changes the boxes too.
//
// The drawing takes its geometry from the panel fm1-base drew (svg#panel, viewBox 0 0 1000 640), so the two agree.
(function () {
  "use strict";
  var T = __TUTORIAL__;
  var NS = "http://www.w3.org/2000/svg";
  var device = document.getElementById("device"), panel = document.getElementById("panel");
  var pill = document.getElementById("fm1mode"), card = document.getElementById("fm1tut-card");
  if (!device || !panel || !pill || !card) return;
  var svg = null, tries = 0, base = T.start, hold = null, picked = null;
  var knobs = {}, buttons = {}, parts = {};          // by name: geometry and the overlay's elements
  var KNOB_ORDER = ["MASTER", "SELECT", "PRESETS", "ALGORITHM", "KNOB1", "KNOB2", "KNOB3", "KNOB4"];

  function el(tag, attrs, parent, text) {
    var e = document.createElementNS(NS, tag);
    for (var k in attrs) e.setAttribute(k, attrs[k]);
    if (text != null) e.textContent = text;
    (parent || svg).appendChild(e);
    return e;
  }
  function label(parent, x, y, s, size, cls) {
    return el("text", { x: x, y: y, "text-anchor": "middle", "font-size": size, class: cls || "" }, parent, s);
  }
  function fit(t, max) {                             // a line wider than its box: squeezed to it
    try {
      t.removeAttribute("textLength");
      if (t.getComputedTextLength() > max) { t.setAttribute("textLength", max); t.setAttribute("lengthAdjust", "spacingAndGlyphs"); }
    } catch (e) { /* (not drawn yet) */ }
  }
  function page() { return T.pages[hold || base]; }
  function keysNow() { return page().keys || T.pages[base].keys || T.keys; }
  function knobNow(name) { return name === "MASTER" ? T.master : name === "SELECT" && T.select ? T.select : page().knobs[name]; }

  // --- the panel's geometry -----------------------------------------------------------------------------------
  function measure() {
    var faces = panel.querySelectorAll("rect.btnface");
    if (!faces.length) return false;
    faces.forEach(function (f) {
      var t = f.parentNode.querySelector("text");
      if (!t) return;
      buttons[t.textContent] = { x: +f.getAttribute("x"), y: +f.getAttribute("y"), w: +f.getAttribute("width"), h: +f.getAttribute("height") };
    });
    panel.querySelectorAll("g.knob").forEach(function (g) {
      var c = g.querySelector("circle.knobface"), t = g.querySelector("text");
      if (c && t) knobs[t.textContent] = { x: +c.getAttribute("cx"), y: +c.getAttribute("cy"), r: +c.getAttribute("r") };
    });
    return true;
  }

  // --- the blueprint ------------------------------------------------------------------------------------------
  function hit(shape, kind, name, say) {             // a control, or its box: tap it (or Enter) to read about it
    shape.setAttribute("class", "hit");
    shape.setAttribute("tabindex", "0");
    shape.setAttribute("role", "button");
    shape.setAttribute("aria-label", say);
    shape.addEventListener("click", function () { choose(kind, name); });
    shape.addEventListener("keydown", function (e) {
      if (e.key === "Enter" || e.key === " ") { e.preventDefault(); choose(kind, name); }
    });
  }
  function build() {
    if (!measure()) { if (++tries < 200) setTimeout(build, 50); return; }
    svg = document.createElementNS(NS, "svg");
    svg.setAttribute("id", "fm1tut");
    svg.setAttribute("viewBox", "0 0 1000 640");
    svg.setAttribute("hidden", "");
    device.appendChild(svg);
    var defs = el("defs", {});
    var grid = el("pattern", { id: "fm1tut-grid", width: 20, height: 20, patternUnits: "userSpaceOnUse" }, defs);
    el("path", { d: "M20 0H0V20", class: "grid" }, grid);
    var mask = el("mask", { id: "fm1tut-mask" }, defs);          // (the screen stays the live one: a hole for it)
    el("rect", { x: 4, y: 4, width: 992, height: 632, rx: 44, fill: "#fff" }, mask);
    el("rect", { x: 316, y: 60, width: 204, height: 204, rx: 4, fill: "#000" }, mask);
    el("rect", { x: 0, y: 0, width: 1000, height: 640, class: "sheet", mask: "url(#fm1tut-mask)" });
    el("rect", { x: 0, y: 0, width: 1000, height: 640, fill: "url(#fm1tut-grid)", mask: "url(#fm1tut-mask)" });
    el("rect", { x: 4, y: 4, width: 992, height: 632, rx: 44, class: "edge" });
    el("rect", { x: 300, y: 44, width: 236, height: 236, rx: 26, class: "outline" });
    parts.title = el("text", { x: 44, y: 38, "font-size": 16, class: "title" }, svg, "");

    // the keys: faint, under everything else
    panel.querySelectorAll("rect.keyface").forEach(function (k) {
      el("rect", { x: k.getAttribute("x"), y: k.getAttribute("y"), width: k.getAttribute("width"), height: k.getAttribute("height"),
                   rx: k.getAttribute("rx"), class: "outline faint" });
    });
    var kg = el("g", { class: "part", "data-part": "keys:keys" });
    el("path", { d: "M380 546H300", class: "leader" }, kg);
    el("circle", { cx: 300, cy: 546, r: 3.2, class: "dot" }, kg);
    el("rect", { x: 380, y: 520, width: 240, height: 52, rx: 3, class: "box" }, kg);
    label(kg, 500, 541, "KEYS", 13, "sub");
    parts.keys = label(kg, 500, 562, "", 18, "name");
    hit(el("rect", { x: 36, y: 340, width: 928, height: 270, rx: 18 }, kg), "keys", "keys", "The keys");

    // the knobs: each one's name over it, what it sets now in a box under it
    KNOB_ORDER.forEach(function (name) {
      var k = knobs[name];
      if (!k) return;
      var g = el("g", { class: "part", "data-part": "knob:" + name }), top = k.y + k.r + 7;
      el("circle", { cx: k.x, cy: k.y, r: k.r, class: "outline" }, g);
      el("path", { d: "M" + k.x + " " + (k.y - k.r + 4) + "V" + (k.y - k.r + 14), class: "outline" }, g);
      label(g, k.x, k.y - k.r - 12, name, 14, "sub");
      el("path", { d: "M" + k.x + " " + (k.y + k.r) + "V" + top, class: "leader" }, g);
      el("rect", { x: k.x - 50, y: top, width: 100, height: 24, rx: 3, class: "box" }, g);
      parts["knob:" + name] = label(g, k.x, top + 17.5, "", 16, "name");
      hit(el("rect", { x: k.x - 52, y: k.y - k.r - 28, width: 104, height: 2 * k.r + 62, rx: 6 }, g), "knob", name, name);
    });

    // the buttons: outlined with their names; those that do something get a line to a box
    var used = [];
    Object.keys(buttons).forEach(function (name) {
      var b = buttons[name], d = T.buttons[name], oct = name.indexOf("OCT") === 0;
      var g = el("g", { class: "part" + (d && !d.unused ? "" : " unused"), "data-part": "button:" + name });
      el("rect", { x: b.x, y: b.y, width: b.w, height: b.h, rx: 9, class: "outline face" }, g);
      label(g, b.x + b.w / 2, b.y + b.h / 2 + 5, name, 14, "sub");
      b.g = g;
      if (d && !d.unused) {
        if (oct) {                                   // straight down to its box
          var cx = b.x + b.w / 2;
          el("path", { d: "M" + cx + " " + (b.y + b.h) + "V336", class: "leader" }, g);
          el("circle", { cx: cx, cy: b.y + b.h, r: 3.2, class: "dot" }, g);
          el("rect", { x: cx - 47, y: 336, width: 94, height: 30, rx: 3, class: "box" }, g);
          fitLater(label(g, cx, 357, d.label, 15, "name"), 86);
          hit(el("rect", { x: cx - 47, y: 336, width: 94, height: 30 }, g), "button", name, name + ": " + d.label);
        } else {
          // (the top row's lines come down the gap to the right of the button, the bottom row's from under it)
          b.sx = b.y < 230 ? b.x + b.w + 4.5 : b.x + b.w / 2;
          b.top = b.y < 230;
          used.push(name);
        }
      }
      hit(el("rect", { x: b.x - 3, y: b.y - 3, width: b.w + 6, height: b.h + 6, rx: 9 }, g), "button", name, name);
    });
    used.sort(function (a, c) { return buttons[a].sx - buttons[c].sx; });   // (in order: no two lines cross)
    var n = used.length, pitch = Math.min(144, 860 / Math.max(n, 1)), w = pitch - 8, x0 = 990 - n * pitch;
    used.forEach(function (name, i) {
      var b = buttons[name], d = T.buttons[name], g = b.g, bx = x0 + i * pitch + pitch / 2, by = 404;
      var path = b.top ? "M" + (b.x + b.w) + " " + (b.y + b.h / 2) + "H" + b.sx + "V296L" + bx + " " + by
                       : "M" + b.sx + " " + (b.y + b.h) + "V296L" + bx + " " + by;
      el("path", { d: path, class: "leader" }, g);
      el("circle", { cx: b.top ? b.x + b.w : b.sx, cy: b.top ? b.y + b.h / 2 : b.y + b.h, r: 3.2, class: "dot" }, g);
      el("rect", { x: bx - w / 2, y: by, width: w, height: 50, rx: 3, class: "box" }, g);
      fitLater(label(g, bx, by + 22, d.label, 18, "name"), w - 10);
      if (d.sub) fitLater(label(g, bx, by + 40, d.sub, 12.5, "sub"), w - 10);
      hit(el("rect", { x: bx - w / 2, y: by, width: w, height: 50 }, g), "button", name, name + ": " + d.label);
    });
    // the screen
    var sg = el("g", { class: "part", "data-part": "screen:screen" });
    hit(el("rect", { x: 316, y: 60, width: 204, height: 204, rx: 4 }, sg), "screen", "screen", "The screen");
    refresh();
  }
  var later = [];
  function fitLater(t, max) { later.push([t, max]); }

  // --- what the knobs and keys do now, and what is picked ------------------------------------------------------
  function refresh() {
    var p = page();
    parts.title.textContent = "KNOBS: " + p.title + " · " + p.how.toUpperCase();
    KNOB_ORDER.forEach(function (name) {
      var t = parts["knob:" + name];
      if (!t) return;
      var k = knobNow(name);
      t.textContent = k ? k[0] : "—";
      t.parentNode.classList.toggle("unused", !k);
      fit(t, 92);
    });
    parts.keys.textContent = keysNow()[0];
    svg.querySelectorAll(".part").forEach(function (g) {
      var id = g.getAttribute("data-part"), name = id.split(":")[1], d = T.buttons[name];
      g.classList.toggle("on", id === picked);
      g.classList.toggle("held", id.indexOf("button:") === 0 && !!d &&
                         ((hold && d.hold === hold) || (!hold && (d.base === base || (d.pages && d.pages.indexOf(base) >= 0)))));
    });
    later.forEach(function (x) { fit(x[0], x[1]); });
  }
  function say(title, where, text, note) {
    card.textContent = "";
    var h = document.createElement("strong");
    h.textContent = title;
    card.appendChild(h);
    if (where) { var s = document.createElement("span"); s.textContent = where; card.appendChild(s); }
    var p = document.createElement("p");
    p.textContent = text;
    card.appendChild(p);
    if (note) { var q = document.createElement("p"); q.className = "note"; q.textContent = note; card.appendChild(q); }
  }
  function choose(kind, name) {
    var p, d, k, note = "";
    picked = kind + ":" + name;
    if (kind === "button") {
      d = T.buttons[name];
      if (!d || d.unused) say(name, "", (d && d.unused) || "Does nothing in this firmware.");
      else {
        var before = hold || base;
        if (d.pages) { base = d.pages[(d.pages.indexOf(base) + 1) % d.pages.length]; hold = null; }   // (a button with two pages: the next)
        else if (d.base && base !== d.base) { base = d.base; hold = null; }
        else if (d.hold) hold = hold === d.hold ? null : d.hold;
        else if (d.base) hold = null;
        p = page();
        if ((hold || base) !== before || d.base || d.hold || d.pages) {
          note = "The knobs here now show " + p.title + " (" + p.how + ").";
          if (hold && d.hold === hold) note += " Tap " + name + " again to let go.";
          else if (d.hold && d.base === base && !hold) note += " Tap " + name + " again for its held set.";
          else if (d.pages) note += " Tap " + name + " again for " + T.pages[d.pages[(d.pages.indexOf(base) + 1) % d.pages.length]].title + ".";
        }
        say(name + " · " + d.label, d.sub || "", d.text, note);
      }
    } else if (kind === "knob") {
      p = page();
      k = knobNow(name);
      if (k) say(name + " · " + k[0], name === "MASTER" || (name === "SELECT" && T.select) ? "on every page" : p.title + " (" + p.how + ")", k[1]);
      else say(name, p.title + " (" + p.how + ")", "Sets nothing on this page.");
    } else if (kind === "keys") {
      k = keysNow();
      say("KEYS · " + k[0], page().keys || T.pages[base].keys ? T.pages[base].title + " (" + T.pages[base].how + ")" : "", k[1]);
    } else say("SCREEN", "", T.screen);
    refresh();
  }

  // --- the guide: the panel plays; a card says what to do next, and the control to use is ringed ---------------
  var BIT = { FX: 0, SEL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11 };
  var G = T.guide, gi = 0, gtimer = null, gwait = 0, rings = null;
  function met(c) {                                  // a step's "done", against what the instrument says of itself
    var f = window.emuLast, st = window.palState;
    if (!c) return false;
    if (c.on && !f) return false;
    if (c.lit) {
      if (!f) return false;
      var m = f.buttons >>> 0;
      if (!(m & (1 << BIT[c.lit])) && !(c.lit === "PLAY" && (m & (1 << 14)))) return false;   // (PLAY's green: bit 14)
    }
    if (c.keys && !(f && (f.keys >>> 0))) return false;
    if (c.rings !== undefined && !(st && st.rings >= c.rings)) return false;
    if (c.ring !== undefined && !(st && st.ring === c.ring)) return false;
    return true;
  }
  function ring(show) {                              // the controls a step uses, ringed on the live panel
    if (!rings) {
      rings = document.createElementNS(NS, "svg");
      rings.setAttribute("id", "fm1guide");
      rings.setAttribute("viewBox", "0 0 1000 640");
      rings.setAttribute("aria-hidden", "true");
      device.appendChild(rings);
    }
    rings.textContent = "";
    (show || []).forEach(function (name) {
      var b = buttons[name], k = knobs[name];
      if (name === "keys") el("rect", { x: 40, y: 344, width: 920, height: 262, rx: 16, class: "use" }, rings);
      else if (b) el("rect", { x: b.x - 5, y: b.y - 5, width: b.w + 10, height: b.h + 10, rx: 12, class: "use" }, rings);
      else if (k) el("circle", { cx: k.x, cy: k.y, r: k.r + 12, class: "use" }, rings);
    });
  }
  function gbutton(text, fn, cls) {
    var b = document.createElement("button");
    b.type = "button";
    b.textContent = text;
    if (cls) b.className = cls;
    b.addEventListener("click", fn);
    return b;
  }
  function gshow() {
    var st = G.steps[gi], row = document.createElement("div"), h = document.createElement("strong"),
        s = document.createElement("span"), p = document.createElement("p");
    card.textContent = "";
    gwait = 0;
    row.className = "row";
    if (!st) {                                       // through
      p.textContent = G.done;
      card.appendChild(p);
      row.appendChild(gbutton("Start again", function () { gi = 0; gshow(); }));
      row.appendChild(gbutton("Play", function () { mode("play"); }));
      if (svg) row.appendChild(gbutton(T.tutorial_label || "Tutorial", function () { mode("tutorial"); }));
      card.appendChild(row);
      ring([]);
      return;
    }
    h.textContent = st.title;
    s.textContent = "Step " + (gi + 1) + " of " + G.steps.length;
    p.textContent = st.text;
    card.appendChild(h); card.appendChild(s); card.appendChild(p);
    if (gi > 0) row.appendChild(gbutton("Back", function () { gi--; gshow(); }));
    row.appendChild(gbutton(gi === G.steps.length - 1 ? "Finish" : "Next", function () { gi++; gshow(); }));
    row.appendChild(gbutton("Exit", function () { mode("play"); }, "quiet"));
    card.appendChild(row);
    ring(st.show);
  }
  function gtick() {                                 // the step done on the instrument itself: a moment, then on
    var st = G.steps[gi];
    if (!st) return;
    if (gwait) { if (Date.now() >= gwait) { gi++; gshow(); } return; }
    if (met(st.done)) {
      gwait = Date.now() + 900;
      card.classList.add("done");
      setTimeout(function () { card.classList.remove("done"); }, 900);
    }
  }

  // --- the switch ---------------------------------------------------------------------------------------------
  var controls = document.getElementById("controls"), wasInert = null, now = "play";
  function mode(m) {
    if (m === "guide" && !G) m = "play";
    if (m === "tutorial" && !svg) return;
    pill.querySelectorAll("button").forEach(function (b) { b.setAttribute("aria-pressed", String(b.dataset.mode === m)); });
    // out of the mode that was
    if (now === "tutorial" && m !== "tutorial") {
      svg.setAttribute("hidden", "");
      if (controls && wasInert !== null) controls.inert = wasInert;
      wasInert = null;
    }
    if (now === "guide" && m !== "guide") {
      if (gtimer) { clearInterval(gtimer); gtimer = null; }
      if (rings) rings.textContent = "";
    }
    now = m;
    card.hidden = m === "play";
    card.classList.toggle("guide", m === "guide");
    device.classList.toggle("fm1tut-on", m === "tutorial");
    if (m === "tutorial") {
      svg.removeAttribute("hidden");
      card.textContent = "";
      var p = document.createElement("p");
      p.textContent = T.intro;
      card.appendChild(p);
      picked = null;
      if (controls) { wasInert = controls.inert; controls.inert = true; }      // (the real controls: out of reach)
      refresh();
    } else if (m === "guide") {
      gshow();
      gtimer = setInterval(gtick, 150);
    }
  }
  pill.addEventListener("click", function (e) {
    var b = e.target.closest("button");
    if (b) mode(b.dataset.mode);
  });
  build();
})();
