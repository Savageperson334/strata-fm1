/* SPDX-License-Identifier: MPL-2.0
 *
 * STRABBLEDROME: the launcher. One firmware, three instruments behind one menu; one of them runs at a time.
 *
 * This is milestone 1 of instruments/strabbledrome/README.md: the real app shell and its menu, on the simulator,
 * in the browser and on the FM-1, before any sound engine is built in. An instrument is a row of the table
 * ENGINE below: its name and the seven calls every FM-1 app has. A row with no calls is an instrument that is
 * not in this build yet; choosing it shows that, and makes no sound.
 *
 *   GLO      one press: the menu, from anywhere. In the menu: back
 *   SELECT   in the menu: the highlight. Anywhere else it is the instrument's own
 *   SEL      in the menu: open what is highlighted. Anywhere else the instrument's own
 *   HOME     in the menu: back. Anywhere else the instrument's own
 *
 * Nothing is held and no two buttons are pressed together. While the menu is open the instrument is not given
 * the panel: no key, knob or button reaches it.
 *
 * Changing instrument. The audio interrupt reads `owner` once a block and calls that instrument's render, or
 * writes silence for -1. To change: owner = -1; when a block has been rendered since (so nothing of the old
 * instrument is still running), the old one is silenced, the shared workspace cleared, the new one started, and
 * owner set to it. See change() and finish_change().
 *
 * The screen is 240 x 240, drawn as two strips of 120: no line of text crosses y = 120. */
#include "fastmath.h"

typedef struct {
    const char *name, *what;
    void (*init)(void), (*ui_init)(void), (*frame)(void), (*input)(void), (*panic)(void);
    void (*render)(int32_t *lr, uint32_t n, uint32_t master_q12);
    void (*say)(const char *a, const char *b);
} engine;
enum { E_STRATA, E_PALINDROME, E_BABBLEBEAT, NENGINE };
static const engine ENGINE[NENGINE] = {
    {"STRATA", "DRONE / RHYTHM", 0, 0, 0, 0, 0, 0, 0},
    {"PALINDROME", "GRANULAR RINGS", 0, 0, 0, 0, 0, 0, 0},
    {"BABBLEBEAT", "ROBOTIC SPEECH", 0, 0, 0, 0, 0, 0, 0},
};
static int ready(int e) { return e >= 0 && e < NENGINE && ENGINE[e].render != 0; }

/* the menu's rows: the instruments, then these two */
enum { M_HELP = NENGINE, M_SYSTEM, NMENU };
enum { V_MENU, V_ENGINE, V_HELP, V_SYSTEM };
static int view, cursor;
static int chosen = -1;                  /* the instrument chosen (-1: none yet), whether or not it is in the build */
static volatile int owner = -1;          /* the instrument the audio interrupt renders (-1: silence) */
static volatile uint32_t blocks;         /* audio blocks rendered: how a change knows the old instrument has stopped */
static int changing = -1;                /* a change under way: to this instrument */
static uint32_t change_block, change_ms;
static uint32_t old_btn, frames, message_until;
static char message[36];

void app_say(const char *a, const char *b)
{
    int n = 0;
    if (view == V_ENGINE && ready(owner) && ENGINE[owner].say) { ENGINE[owner].say(a, b); return; }
    while (a && *a && n < 33) message[n++] = *a++;
    if (b && *b && n < 32) message[n++] = ' ';
    while (b && *b && n < 33) message[n++] = *b++;
    message[n] = 0; message_until = plat_ms() + 1400u;
}

/* one instrument out, another in: asked for here, done in finish_change once the audio has let go */
static void change(int e)
{
    chosen = e;
    if (e == owner && ready(e)) return;      /* (already running: back to it as it is) */
    owner = -1;
    changing = e; change_block = blocks; change_ms = plat_ms();
}
static void finish_change(void)
{
    if (changing < 0) return;
    /* a block rendered with owner -1, or (no audio running at all) a tenth of a second */
    if (blocks == change_block && plat_ms() - change_ms < 100u) return;
    for (int e = 0; e < NENGINE; e++) if (ready(e) && ENGINE[e].panic) ENGINE[e].panic();
    if (ready(changing)) {
        ENGINE[changing].init();
        if (ENGINE[changing].ui_init) ENGINE[changing].ui_init();
        owner = changing;
    }
    changing = -1;
}

void app_init(void) { view = V_MENU; cursor = 0; chosen = -1; owner = -1; changing = -1; old_btn = frames = message_until = 0; message[0] = 0; }
void app_panic(void) { int e = owner; owner = -1; if (ready(e) && ENGINE[e].panic) ENGINE[e].panic(); }
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    int e = owner;
    blocks++;
    if (ready(e)) { ENGINE[e].render(lr, n, master_q12); return; }
    for (uint32_t i = 0; i < 2u * n; i++) lr[i] = 0;
}

void app_input_only(void)
{
    uint32_t btn = plat_buttons(), nb = btn & ~old_btn;
    old_btn = btn;
    finish_change();
    if (view == V_ENGINE) {
        if (nb & (1u << B_GLO)) { view = V_MENU; cursor = chosen >= 0 ? chosen : 0; plat_enc(EN_SELECT); return; }
        if (ready(owner) && ENGINE[owner].input) ENGINE[owner].input();
        return;
    }
    /* the menu and its two pages own the panel: nothing reaches an instrument from here */
    if (view == V_MENU) {
        int d = plat_enc(EN_SELECT);
        cursor += d;
        if (cursor < 0) cursor = 0;
        if (cursor >= NMENU) cursor = NMENU - 1;
        if (nb & (1u << B_SEL)) {
            if (cursor < NENGINE) { change(cursor); view = V_ENGINE; }
            else view = cursor == M_HELP ? V_HELP : V_SYSTEM;
        } else if (nb & ((1u << B_GLO) | (1u << B_HOME))) {
            if (chosen >= 0) view = V_ENGINE;
            else app_say("CHOOSE AN INSTRUMENT:", "SEL");
        }
    } else if (nb & ((1u << B_GLO) | (1u << B_HOME) | (1u << B_SEL))) view = V_MENU;
}

/* ---- the screen ---- */
static int32_t yoff;
static int text(int x, int y, const char *s, uint16_t c)
{
    if (y < yoff || y + 14 > yoff + 120) return x + text_w(&FONT_XS, s);
    return cv_text(x, y - yoff, &FONT_XS, s, c);
}
static void bar(int x, int y, int w, int h, uint16_t c) { cv_rect(x, y - yoff, w, h, c); }
static void head(const char *left, const char *right)
{
    text(6, 4, left, C_HI);
    text(234 - text_w(&FONT_XS, right), 4, right, C_GRAY);
    bar(6, 20, 228, 1, C_LINE);
}
static const char *const ROW[NMENU] = {0, 0, 0, "HELP / CONTROLS", "SYSTEM"};
static void scene(void)
{
    if (view == V_MENU) {
        head("STRABBLEDROME", "MENU");
        for (int i = 0; i < NMENU; i++) {
            int y = 28 + 17 * i, on = i == cursor;
            if (on) bar(4, y - 1, 232, 16, C_LINE);
            text(8, y, on ? ">" : " ", C_WHITE);
            text(20, y, i < NENGINE ? ENGINE[i].name : ROW[i], on ? C_WHITE : C_HI);
            if (i < NENGINE) text(112, y, ENGINE[i].what, on ? C_AMB : C_GRAY);
            if (i == chosen) text(226, y, "*", C_AMB);
        }
        if (plat_ms() < message_until) text(8, 126, message, C_AMB);
        else if (chosen >= 0) { text(text(8, 126, "* NOW: ", C_GRAY), 126, ENGINE[chosen].name, C_AMB); }
        text(8, 150, "SELECT: CHOOSE", C_GRAY);
        text(8, 164, "SEL: OPEN", C_GRAY);
        text(8, 178, "GLO OR HOME: BACK", C_GRAY);
        text(8, 206, "ONE INSTRUMENT RUNS AT A TIME", C_LINE);
    } else if (view == V_HELP) {
        head("HELP / CONTROLS", "GLO: BACK");
        text(8, 28, "GLO     THE MENU, FROM ANYWHERE", C_WHITE);
        text(8, 46, "IN THE MENU", C_AMB);
        text(8, 60, "SELECT  THE HIGHLIGHT", C_HI);
        text(8, 74, "SEL     OPEN IT", C_HI);
        text(8, 88, "GLO, HOME  BACK", C_HI);
        text(8, 126, "IN AN INSTRUMENT", C_AMB);
        text(8, 140, "EVERY OTHER KNOB, KEY AND BUTTON", C_HI);
        text(8, 154, "IS THE INSTRUMENT'S OWN.", C_HI);
        text(8, 178, "NOTHING IS HELD. NO TWO BUTTONS", C_GRAY);
        text(8, 192, "ARE PRESSED TOGETHER.", C_GRAY);
    } else if (view == V_SYSTEM) {
        int n = 0;
        char count[2] = {'0', 0};
        for (int e = 0; e < NENGINE; e++) n += ready(e);
        count[0] = (char)('0' + n);
        head("SYSTEM", "GLO: BACK");
        text(text(8, 28, "STRABBLEDROME ", C_WHITE), 28, APP_VERSION, C_HI);
        text(text(text(8, 46, "INSTRUMENTS IN THIS BUILD: ", C_GRAY), 46, count, C_AMB), 46, " OF 3", C_GRAY);
        for (int e = 0; e < NENGINE; e++) {
            text(20, 64 + 14 * e, ENGINE[e].name, C_HI);
            text(112, 64 + 14 * e, ready(e) ? "BUILT IN" : "NOT YET", ready(e) ? C_AMB : C_GRAY);
        }
        text(8, 126, "ONE INSTRUMENT RUNS AT A TIME.", C_GRAY);
        text(8, 140, "CHANGING IT STOPS THE SOUND AND", C_GRAY);
        text(8, 154, "STARTS THE OTHER AFRESH.", C_GRAY);
    } else {
        /* an instrument that is not in this build: its name, and the way back */
        head(chosen >= 0 ? ENGINE[chosen].name : "", "GLO: MENU");
        text(8, 46, chosen >= 0 ? ENGINE[chosen].what : "", C_GRAY);
        text(8, 74, "NOT IN THIS BUILD YET.", C_AMB);
        text(8, 88, "THE LAUNCHER IS WHAT IS HERE.", C_GRAY);
        text(8, 126, "PRESS GLO FOR THE MENU.", C_WHITE);
    }
}
static void draw(void)
{
    for (int i = 0; i < 2; i++) { yoff = i * 120; cv_begin(240, 120, C_BLACK); scene(); cv_blit(0, (uint32_t)yoff); }
}
void app_ui_init(void) { lcd_fill(0, 0, 240, 240, C_BLACK); draw(); }
void app_frame(void)
{
    app_input_only();
    if (view == V_ENGINE && ready(owner)) { if (ENGINE[owner].frame) ENGINE[owner].frame(); return; }
    /* the launcher's own lights: GLO always (the way to the menu, or back); in the menu SEL and HOME too */
    plat_lights(view == V_ENGINE ? 0 : 1u << B_GLO, 0,
                view == V_ENGINE ? 1u << B_GLO : view == V_MENU ? (1u << B_SEL) | (1u << B_HOME) : 0, 0, 0);
    if ((++frames & 3u) == 0) draw();
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "view")) *got = view;
    else if (!strcmp(what, "cursor")) *got = cursor;
    else if (!strcmp(what, "chosen")) *got = chosen;
    else if (!strcmp(what, "owner")) *got = owner;
    else if (!strcmp(what, "changing")) *got = changing;
    else if (!strcmp(what, "ready")) { *got = 0; for (int e = 0; e < NENGINE; e++) *got += ready(e); }
    else if (!strcmp(what, "note")) *got = plat_ms() < message_until;
    else return 0;
    return 1;
}
#endif
