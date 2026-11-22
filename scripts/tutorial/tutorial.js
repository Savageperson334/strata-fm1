// SPDX-License-Identifier: MPL-2.0
// The emulator's Play / Tutorial switch (scripts/site_nav.py puts this, tutorial.css and one firmware's
// tutorial JSON into the emulator's page).
//
//   Play      the panel as it is: buttons, knobs and keys play the instrument.
//   Tutorial  a blueprint drawn over the panel: every control outlined, a line from each one to a box that says
//             what it does in this firmware. Tapping a control here does not play it: it shows what it does in the
//             card over the panel. A button that changes what the knobs or keys do (a mode, or a layer held)
//             changes the boxes too: tap it once for "held", again to let go.
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
  function keysNow() { return page().keys || T.pages[base].keys; }

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
      var k = name === "MASTER" ? T.master : p.knobs[name];
      t.textContent = k ? k[0] : "—";
      t.parentNode.classList.toggle("unused", !k);
      fit(t, 92);
    });
    parts.keys.textContent = keysNow()[0];
    svg.querySelectorAll(".part").forEach(function (g) {
      var id = g.getAttribute("data-part"), name = id.split(":")[1], d = T.buttons[name];
      g.classList.toggle("on", id === picked);
      g.classList.toggle("held", id.indexOf("button:") === 0 && !!d && ((hold && d.hold === hold) || (!hold && d.base === base)));
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
        if (d.base && base !== d.base) { base = d.base; hold = null; }
        else if (d.hold) hold = hold === d.hold ? null : d.hold;
        else if (d.base) hold = null;
        p = page();
        if ((hold || base) !== before || d.base || d.hold) {
          note = "The knobs here now show " + p.title + " (" + p.how + ").";
          if (hold && d.hold === hold) note += " Tap " + name + " again to let go.";
          else if (d.hold && d.base === base && !hold) note += " Tap " + name + " again for its held set.";
        }
        say(name + " · " + d.label, d.sub || "", d.text, note);
      }
    } else if (kind === "knob") {
      p = page();
      k = name === "MASTER" ? T.master : p.knobs[name];
      if (k) say(name + " · " + k[0], name === "MASTER" ? "always" : p.title + " (" + p.how + ")", k[1]);
      else say(name, p.title + " (" + p.how + ")", "Sets nothing on this page.");
    } else if (kind === "keys") {
      k = keysNow();
      say("KEYS · " + k[0], T.pages[base].title + " (" + T.pages[base].how + ")", k[1]);
    } else say("SCREEN", "", T.screen);
    refresh();
  }

  // --- the switch ---------------------------------------------------------------------------------------------
  var controls = document.getElementById("controls"), wasInert = false;
  function mode(tutorial) {
    if (!svg) return;
    pill.querySelectorAll("button").forEach(function (b) { b.setAttribute("aria-pressed", String((b.dataset.mode === "tutorial") === tutorial)); });
    if (tutorial) {
      svg.removeAttribute("hidden");
      card.hidden = false;
      if (!picked) { card.textContent = ""; var p = document.createElement("p"); p.textContent = T.intro; card.appendChild(p); }
      if (controls) { wasInert = controls.inert; controls.inert = true; }      // (the real controls: out of reach)
      refresh();
    } else {
      svg.setAttribute("hidden", "");
      card.hidden = true;
      if (controls) controls.inert = wasInert;
    }
    device.classList.toggle("fm1tut-on", tutorial);
  }
  pill.addEventListener("click", function (e) {
    var b = e.target.closest("button");
    if (b) mode(b.dataset.mode === "tutorial");
  });
  build();
})();
