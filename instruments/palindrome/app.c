/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME's panel and screen (docs/PALINDROME_SILT_FIRST_REDESIGN.md).
 *
 * SILT is the instrument and the page the FM-1 starts on; MORPH is the injector. One press is one thing: a button
 * either opens a page, which stays until another is opened, or does its one action. Nothing is held.
 *
 *   EDIT, HOME  SILT: the grains of the selected ring (POSITION SIZE DENSITY SPRAY, REVERSE, PITCH)
 *   FX          MORPH SOURCE: the four-corner synth (X Y COMPLEX TONE, MOVE, EXCITE); the keys are its notes
 *   SEL         RINGS: the rings there are and where each came from; CARRY; KNOB4 arms a delete, SEL confirms it
 *   ENV         SILT SHAPE: SHAPE VARY SCATTER DETUNE, CLUSTER
 *   LFO         SILT MOTION: SCAN, TIMING
 *   ARP         MIX
 *   SEQ         REC FX: what the next capture does to the sound it records
 *   GLO, held   the guide: what every button does, on the screen, laid out as the buttons are
 *   REC         captures the source into a NEW ring: MORPH on MORPH's page, else SILT's grains of the selected ring
 *               (MORPH while there is no ring). It runs one ring's length and finishes by itself; pressed
 *               meanwhile it does nothing but say so. No room: it says so and opens RINGS; nothing is deleted.
 *   PLAY        the stream of grains, on and off
 *   SAVE        nothing: it says so
 *
 * SELECT is the ring selector on every page. Everywhere but on MORPH's page the keys hold the grains' pitches.
 *
 * The screen: the rings as concentric circles, the oldest innermost, each its own recording's wave; the selected
 * one bright and dense. Grains ride the ring they play. A capture draws itself as the next circle filling. MORPH is
 * the small square in the middle (large on its own page). Top: the page, the keys' job, PLAY, the selected ring.
 * Under the rings: the SOURCE REC would capture, and REC's state. At the foot: the knobs by what they set now,
 * placed as they are on the panel. */
#include "palindrome.h"
#include "fastmath.h"

enum { PG_SILT, PG_MORPH, PG_RINGS, PG_SHAPE, PG_MOTION, PG_MIX, PG_RECFX, NPAGES };
static int page, source, octave, running, slots[PM_VOICES], replace_at;
static int gslots[PM_PITCHES], greplace_at;  /* the keys held for the grains: semitones, PM_NO_PITCH none */
static int arm;                          /* RINGS: 0, or what SEL would now do: 1 delete the selected ring, 2 clear all */
static int guide;                        /* GLO is held: the guide is on the screen */
static int played;                       /* PLAY has been pressed once (its hint no longer needed) */
static uint32_t old_keys, old_btn, frames;
static float gain_now;
static float audio_l[APP_BLOCK],audio_r[APP_BLOCK];
static char message[40];
static uint32_t message_until;
static int touched=-1;                   /* the knob turned last (0..6): its box white, its value in it, for a moment */
static uint32_t touched_at;
static const int enc[7]={EN_K1,EN_K2,EN_K3,EN_K4,EN_SELECT,EN_ALGO,EN_PRESET};
static const char *const labels[PM_PARAMS]={
 "X","Y","COMPLEX","TONE","SPREAD","MOVE","EXCITE",
 "POSITION","SIZE","DENSITY","SPRAY","PITCH","REVERSE","CARRY",
 "MIX","FADE","SCAN","SHAPE","STEREO","DAMP","LEVEL",
 "NOISE","COLOR","CRUSH","GATE","DROP",
 "TIMING","VARY","SCATTER","DETUNE","CLUSTER"};
/* the control on knob j (KNOB1..4, SELECT, ALGORITHM, PRESETS) of a page, or PG_NONE (that knob sets nothing there).
 * SELECT (j 4) is the ring selector on every page, and KNOB4 on RINGS arms a delete: neither is in this table */
#define PG_NONE 255
static const uint8_t PAGE[NPAGES][7]={
    {S_POS,S_SIZE,S_DENSITY,S_SPRAY,PG_NONE,S_REVERSE,S_PITCH},                 /* SILT */
    {M_X,M_Y,M_COMPLEX,M_TONE,PG_NONE,M_MOVE,M_EXCITE},                         /* MORPH (SPREAD: on MIX) */
    {R_CARRY,PG_NONE,PG_NONE,PG_NONE,PG_NONE,PG_NONE,PG_NONE},                  /* RINGS */
    {S_SHAPE,S_VARY,S_SCATTER,S_DETUNE,PG_NONE,S_CLUSTER,PG_NONE},              /* SHAPE */
    {G_SCAN,S_TIMING,PG_NONE,PG_NONE,PG_NONE,PG_NONE,PG_NONE},                  /* MOTION */
    {G_MIX,G_FADE,M_SPREAD,G_STEREO,PG_NONE,PG_NONE,G_LEVEL},                   /* MIX */
    {R_NOISE,R_COLOR,R_CRUSH,R_GATE,PG_NONE,R_DROP,G_DAMP}};                    /* REC FX */
static int pidx(int pg,int j){return PAGE[pg][j]==PG_NONE?-1:PAGE[pg][j];}
static const uint8_t PAGE_LED[NPAGES]={B_EDIT,B_FX,B_SEL,B_ENV,B_LFO,B_ARP,B_SEQ};

static void number(int v,char *s){
    char t[12];int n=0;unsigned u;
    if(v<0){*s++='-';u=(unsigned)(-v);}else u=(unsigned)v;
    do{t[n++]=(char)('0'+u%10u);u/=10u;}while(u);
    while(n)*s++=t[--n];
    *s=0;
}
static char *two(int v,char *s){*s++=(char)('0'+(v/10)%10);*s++=(char)('0'+v%10);*s=0;return s;}   /* "02" */
static char *put(char *d,const char *s){while(*s)*d++=*s++;*d=0;return d;}
void app_say(const char *a,const char *b){
    uint32_t i=0;
    for(;a&&a[i]&&i<38;i++)message[i]=a[i];
    if(b&&*b&&i<37){message[i++]=' ';while(*b&&i<38)message[i++]=*b++;}
    message[i]=0;message_until=plat_ms()+1600u;
}
static int voices(void){int n=0;for(int j=0;j<PM_VOICES;j++)n+=slots[j]>=0;return n;}
static int gkeys(void){int n=0;for(int j=0;j<PM_PITCHES;j++)n+=gslots[j]!=PM_NO_PITCH;return n;}
static void all_off(void){
    for(int j=0;j<PM_VOICES;j++){slots[j]=-1;pm_note(j,-1);}
    for(int j=0;j<PM_PITCHES;j++){gslots[j]=PM_NO_PITCH;pm_grain_pitch(j,PM_NO_PITCH);}
}
static void latch(int note){
    int empty=-1;
    for(int j=0;j<PM_VOICES;j++){
        if(slots[j]==note){slots[j]=-1;pm_note(j,-1);return;}
        if(slots[j]<0&&empty<0)empty=j;
    }
    if(empty<0)empty=(replace_at++)%PM_VOICES;
    slots[empty]=note;pm_note(empty,note);
}
/* SILT's keys: a tap holds a pitch for the grains and plays it at once, a tap on a held one lets it go */
static void glatch(int semis){
    int empty=-1;
    if(semis<-48||semis>48)return;
    for(int j=0;j<PM_PITCHES;j++){
        if(gslots[j]==semis){gslots[j]=PM_NO_PITCH;pm_grain_pitch(j,PM_NO_PITCH);return;}
        if(gslots[j]==PM_NO_PITCH&&empty<0)empty=j;
    }
    if(empty<0)empty=(greplace_at++)%PM_PITCHES;
    gslots[empty]=semis;pm_grain_pitch(empty,semis);pm_strike(empty);
}

/* --- the rings, oldest first (by their numbers), as the screen and SELECT take them --- */
static int order[PM_RINGS],norder;
static void rings_now(void){
    int ids[PM_RINGS],id;
    norder=0;
    for(int s=0;s<PM_RINGS;s++)if(pm_ring_info(s,&id,0,0)){
        int k=norder++;
        while(k>0&&ids[k-1]>id){ids[k]=ids[k-1];order[k]=order[k-1];k--;}
        ids[k]=id;order[k]=s;
    }
}
static int ring_id(int slot){int id=0;return pm_ring_info(slot,&id,0,0)?id:0;}
static int place_of(int slot){for(int k=0;k<norder;k++)if(order[k]==slot)return k;return -1;}
/* what REC would capture now: MORPH on its page and while there is no ring, else as FX or EDIT last left it */
static int source_now(void){return source==PM_SRC_SILT&&pm_ring_count()>0?PM_SRC_SILT:PM_SRC_MORPH;}
static void say_ring(const char *before,int slot,const char *after){
    char t[40],*e=put(t,before);
    e=two(ring_id(slot),e);
    put(e,after);
    app_say(t,"");
}

/* sine and cosine of 256 steps round the circle, x 127: the rings' geometry, made once */
static int8_t pd_sin[256],pd_cos[256];
void app_init(void){
    pm_init();page=PG_SILT;source=PM_SRC_MORPH;octave=-1;running=0;replace_at=greplace_at=0;arm=0;played=0;
    old_keys=old_btn=frames=message_until=0;gain_now=0;touched=-1;guide=0;
    for(int i=0;i<256;i++){
        pd_sin[i]=(int8_t)(fm_sin_turns((float)i*(1.f/256.f))*127.f);
        pd_cos[i]=(int8_t)(fm_sin_turns((float)i*(1.f/256.f)+.25f)*127.f);
    }
    all_off();
}
void app_panic(void){pm_panic();all_off();running=0;}
void app_render(int32_t *lr,uint32_t n,uint32_t master_q12){
    /* Audio frame sizes are bounded by APP_BLOCK, but handle arbitrary n defensively. */
    float gain=(float)master_q12*(8300000.f/4096.f);
    while(n){
        uint32_t k=n>APP_BLOCK?APP_BLOCK:n;
        pm_render(audio_l,audio_r,k);
        for(uint32_t i=0;i<k;i++){
            gain_now+=(gain-gain_now)*.004f;
            *lr++=(int32_t)(audio_l[i]*gain_now);*lr++=(int32_t)(audio_r[i]*gain_now);
        }
        n-=k;
    }
}
/* REC: one press, one new ring (or the reason there is none) */
static void capture(void){
    int src=source_now(),r;
    if(pm_capture_state()!=PM_CAP_IDLE){app_say("CAPTURING: IT ENDS BY ITSELF","");return;}
    if(pm_ring_count()>=PM_RINGS){page=PG_RINGS;app_say("RINGS FULL: DELETE ONE HERE","");return;}
    /* nothing to record: say what would make a sound, start nothing (NOISE and CARRY are sound of their own) */
    if(src==PM_SRC_MORPH&&!voices()&&pm_get(R_NOISE)<.02f&&!(pm_get(R_CARRY)>.02f&&pm_ring_selected()>=0)){
        app_say(page==PG_MORPH?"NO SOUND: TAP KEYS FIRST":"NO SOUND: FX, THEN TAP KEYS","");return;
    }
    if(src==PM_SRC_SILT&&!running&&!pm_active_grains()&&pm_get(R_NOISE)<.02f&&pm_get(R_CARRY)<.02f){
        app_say("NO GRAINS: PRESS PLAY FIRST","");return;
    }
    r=pm_capture(src);
    if(r==PM_ERR_FULL){page=PG_RINGS;app_say("RINGS FULL: DELETE ONE HERE","");}
    else if(r)app_say("CAPTURING: IT ENDS BY ITSELF","");
}
static void select_ring(int d,uint32_t now){
    int at;
    rings_now();
    touched=4;touched_at=now;
    if(!norder){app_say("NO RINGS YET","");return;}
    if(pm_capture_state()!=PM_CAP_IDLE){app_say("CAPTURING: THEN SELECT","");return;}
    at=place_of(pm_ring_selected())+(d>0?1:-1);
    if(at<0)at=0;
    if(at>=norder)at=norder-1;
    pm_ring_select(order[at]);
    arm=0;
}
void app_input_only(void){
    uint32_t keys=plat_keys(),btn=plat_buttons(),newb=btn&~old_btn,newk=keys&~old_keys,now=plat_ms();
    int slot=-1,ev=pm_service(&slot);
    /* a capture that has finished: the new ring is selected, and SILT is where it is played */
    if(ev==PM_EV_COMMITTED){page=PG_SILT;source=PM_SRC_SILT;arm=0;say_ring("RING ",slot," CAPTURED");}
    else if(ev==PM_EV_SILENT)app_say("NO SOUND: NOTHING CAPTURED","");
    /* every button: its one press */
    if((newb&(1u<<B_SEL))&&page==PG_RINGS&&arm){             /* the armed delete, confirmed */
        if(arm==2){pm_ring_clear();all_off();running=0;pm_grains(0);app_say("ALL RINGS CLEARED","");}
        else if(pm_ring_selected()>=0){
            int s=pm_ring_selected();
            say_ring("RING ",s," DELETED");
            pm_ring_delete(s);
        }
        arm=0;newb&=~(1u<<B_SEL);
    }
    if(newb&~((1u<<B_OCTDN)|(1u<<B_OCTUP)|(1u<<B_GLO)))arm=0;   /* any other button (but the guide's): not deleted */
    if(newb&(1u<<B_EDIT)){page=PG_SILT;source=PM_SRC_SILT;}
    if(newb&(1u<<B_HOME)){page=PG_SILT;source=PM_SRC_SILT;}
    if(newb&(1u<<B_FX)){page=PG_MORPH;source=PM_SRC_MORPH;}
    if(newb&(1u<<B_SEL))page=PG_RINGS;
    if(newb&(1u<<B_ENV))page=PG_SHAPE;
    if(newb&(1u<<B_LFO))page=PG_MOTION;
    if(newb&(1u<<B_ARP))page=PG_MIX;
    if(newb&(1u<<B_SEQ))page=PG_RECFX;
    guide=(int)((btn>>B_GLO)&1u);
    if(newb&(1u<<B_PLAY)){running=!running;pm_grains(running);played=1;}
    if(newb&(1u<<B_REC))capture();
    if(newb&(1u<<B_SAVE))app_say("SAVE: NOT AVAILABLE","");
    if((newb&(1u<<B_OCTDN))&&octave>-3)octave--;
    if((newb&(1u<<B_OCTUP))&&octave<2)octave++;
    /* the keys: MORPH's notes on its page, the grains' pitches everywhere else */
    for(int k=0;k<NKEYS;k++)if(newk&(1u<<k)){
        if(page==PG_MORPH)latch(53+12*octave+k);
        else glatch(53+12*(octave+1)+k-60);                  /* (the C key, as the keys start: the recorded pitch) */
    }
    for(int j=0;j<7;j++){
        int d=plat_enc(enc[j]),id=pidx(page,j);
        if(!d)continue;
        if(j==4){select_ring(d,now);continue;}               /* SELECT: the ring, on every page */
        if(page==PG_RINGS&&j==3){                            /* KNOB4 on RINGS: arm a delete (SEL confirms) */
            arm+=d>0?1:-1;
            if(arm<0)arm=0;
            if(arm>2)arm=2;
            if(arm==1&&pm_ring_selected()<0)arm=d>0?2:0;
            touched=j;touched_at=now;
            continue;
        }
        if(id<0)continue;
        pm_set(id,fm_clampf(pm_get(id)+(float)d/64.f,0,1));
        touched=j;touched_at=now;        /* (its box shows the value for a moment) */
    }
    old_keys=keys;old_btn=btn;
    {
        /* the lights: the page's button, PLAY while the grains run, REC blinking through a capture; the buttons
         * that do something dim, the three that do nothing dark */
        uint32_t lit=(1u<<PAGE_LED[page])|(running?LED_PLAY_GREEN:0),kl=0;
        if(pm_capture_state()!=PM_CAP_IDLE&&((now>>7)&1u))lit|=1u<<B_REC;
        if(guide)lit|=1u<<B_GLO;
        if(page==PG_MORPH)for(int j=0;j<PM_VOICES;j++){
            int k=slots[j]-(53+12*octave);
            if(k>=0&&k<NKEYS)kl|=1u<<k;
        }
        else for(int j=0;j<PM_PITCHES;j++){
            int k=gslots[j]+60-(53+12*(octave+1));
            if(gslots[j]!=PM_NO_PITCH&&k>=0&&k<NKEYS)kl|=1u<<k;
        }
        plat_lights(lit,kl,(1u<<B_FX)|(1u<<B_SEL)|(1u<<B_ENV)|(1u<<B_LFO)|(1u<<B_EDIT)|(1u<<B_GLO)|(1u<<B_HOME)|
                           (1u<<B_ARP)|(1u<<B_SEQ)|(1u<<B_PLAY)|(1u<<B_REC),0,1);
    }
}

/* ------------------------------------------------------------- the picture --- */
#define PD_CX 120
#define PD_CY 96
#define PD_AMP 6                         /* how far a ring's wave reaches in and out of its circle */
static const int32_t PD_R[PM_RINGS]={42,59,76};   /* the rings' radii, the oldest innermost */
/* the screen is drawn as two strips of 240 x 120 (the canvas holds no more): the whole picture twice, each time
 * the part in the strip kept. Text is placed so that no line of it crosses from one strip to the other */
static int32_t pd_y0;
static void pd_line(int32_t x0,int32_t y0,int32_t x1,int32_t y1,uint16_t c){
    if((y0<pd_y0&&y1<pd_y0)||(y0>=pd_y0+120&&y1>=pd_y0+120))return;
    cv_line(x0,y0-pd_y0,x1,y1-pd_y0,c);
}
static void pd_box(int32_t x,int32_t y,int32_t w,int32_t h,uint16_t c){cv_rect(x,y-pd_y0,w,h,c);}
static int32_t pd_text(int32_t x,int32_t y,const char *s,uint16_t c){
    if(y<pd_y0||y+14>pd_y0+120)return x+text_w(&FONT_XS,s);
    return cv_text(x,y-pd_y0,&FONT_XS,s,c);
}
static void pd_frame(int32_t x,int32_t y,int32_t w,int32_t h,uint16_t c){
    pd_box(x,y,w,1,c);pd_box(x,y+h-1,w,1,c);pd_box(x,y,1,h,c);pd_box(x+w-1,y,1,h,c);
}
static void pd_polar(int32_t r,uint32_t a,int32_t *x,int32_t *y){
    a&=255u;*x=PD_CX+r*pd_sin[a]/127;*y=PD_CY-r*pd_cos[a]/127;       /* a 0 at the top, clockwise */
}
static uint32_t pd_turn(float t){return (uint32_t)(int32_t)((t-fm_floorf(t))*256.f)&255u;}
/* a knob's box, 60 x 20 at column col, row row of the foot of the screen: what it sets, and a bar; while it is being
 * turned, in white with its value (val: the value's own text, or 0 for 0..100) */
static void pd_knob(int32_t col,int32_t row,const char *name,float v,int lit,uint16_t fill,const char *val){
    int32_t x=col*60,y=200+row*20,w=(int32_t)(v*52.f+.5f);
    if(lit||val){                            /* the value at the right, the name cut to the letters that fit beside it */
        char t[12],nm[12];
        int32_t tw,n=0;
        if(val){while(val[n]&&n<11){t[n]=val[n];n++;}t[n]=0;n=0;}
        else number((int)(v*100+.5f),t);
        tw=text_w(&FONT_XS,t);
        while(name[n]&&n<11){nm[n]=name[n];n++;}
        nm[n]=0;
        while(n>1&&text_w(&FONT_XS,nm)>52-tw-4)nm[--n]=0;
        pd_text(x+4,y+1,nm,lit?C_WHITE:C_GRAY);
        pd_text(x+56-tw,y+1,t,lit?C_WHITE:C_HI);
    }else pd_text(x+4,y+1,name,C_GRAY);
    pd_box(x+4,y+15,52,3,C_LINE);
    if(w>0)pd_box(x+4,y+15,w,3,lit?C_WHITE:fill);
}

static float ring_lo[PM_RINGS][128],ring_hi[PM_RINGS][128],pd_scope[32],g_at[PM_GRAINS],g_env[PM_GRAINS];
static int g_dir[PM_GRAINS],g_slot[PM_GRAINS];

/* what to do next, while it has not been done (0: nothing to say). Two short lines at most */
static const char *pd_hint(const char **second){
    *second=0;
    if(pm_capture_state()!=PM_CAP_IDLE)return 0;
    if(page==PG_MORPH){
        if(!voices())return "TAP KEYS FOR A SOUND";
        if(!norder)return "REC CAPTURES IT AS RING 01";
        return 0;
    }
    if(!norder){*second="FX = SOUND   REC = CAPTURE";return "NO RINGS YET";}
    if(page==PG_SILT&&!running&&!played)return "PLAY STARTS THE GRAINS";
    return 0;
}

/* one ring's circle at radius r: its wave where it has been written, dots where it has not. dense: every step of
 * the circle (the selected ring: brighter and thicker), else every other */
static void pd_ring(int slot,int32_t r,uint16_t c,int dense){
    int32_t x0,y0,x1,y1;
    for(int i=0;i<128;i++){
        uint32_t a=(uint32_t)i*2u;
        int32_t a0,a1;
        if(slot<0||ring_hi[slot][i]<ring_lo[slot][i]){       /* nothing there (yet) */
            if(!(i&1)){pd_polar(r,a,&x0,&y0);pd_box(x0,y0,1,1,C_LINE);}
            continue;
        }
        a0=(int32_t)(ring_lo[slot][i]*9.f);a1=(int32_t)(ring_hi[slot][i]*9.f);
        if(a0<-PD_AMP)a0=-PD_AMP;
        if(a1>PD_AMP)a1=PD_AMP;
        pd_polar(r+a0,a,&x0,&y0);pd_polar(r+a1,a,&x1,&y1);
        pd_line(x0,y0,x1,y1,c);
        if(dense){pd_polar(r+a0,a+1u,&x0,&y0);pd_polar(r+a1,a+1u,&x1,&y1);pd_line(x0,y0,x1,y1,c);}
    }
}
/* MORPH's square, half its side h: the place of the sound among the four corners, the sound itself as a loop round
 * it; with names: the corners named, MOVE's path, the voices under it (MORPH's own page) */
static void pd_square(int32_t h,int names,int on){
    static const char *const CORNER[4]={"FM","MODAL","ADD","FOLD"};   /* top left, top right, bottom left, bottom right */
    uint16_t fldc=on?C_HI:C_DIM,fldg=on?C_GRAY:C_LINE;
    float mx=pm_morph_x(),my=pm_morph_y(),w[4];
    int32_t dx=PD_CX-h+(int32_t)(mx*(float)(2*h)),dy=PD_CY+h-(int32_t)(my*(float)(2*h)),x0=0,y0=0,x1,y1,k;
    int32_t loop=names?16:7;
    w[0]=(1-mx)*my;w[1]=mx*my;w[2]=(1-mx)*(1-my);w[3]=mx*(1-my);
    pd_box(PD_CX-h,PD_CY-h,2*h+1,2*h+1,C_BLACK);             /* (over the rings behind it) */
    pd_frame(PD_CX-h,PD_CY-h,2*h+1,2*h+1,on?C_GRAY:C_LINE);
    if(names){
        int32_t bx=PD_CX-h+(int32_t)(pm_get(M_X)*(float)(2*h)),by=PD_CY+h-(int32_t)(pm_get(M_Y)*(float)(2*h));
        int32_t rad=(int32_t)(pm_get(M_MOVE)*.35f*(float)(2*h));
        pd_line(PD_CX-h,dy,PD_CX+h,dy,C_LINE);pd_line(dx,PD_CY-h,dx,PD_CY+h,C_LINE);       /* its X and its Y */
        for(k=0;k<4;k++)                     /* each corner's name, lit as much as that way of making sound is heard */
            pd_text((k&1)?PD_CX+h-3-text_w(&FONT_XS,CORNER[k]):PD_CX-h+4,(k&2)?PD_CY+h-14:PD_CY-h+1,
                    CORNER[k],w[k]>.4f?fldc:w[k]>.12f?fldg:C_DIM);
        if(rad>1)for(k=0;k<256;k+=16){       /* the path MOVE carries it on */
            x0=bx+rad*pd_sin[k]/127;y0=by-rad*pd_cos[k]/127;
            if(x0>PD_CX-h&&x0<PD_CX+h&&y0>PD_CY-h&&y0<PD_CY+h)pd_box(x0,y0,1,1,fldg);
        }
        {                                    /* the four voices, apart as SPREAD has them */
            float gap=7.f+13.f*pm_get(M_SPREAD);
            for(k=0;k<PM_VOICES;k++){
                int32_t x=PD_CX+(int32_t)(((float)k-1.5f)*gap);
                if(slots[k]>=0)pd_box(x-2,PD_CY+h+5,4,4,fldc);
                else pd_box(x,PD_CY+h+7,1,1,fldg);
            }
        }
    }
    for(k=0;k<=32;k++){                      /* the sound, as a loop round its place */
        int32_t r=loop*3/8+(int32_t)(pd_scope[k&31]*(float)loop);
        if(r<1)r=1;
        if(r>loop-1)r=loop-1;
        x1=dx+r*pd_sin[(k*8)&255]/127;y1=dy-r*pd_cos[(k*8)&255]/127;
        if(x1<PD_CX-h+1)x1=PD_CX-h+1;
        if(x1>PD_CX+h-1)x1=PD_CX+h-1;
        if(y1<PD_CY-h+1)y1=PD_CY-h+1;
        if(y1>PD_CY+h-1)y1=PD_CY+h-1;
        if(k)pd_line(x0,y0,x1,y1,fldc);
        x0=x1;y0=y1;
    }
    pd_box(dx-1,dy-1,3,3,on?C_WHITE:C_GRAY);
}
/* RINGS: the rings there are, oldest first, and where each came from; then what KNOB4 has armed */
static void pd_list(void){
    int sel=pm_ring_selected(),cap=pm_capture_slot(),row=0,i;
    char t[40],*e;
    for(i=0;i<PM_RINGS;i++){
        int32_t y=22+row*21;
        int slot=i<norder?order[i]:-1,id,parent,src,on;
        if(slot<0&&i==norder&&cap>=0){       /* the one being captured */
            e=put(t,"NEW   CAPTURING ");number((int)(pm_capture_progress()*100.f),e);while(*e)e++;put(e,"%");
            pd_frame(4,y,232,19,C_LINE);pd_text(10,y+3,t,C_WHITE);row++;continue;
        }
        if(slot<0){pd_frame(4,y,232,19,C_LINE);pd_text(10,y+3,"--    EMPTY",C_DIM);row++;continue;}
        pm_ring_info(slot,&id,&parent,&src);
        on=slot==sel;
        e=two(id,t);e=put(e,src==PM_SRC_MORPH?"    FROM MORPH":"    FROM RING ");
        if(src!=PM_SRC_MORPH)two(parent,e);
        if(on)pd_box(4,y,232,19,C_LINE);
        pd_frame(4,y,232,19,on?C_HI:C_LINE);
        pd_text(10,y+3,t,on?C_WHITE:C_GRAY);
        if(on)pd_text(236-6-text_w(&FONT_XS,"SELECTED"),y+3,"SELECTED",C_HI);
        row++;
    }
    {
        int32_t y=22+PM_RINGS*21+3;          /* (no line of text across the two strips' join at 120) */
        number(norder,t);e=t;while(*e)e++;e=put(e," OF ");number(PM_RINGS,e);while(*e)e++;put(e," USED");
        pd_text(10,y,t,C_GRAY);
        pd_text(236-6-text_w(&FONT_XS,"SELECT: CHOOSE"),y,"SELECT: CHOOSE",C_GRAY);
        y=122;
        if(arm){
            if(arm==2)put(t,"CLEAR ALL RINGS AND NOTES?");
            else{e=put(t,"DELETE RING ");e=two(ring_id(sel),e);put(e,"?");}
            pd_box(4,y,232,36,C_LINE);pd_frame(4,y,232,36,C_WHITE);
            pd_text(PD_CX-text_w(&FONT_XS,t)/2,y+3,t,C_WHITE);
            pd_text(PD_CX-text_w(&FONT_XS,"SEL = YES   ANY OTHER = NO")/2,y+19,"SEL = YES   ANY OTHER = NO",C_AMB);
        }else{
            pd_text(10,y+3,"KNOB1: CARRY - THE SELECTED RING",C_DIM);
            pd_text(10,y+17,"UNDER THE NEXT CAPTURE",C_DIM);
            pd_text(10,y+35,"KNOB4: DELETE A RING",C_DIM);
        }
    }
}

/* GLO held: what every button does, where the buttons are (two rows of six), then the keys and the knobs */
static void pd_guide(void){
    static const char *const G[12][3]={
        {"FX","MORPH","SOURCE"},{"SEL","RINGS","LIST"},{"ENV","SILT","SHAPE"},{"LFO","SILT","MOTION"},{"EDIT","SILT","GRAINS"},{"GLO","THIS","GUIDE"},
        {"HOME","SILT","HOME"},{"SAVE","-",""},{"ARP","MIX",""},{"SEQ","REC FX",""},{"PLAY","GRAINS","ON/OFF"},{"REC","NEW","RING"}};
    int i;
    pd_text(4,2,"PALINDROME GUIDE",C_HI);
    pd_text(236-text_w(&FONT_XS,"LET GO OF GLO"),2,"LET GO OF GLO",C_GRAY);
    for(i=0;i<12;i++){
        int32_t x=(i%6)*40,y=17+(i/6)*42;
        int used=G[i][1][0]!='-',here=i<6?(i==0&&page==PG_MORPH)||(i==1&&page==PG_RINGS)||(i==2&&page==PG_SHAPE)||(i==3&&page==PG_MOTION)||(i==4&&page==PG_SILT)
                                         :(i==8&&page==PG_MIX)||(i==9&&page==PG_RECFX);
        pd_frame(x+1,y,38,41,here?C_HI:C_LINE);                  /* (the open page's button: framed bright) */
        pd_text(x+4,y+1,G[i][0],used?C_GRAY:C_DIM);
        pd_text(x+4,y+13,G[i][1],used?C_WHITE:C_DIM);
        pd_text(x+4,y+25,G[i][2],C_HI);
    }
    pd_text(4,104,"SELECT: THE RING, ON EVERY PAGE",C_WHITE);
    pd_text(4,122,"KEYS: THE GRAINS' PITCHES",C_WHITE);
    pd_text(4,136,"(ON MORPH'S PAGE: ITS NOTES)",C_GRAY);
    pd_text(4,150,"OCT- OCT+: THE KEYS DOWN / UP",C_GRAY);
    pd_text(4,164,"RINGS PAGE: KNOB4, THEN SEL, DELETES",C_GRAY);
    pd_text(4,180,"KNOBS: NAMED BELOW (MASTER = VOLUME)",C_GRAY);
}

static void pd_scene(void){
    static const char *const TITLE[NPAGES]={"SILT","MORPH SOURCE","RINGS","SILT SHAPE","SILT MOTION","MIX","REC FX"};
    int sel=pm_ring_selected(),cap=pm_capture_slot(),morph=page==PG_MORPH,i;
    int32_t x0,y0,x1,y1,tx;
    char t[40],*e;

    if(guide)pd_guide();
    else{
    if(page==PG_RINGS)pd_list();
    else{
        /* --- the rings: each recording its own circle, the oldest innermost; the selected one bright and dense --- */
        for(i=0;i<PM_RINGS;i++){
            int slot=i<norder?order[i]:(i==norder&&cap>=0)?cap:-1;
            if(slot==cap&&cap>=0)pd_ring(slot,PD_R[i],C_WHITE,1);            /* being captured: filling, in white */
            else if(slot<0)pd_ring(-1,PD_R[i],C_LINE,0);
            else if(slot==sel)pd_ring(slot,PD_R[i],morph?C_GRAY:C_HI,!morph);
            else pd_ring(slot,PD_R[i],morph?C_LINE:C_DIM,0);
        }
        if(cap>=0&&norder<PM_RINGS){             /* the capture's head */
            uint32_t a=pd_turn(pm_capture_progress());
            pd_polar(PD_R[norder]-PD_AMP-2,a,&x0,&y0);pd_polar(PD_R[norder]+PD_AMP+2,a,&x1,&y1);pd_line(x0,y0,x1,y1,C_WHITE);
        }
        if(sel>=0&&place_of(sel)>=0&&!morph){    /* where grains start on the selected ring, and how far they scatter */
            int32_t r=PD_R[place_of(sel)]+PD_AMP+3,half=(int32_t)(pm_get(S_SPRAY)*128.f),k;
            uint32_t a=pd_turn(pm_position());
            for(k=-half;k<=half;k+=3){pd_polar(r,a+(uint32_t)k,&x0,&y0);pd_box(x0,y0,1,1,C_AMB);}
            pd_polar(r-2,a,&x0,&y0);pd_polar(r+3,a,&x1,&y1);pd_line(x0,y0,x1,y1,C_WHITE);
        }
        for(i=0;i<PM_GRAINS;i++)if(g_dir[i]){    /* the grains, each on the ring it plays, a tail where it has been */
            int k=place_of(g_slot[i]);
            int32_t s=g_env[i]>.6f?2:1,r;
            uint32_t a=pd_turn(g_at[i]),back=g_dir[i]>0?253u:3u;
            if(k<0)continue;
            r=PD_R[k];
            pd_polar(r,a+back*2u,&x0,&y0);pd_box(x0,y0,1,1,C_AMB);
            pd_polar(r,a+back,&x0,&y0);pd_box(x0,y0,2,2,C_AMB);
            pd_polar(r,a,&x0,&y0);
            pd_box(x0-s,y0-s,2*s+1,2*s+1,g_env[i]>.4f?(morph?C_GRAY:C_WHITE):C_AMB);
        }
        /* --- MORPH: the injector, small in the middle; on its own page large, the rings behind it --- */
        pd_square(morph?38:20,morph,morph);
    }

    /* --- top: the page; the keys' job; PLAY; the selected ring --- */
    tx=pd_text(4,1,TITLE[page],C_HI);
    if(sel>=0){e=put(t,"RING ");two(ring_id(sel),e);}
    else put(t,"NO RINGS");
    x1=236-text_w(&FONT_XS,t);
    pd_text(x1,1,t,sel>=0?C_WHITE:C_DIM);
    x1-=13;                                      /* PLAY: a triangle while the grains run, a square while not */
    if(running)for(i=0;i<5;i++)pd_box(x1+i,3+i,1,9-2*i,C_WHITE);
    else pd_box(x1,4,7,7,C_DIM);
    if(morph){e=put(t,"KEYS: NOTES");}
    else{
        int n=0;
        e=put(t,"KEYS:");
        for(i=0;i<PM_PITCHES;i++)if(gslots[i]!=PM_NO_PITCH){
            *e++=' ';
            if(gslots[i]>0)*e++='+';
            number(gslots[i],e);
            while(*e)e++;
            n++;
        }
        if(!n)put(e," PITCH");
    }
    {
        int32_t tw=text_w(&FONT_XS,t),mid=(tx+x1)/2;
        if(tw<=x1-tx-8)pd_text(mid-tw/2,1,t,morph?C_GRAY:C_AMB);
    }

    /* --- under the rings: what REC would capture, and how REC stands --- */
    {
        int src=pm_capture_state()!=PM_CAP_IDLE?pm_capture_source():source_now();
        e=put(t,src==PM_SRC_MORPH?"MORPH":"SILT ");
        if(src==PM_SRC_SILT)two(ring_id(sel),e);
        tx=pd_text(4,184,"SOURCE:",C_GRAY);
        pd_text(tx+4,184,t,src==PM_SRC_MORPH?C_HI:C_AMB);
        if(pm_capture_state()!=PM_CAP_IDLE){
            number((int)(pm_capture_progress()*100.f),t);e=t;while(*e)e++;put(e,"%");
            x0=236-text_w(&FONT_XS,t);pd_text(x0,184,t,C_WHITE);
        }else if(norder>=PM_RINGS){x0=236-text_w(&FONT_XS,"FULL");pd_text(x0,184,"FULL",C_AMB);}
        else{x0=236-text_w(&FONT_XS,"READY");pd_text(x0,184,"READY",C_HI);}
        pd_text(x0-4-text_w(&FONT_XS,"REC:"),184,"REC:",C_GRAY);
    }

    /* --- a message, or else a hint, across the middle --- */
    {
        const char *second=0,*first=message_until&&(int32_t)(message_until-plat_ms())>0?message:0;
        uint16_t c=C_WHITE;
        if(!first){first=pd_hint(&second);c=C_GRAY;}
        if(first){
            int32_t tw=text_w(&FONT_XS,first),tw2=second?text_w(&FONT_XS,second):0,w=(tw>tw2?tw:tw2)+12;
            int32_t y=second?140:150,h=second?32:18;                 /* (in the lower strip, whole) */
            pd_box(PD_CX-w/2,y,w,h,C_LINE);
            pd_box(PD_CX-w/2+1,y+1,w-2,h-2,C_BLACK);
            pd_text(PD_CX-tw/2,y+2,first,c);
            if(second)pd_text(PD_CX-tw2/2,y+16,second,c);
        }
    }

    }                                        /* (not the guide) */

    /* --- the knobs by what they set, where they are on the panel: MASTER SELECT | KNOB1 KNOB2 over
     *     PRESETS ALGORITHM | KNOB3 KNOB4 --- */
    {
        int lit=touched>=0&&plat_ms()-touched_at<900u?touched:-1;
        static const uint8_t COL[7]={2,3,2,3,1,1,0},ROW[7]={0,0,1,1,0,1,1};        /* KNOB1..4, SELECT, ALGORITHM, PRESETS */
        uint16_t fill=morph?C_HI:C_AMB;
        pd_knob(0,0,"VOLUME",(float)plat_master()*(1.f/4096.f),0,C_GRAY,0);          /* MASTER */
        for(i=0;i<7;i++){
            int id=pidx(page,i);
            if(i==4){                            /* SELECT: the ring, everywhere */
                int at=place_of(sel);
                if(at>=0){two(ring_id(sel),t);pd_knob(COL[i],ROW[i],"RING",(float)(at+1)/(float)norder,lit==i,C_WHITE,t);}
                else pd_knob(COL[i],ROW[i],"RING",0,lit==i,C_WHITE,"--");
            }else if(page==PG_RINGS&&i==3)
                pd_knob(COL[i],ROW[i],"DEL",(float)arm*.5f,lit==i,C_WHITE,arm==2?"ALL":arm?"ONE":"NO");
            else if(id>=0)pd_knob(COL[i],ROW[i],labels[id],pm_get(id),lit==i,fill,0);
        }
        pd_box(119,201,2,38,C_DIM);                                                  /* left knobs | right knobs */
    }
}

static void draw(void){
    int i;
    rings_now();
    for(i=0;i<PM_RINGS;i++)pm_ring_wave(i,ring_lo[i],ring_hi[i],128);
    pm_scope(pd_scope,32);
    for(i=0;i<PM_GRAINS;i++)g_dir[i]=pm_grain(i,&g_at[i],&g_env[i],&g_slot[i]);
    for(i=0;i<2;i++){
        pd_y0=i*120;
        cv_begin(240,120,C_BLACK);
        pd_scene();
        cv_blit(0,(uint32_t)pd_y0);
    }
}
void app_ui_init(void){
    lcd_fill(0,0,240,240,C_BLACK);
    app_say(APP_NAME,APP_VERSION);
    message_until=plat_ms()+2500u;
    draw();
}
void app_frame(void){app_input_only();if((++frames&3u)==0)draw();}
#ifdef PLAT_HOST                         /* the simulator's checks (strcmp: the FM-1's toolchain has no <string.h>) */
int app_expect(const char *what,int *got){
    if(!strcmp(what,"page"))*got=page;
    else if(!strcmp(what,"voices"))*got=voices();
    else if(!strcmp(what,"rings"))*got=pm_ring_count();
    else if(!strcmp(what,"ring"))*got=ring_id(pm_ring_selected());             /* the selected ring's number, 0 none */
    else if(!strcmp(what,"parent")){int id=0,parent=0;pm_ring_info(pm_ring_selected(),&id,&parent,0);*got=parent;}
    else if(!strcmp(what,"capturing"))*got=pm_capture_state()!=PM_CAP_IDLE;
    else if(!strcmp(what,"source"))*got=source_now();
    else if(!strcmp(what,"grains"))*got=running;
    else if(!strcmp(what,"sounding"))*got=pm_active_grains()>0;
    else if(!strcmp(what,"gkeys"))*got=gkeys();
    else if(!strcmp(what,"arm"))*got=arm;
    else if(!strcmp(what,"guide"))*got=guide;
    else if(!strcmp(what,"size"))*got=(int)(pm_get(S_SIZE)*100+.5f);
    else if(!strcmp(what,"pitch"))*got=(int)(pm_get(S_PITCH)*100+.5f);
    else if(!strcmp(what,"noise"))*got=(int)(pm_get(R_NOISE)*100+.5f);
    else if(!strcmp(what,"cluster"))*got=(int)(pm_get(S_CLUSTER)*100+.5f);
    else return 0;
    return 1;
}
#endif
