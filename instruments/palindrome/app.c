/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME's panel and screen.
 *
 * The screen is a picture of the engine with just enough words to find your way:
 *
 *   the ring     SILT's memory, drawn as the loop it is. What was recorded lies where it was written; the white
 *                tick is the write head going round, writing over it. The mark outside the ring is where grains
 *                start (POSITION), the dotted arc beside it how far they scatter (SPRAY). Each grain is a spot
 *                riding the ring: outside it playing forward, inside it playing backward.
 *   the square   MORPH's vector space, in the middle of the ring because its sound is what the ring records. The
 *                four corners are the four ways of making sound, each named and lit as much as it is heard. The
 *                white spot is where the sound is now; the loop around it is the sound itself; the dotted circle
 *                is the path MOVE carries it on. The pips under the square are the four voices.
 *   the corners  what the buttons do and how they stand: FX: MORPH and EDIT: SILT at the top (whose knobs these
 *                are, the one in use lit; SEL held, SEL: SHARED; REC held, REC: RECORDING), REC and how the memory
 *                is written at the bottom left, PLAY: GRAINS at the bottom right, lit while they run.
 *   REC          a tap steps how the memory is written (LIVE, FREEZE, OVERDUB, RESAMPLE); held, the knobs are the
 *                recording's own: NOISE, COLOR, CRUSH, GATE, DROP, DAMP, FEEDBACK. The head's tick dims while GATE
 *                or DROP has it lifted, and specks trail it as NOISE goes in.
 *   EDIT         a tap gives the knobs to SILT; held, they are the rest of SILT's (EDIT: SILT MORE): TIMING, VARY,
 *                SCATTER, DETUNE, STEREO, CLUSTER, SCAN. In SILT the keys are the grains' pitches: up to four held
 *                (a tap holds a key, another lets it go), the grains taking them in turn; the held pitches are
 *                written under the square, in semitones from the recorded pitch.
 *   the boxes    the seven knobs by what they set now, placed as they are on the panel (MASTER, SELECT, PRESETS,
 *                ALGORITHM left of the screen; KNOB1..4 right of it), each with a bar; a turned knob's value shows
 *                in its box.
 *   a hint       until you have done it once: what to do next, in the middle of the screen. */
#include "palindrome.h"
#include "fastmath.h"

static int mode, octave, recording, running, slots[PM_VOICES], replace_at;
static uint32_t old_keys, old_btn, frames, home_t;
static float gain_now;
static float audio_l[APP_BLOCK],audio_r[APP_BLOCK];
static char message[24];
static uint32_t message_until;
static int touched=-1;                   /* the knob turned last (0..6): its box white, its value in it, for a moment */
static uint32_t touched_at;
static int learned;                      /* the hints no longer needed: bit 0 a key played, 1 PLAY pressed, 2 REC
                                          * tapped, 3 REC held and a knob turned, 4 a key played in SILT, 5 EDIT
                                          * held and a knob turned */
static int gslots[PM_PITCHES],greplace_at;   /* the keys held for the grains: semitones, PM_NO_PITCH none */
static const int enc[7]={EN_K1,EN_K2,EN_K3,EN_K4,EN_SELECT,EN_ALGO,EN_PRESET};
static const char *const labels[PM_PARAMS]={
 "X","Y","COMPLEX","TONE","SPREAD","MOVE","EXCITE",
 "POSITION","SIZE","DENSITY","SPRAY","PITCH","REVERSE","FEEDBACK",
 "MIX","FADE","SCAN","SHAPE","STEREO","DAMP","LEVEL",
 "NOISE","COLOR","CRUSH","GATE","DROP",
 "TIMING","VARY","SCATTER","DETUNE","CLUSTER"};
/* the control on knob j (KNOB1..4, SELECT, ALGORITHM, PRESETS) of a page, or -1 (that knob sets nothing there):
 * 0 MORPH, 1 SILT, 2 both halves' (SEL held), 3 the recording's (REC held), 4 the rest of SILT's (EDIT held) */
#define PG_NONE 255
static int pidx(int page,int j){
    static const uint8_t PAGE[5][7]={
        {M_X,M_Y,M_COMPLEX,M_TONE,M_SPREAD,M_MOVE,M_EXCITE},
        {S_POS,S_SIZE,S_DENSITY,S_SPRAY,S_PITCH,S_REVERSE,S_SHAPE},
        {G_MIX,G_FADE,PG_NONE,PG_NONE,PG_NONE,PG_NONE,G_LEVEL},
        {R_NOISE,R_COLOR,R_CRUSH,R_GATE,R_DROP,G_DAMP,S_FEEDBACK},
        {S_TIMING,S_VARY,S_SCATTER,S_DETUNE,G_STEREO,S_CLUSTER,G_SCAN}};
    return PAGE[page][j]==PG_NONE?-1:PAGE[page][j];
}
/* the knobs' page now: a held REC, SEL or EDIT before the half chosen */
static int page_of(uint32_t btn){
    return (btn&(1u<<B_REC))?3:(btn&(1u<<B_SEL))?2:(btn&(1u<<B_EDIT))?4:mode;
}
static int rec_turned;                   /* a knob turned while REC was held: its release does not step the mode */
static const char *const rec_names[]={"LIVE","FREEZE","OVERDUB","RESAMPLE"};

static void number(int v,char *s){
    char t[12];int n=0;unsigned u;
    if(v<0){*s++='-';u=(unsigned)(-v);}else u=(unsigned)v;
    do{t[n++]=(char)('0'+u%10u);u/=10u;}while(u);
    while(n)*s++=t[--n];
    *s=0;
}
void app_say(const char *a,const char *b){
    uint32_t i=0;
    for(;a&&a[i]&&i<23;i++)message[i]=a[i];
    if(b&&*b&&i<22){message[i++]=' ';while(*b&&i<23)message[i++]=*b++;}
    message[i]=0;message_until=plat_ms()+1200u;
}
static int voices(void){int n=0;for(int j=0;j<PM_VOICES;j++)n+=slots[j]>=0;return n;}
static int gkeys(void){int n=0;for(int j=0;j<PM_PITCHES;j++)n+=gslots[j]!=PM_NO_PITCH;return n;}
static void all_off(void){
    for(int j=0;j<PM_VOICES;j++){slots[j]=-1;pm_note(j,-1);}
    for(int j=0;j<PM_PITCHES;j++){gslots[j]=PM_NO_PITCH;pm_grain_pitch(j,PM_NO_PITCH);}
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
static void latch(int note){
    int empty=-1;
    for(int j=0;j<PM_VOICES;j++){
        if(slots[j]==note){slots[j]=-1;pm_note(j,-1);return;}
        if(slots[j]<0&&empty<0)empty=j;
    }
    if(empty<0)empty=(replace_at++)%PM_VOICES;
    slots[empty]=note;pm_note(empty,note);
}

/* sine and cosine of 256 steps round the circle, x 127: the ring's geometry, made once */
static int8_t pd_sin[256],pd_cos[256];
void app_init(void){
    pm_init();mode=0;octave=-1;recording=PM_LIVE;running=0;replace_at=greplace_at=0;
    old_keys=old_btn=frames=message_until=home_t=0;gain_now=0;touched=-1;learned=0;
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
void app_input_only(void){
    uint32_t keys=plat_keys(),btn=plat_buttons(),newb=btn&~old_btn,newk=keys&~old_keys,now=plat_ms();
    /* (the screen's corners say how these four stand: no message needed) */
    if(newb&(1u<<B_FX))mode=0;
    if(newb&(1u<<B_EDIT))mode=1;
    /* REC: let go without a knob turned, the next way of writing the memory; held, its knobs (below) */
    if(newb&(1u<<B_REC))rec_turned=0;
    if((old_btn&(1u<<B_REC))&&!(btn&(1u<<B_REC))&&!rec_turned){
        recording=(recording+1)&3;pm_record(recording);learned|=4;
    }
    if(newb&(1u<<B_PLAY)){running=!running;pm_grains(running);learned|=2;}
    /* HOME held a second: every voice and the grains off. A tap only says so (and the version): no accident */
    if(newb&(1u<<B_HOME))home_t=now?now:1u;
    if(home_t){
        if(!(btn&(1u<<B_HOME))){home_t=0;app_say("V" APP_VERSION,"HOLD HOME: CLEAR");}
        else if(now-home_t>=1000u){home_t=0;all_off();running=0;pm_grains(0);app_say("CLEARED","");}
    }
    if(newb&(1u<<B_SAVE))app_say("NOTHING TO SAVE YET","");
    if((newb&(1u<<B_OCTDN))&&octave>-3)octave--;
    if((newb&(1u<<B_OCTUP))&&octave<2)octave++;
    for(int k=0;k<NKEYS;k++)if(newk&(1u<<k)){
        learned|=1;
        if(!mode)latch(53+12*octave+k);
        else{learned|=16;glatch(53+12*(octave+1)+k-60);}      /* (the C key, as the keys start: the recorded pitch) */
    }
    int page=page_of(btn);
    for(int j=0;j<7;j++){
        int d=plat_enc(enc[j]),id=pidx(page,j);
        if(!d||id<0)continue;
        pm_set(id,fm_clampf(pm_get(id)+(float)d/64.f,0,1));
        touched=j;touched_at=now;        /* (its box shows the value for a moment) */
        if(page==3){rec_turned=1;learned|=8;}
        if(page==4)learned|=32;
    }
    old_keys=keys;old_btn=btn;
    uint32_t lit=(1u<<(mode?B_EDIT:B_FX))|(running?LED_PLAY_GREEN:0)|
        (recording!=PM_LIVE?(1u<<B_REC):0)|(btn&(1u<<B_REC))|(btn&(1u<<B_SEL))|(btn&(1u<<B_HOME));
    uint32_t kl=0;
    if(!mode)for(int j=0;j<PM_VOICES;j++){
        int k=slots[j]-(53+12*octave);
        if(k>=0&&k<NKEYS)kl|=1u<<k;
    }
    else for(int j=0;j<PM_PITCHES;j++){
        int k=gslots[j]+60-(53+12*(octave+1));
        if(gslots[j]!=PM_NO_PITCH&&k>=0&&k<NKEYS)kl|=1u<<k;
    }
    plat_lights(lit,kl,(1u<<B_FX)|(1u<<B_EDIT)|(1u<<B_REC)|(1u<<B_PLAY)|(1u<<B_HOME)|(1u<<B_SEL),0,1);
}

/* ------------------------------------------------------------- the picture --- */
#define PD_CX 120
#define PD_CY 100
#define PD_RB 74                         /* the memory ring's radius */
#define PD_RAMP 10                       /* how far the recorded wave reaches in and out of it */
#define PD_FH 38                         /* half the side of MORPH's square */
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
static void pd_polar(int32_t r,uint32_t a,int32_t *x,int32_t *y){
    a&=255u;*x=PD_CX+r*pd_sin[a]/127;*y=PD_CY-r*pd_cos[a]/127;       /* a 0 at the top, clockwise */
}
static uint32_t pd_turn(float t){return (uint32_t)(int32_t)((t-fm_floorf(t))*256.f)&255u;}
/* a button and what it does or how it stands, in a corner: "REC: FREEZE". right: the text ends at x */
static void pd_legend(int32_t x,int32_t y,int right,const char *button,const char *what,uint16_t c){
    if(right)x-=text_w(&FONT_XS,button)+4+text_w(&FONT_XS,what);
    x=pd_text(x,y,button,C_GRAY);
    pd_text(x+4,y,what,c);
}
/* a knob's box, 60 x 20 at column col, row row of the foot of the screen: what it sets, and a bar; while it is being
 * turned, in white with its value */
static void pd_knob(int32_t col,int32_t row,const char *name,float v,int lit,uint16_t fill){
    int32_t x=col*60,y=200+row*20,w=(int32_t)(v*52.f+.5f);
    if(lit){                                 /* the value at the right, the name cut to the letters that fit beside it */
        char t[12],nm[12];
        int32_t tw,n=0;
        number((int)(v*100+.5f),t);
        tw=text_w(&FONT_XS,t);
        while(name[n]&&n<11){nm[n]=name[n];n++;}
        nm[n]=0;
        while(n>1&&text_w(&FONT_XS,nm)>52-tw-4)nm[--n]=0;
        pd_text(x+4,y+1,nm,C_WHITE);
        pd_text(x+56-tw,y+1,t,C_WHITE);
    }else pd_text(x+4,y+1,name,C_GRAY);
    pd_box(x+4,y+15,52,3,C_LINE);
    if(w>0)pd_box(x+4,y+15,w,3,lit?C_WHITE:fill);
}

static float ring_lo[128],ring_hi[128],pd_scope[32],g_at[PM_GRAINS],g_env[PM_GRAINS];
static int g_dir[PM_GRAINS];

/* what to do next, until it has been done once (0: nothing to say) */
static const char *pd_hint(void){
    if(!(learned&1)&&!mode)return "TAP KEYS TO PLAY NOTES";
    if(mode&&pm_fill()<.5f&&!voices())return "NO SOUND YET: TAP FX";
    if(mode&&!running&&!(learned&2))return "PLAY STARTS THE GRAINS";
    if(mode&&running&&recording==PM_LIVE&&!(learned&4))return "REC FREEZES THE LOOP";
    if(mode&&running&&(learned&4)&&!(learned&16))return "KEYS PLAY THE GRAINS AS NOTES";
    if(mode&&running&&(learned&16)&&!(learned&32))return "HOLD EDIT: MORE GRAIN KNOBS";
    if(mode&&running&&(learned&32)&&!(learned&8))return "HOLD REC: NOISE, GATE";
    return 0;
}

static void pd_scene(void){
    int page=page_of(old_btn),rec=page==3,sel=page==2,more=page==4;
    int ring_on=mode||sel||rec,field_on=(!mode||sel)&&!rec,i;
    uint16_t ringc=ring_on?C_HI:C_DIM,ringa=ring_on?C_AMB:C_DIM;
    uint16_t fldc=field_on?C_HI:C_DIM,fldg=field_on?C_GRAY:C_LINE;
    int32_t x0,y0,x1,y1;
    float fill=pm_fill(),head=pm_head(),mx=pm_morph_x(),my=pm_morph_y();

    /* --- the ring: the memory where it lies, the head writing over it --- */
    for(i=0;i<128;i++){
        uint32_t a=(uint32_t)i*2u;
        int32_t a0,a1;
        if(ring_hi[i]<ring_lo[i]){pd_polar(PD_RB,a,&x0,&y0);pd_box(x0,y0,1,1,C_LINE);continue;}   /* not written yet */
        a0=(int32_t)(ring_lo[i]*26.f);a1=(int32_t)(ring_hi[i]*26.f);
        if(a0<-PD_RAMP)a0=-PD_RAMP;
        if(a1>PD_RAMP)a1=PD_RAMP;
        pd_polar(PD_RB+a0,a,&x0,&y0);pd_polar(PD_RB+a1,a,&x1,&y1);
        pd_line(x0,y0,x1,y1,ringc);
    }
    {
        uint32_t a=pd_turn(head);
        if(recording==PM_FREEZE){            /* stopped: two short marks, the memory between them safe */
            pd_polar(PD_RB+PD_RAMP+2,a,&x0,&y0);pd_polar(PD_RB+PD_RAMP+7,a,&x1,&y1);pd_line(x0,y0,x1,y1,C_AMB);
            pd_polar(PD_RB-PD_RAMP-2,a,&x0,&y0);pd_polar(PD_RB-PD_RAMP-7,a,&x1,&y1);pd_line(x0,y0,x1,y1,C_AMB);
        }else{
            /* white while it writes; dim while GATE or DROP has it lifted. NOISE going in: specks trailing it */
            static uint32_t speck=0x2f6e2b1u;
            int writing=pm_writing()>.5f,k,specks=writing?(int)(pm_get(R_NOISE)*10.f+.5f):0;
            uint16_t hc=writing?C_WHITE:C_DIM;
            for(k=0;k<specks;k++){
                speck=speck*1664525u+1013904223u;
                pd_polar(PD_RB-PD_RAMP+(int32_t)((speck>>20)%(2u*PD_RAMP+1u)),a-1u-((speck>>8)%14u),&x0,&y0);
                pd_box(x0,y0,1,1,C_GRAY);
            }
            pd_polar(PD_RB-PD_RAMP-3,a,&x0,&y0);pd_polar(PD_RB+PD_RAMP+3,a,&x1,&y1);pd_line(x0,y0,x1,y1,hc);
            if(recording==PM_OVERDUB){       /* writing on top: a second tick */
                pd_polar(PD_RB-PD_RAMP-3,a+3u,&x0,&y0);pd_polar(PD_RB+PD_RAMP+3,a+3u,&x1,&y1);pd_line(x0,y0,x1,y1,hc);
            }
            if(recording==PM_RESAMPLE){      /* writing the grains back in: a block on the inside */
                pd_polar(PD_RB-PD_RAMP-6,a,&x0,&y0);pd_box(x0-2,y0-2,5,5,hc);
            }
        }
    }
    {                                        /* where grains start, and how far they scatter */
        uint32_t a=pd_turn(head-pm_position()*fill);
        int32_t half=(int32_t)(pm_get(S_SPRAY)*.5f*fill*256.f),k;
        for(k=-half;k<=half;k+=3){pd_polar(93,a+(uint32_t)k,&x0,&y0);pd_box(x0,y0,1,1,ringa);}
        pd_polar(90,a,&x0,&y0);pd_polar(96,a,&x1,&y1);pd_line(x0,y0,x1,y1,ring_on?C_WHITE:C_GRAY);
    }
    for(i=0;i<PM_GRAINS;i++)if(g_dir[i]){    /* the grains: forward outside the ring, backward inside it */
        int32_t s=g_env[i]>.6f?2:1,r=g_dir[i]>0?87:60;
        uint32_t a=pd_turn(g_at[i]),back=g_dir[i]>0?253u:3u;       /* (its tail: where it has just been) */
        pd_polar(r,a+back*2u,&x0,&y0);pd_box(x0,y0,1,1,ringa);
        pd_polar(r,a+back,&x0,&y0);pd_box(x0,y0,2,2,ringa);
        pd_polar(r,a,&x0,&y0);
        pd_box(x0-s,y0-s,2*s+1,2*s+1,g_env[i]>.4f?(ring_on?C_WHITE:C_GRAY):ringa);
    }

    /* --- the square: MORPH's four corners, the sound's place among them, the sound itself --- */
    pd_line(PD_CX-PD_FH,PD_CY-PD_FH,PD_CX+PD_FH,PD_CY-PD_FH,C_LINE);pd_line(PD_CX-PD_FH,PD_CY+PD_FH,PD_CX+PD_FH,PD_CY+PD_FH,C_LINE);
    pd_line(PD_CX-PD_FH,PD_CY-PD_FH,PD_CX-PD_FH,PD_CY+PD_FH,C_LINE);pd_line(PD_CX+PD_FH,PD_CY-PD_FH,PD_CX+PD_FH,PD_CY+PD_FH,C_LINE);
    {
        static const char *const CORNER[4]={"FM","MODAL","ADD","FOLD"};   /* top left, top right, bottom left, bottom right */
        float w[4];
        int32_t dx=PD_CX-PD_FH+(int32_t)(mx*(2*PD_FH)),dy=PD_CY+PD_FH-(int32_t)(my*(2*PD_FH));
        int32_t bx=PD_CX-PD_FH+(int32_t)(pm_get(M_X)*(2*PD_FH)),by=PD_CY+PD_FH-(int32_t)(pm_get(M_Y)*(2*PD_FH));
        int32_t rad=(int32_t)(pm_get(M_MOVE)*.35f*(2*PD_FH)),k;
        w[0]=(1-mx)*my;w[1]=mx*my;w[2]=(1-mx)*(1-my);w[3]=mx*(1-my);
        pd_line(PD_CX-PD_FH,dy,PD_CX+PD_FH,dy,C_LINE);pd_line(dx,PD_CY-PD_FH,dx,PD_CY+PD_FH,C_LINE);       /* its X and its Y */
        for(k=0;k<4;k++)                     /* each corner's name, lit as much as that way of making sound is heard */
            pd_text((k&1)?PD_CX+PD_FH-3-text_w(&FONT_XS,CORNER[k]):PD_CX-PD_FH+4,(k&2)?PD_CY+PD_FH-14:PD_CY-PD_FH+1,
                    CORNER[k],w[k]>.4f?fldc:w[k]>.12f?fldg:field_on?C_DIM:C_LINE);
        if(rad>1)for(k=0;k<256;k+=16){       /* the path MOVE carries it on */
            x0=bx+rad*pd_sin[k]/127;y0=by-rad*pd_cos[k]/127;
            if(x0>PD_CX-PD_FH&&x0<PD_CX+PD_FH&&y0>PD_CY-PD_FH&&y0<PD_CY+PD_FH)pd_box(x0,y0,1,1,fldg);
        }
        for(k=0;k<=32;k++){                  /* the sound, as a loop round its place */
            int32_t r=6+(int32_t)(pd_scope[k&31]*16.f);
            if(r<1)r=1;
            if(r>15)r=15;
            x1=dx+r*pd_sin[(k*8)&255]/127;y1=dy-r*pd_cos[(k*8)&255]/127;
            if(k)pd_line(x0,y0,x1,y1,fldc);
            x0=x1;y0=y1;
        }
        pd_box(dx-1,dy-1,3,3,C_WHITE);
    }
    {                                        /* the four voices, apart as SPREAD has them */
        float gap=7.f+13.f*pm_get(M_SPREAD);
        for(i=0;i<PM_VOICES;i++){
            int32_t x=PD_CX+(int32_t)(((float)i-1.5f)*gap);
            if(slots[i]>=0)pd_box(x-2,PD_CY+PD_FH+5,4,4,fldc);
            else pd_box(x,PD_CY+PD_FH+7,1,1,fldg);
        }
    }

    {                                        /* the keys held for the grains, in semitones from the recorded pitch */
        char t[28];
        int n=0;
        for(i=0;i<PM_PITCHES;i++)if(gslots[i]!=PM_NO_PITCH){
            if(n)t[n++]=' ';
            if(gslots[i]>0)t[n++]='+';
            number(gslots[i],t+n);
            while(t[n])n++;
        }
        t[n]=0;
        if(n)pd_text(PD_CX-text_w(&FONT_XS,t)/2,PD_CY+PD_FH+10,t,ring_on?C_AMB:C_DIM);
    }

    /* --- the corners: the buttons, what they do and how they stand (lit: in use, or running) --- */
    if(rec)pd_legend(4,1,0,"REC:","RECORDING",C_HI);
    else if(sel)pd_legend(4,1,0,"SEL:","SHARED",C_HI);
    else pd_legend(4,1,0,"FX:","MORPH",!mode?C_HI:C_DIM);
    pd_legend(236,1,1,"EDIT:",more?"SILT MORE":"SILT",mode&&!sel&&!rec?C_HI:C_DIM);
    pd_legend(4,184,0,"REC:",rec_names[recording],recording==PM_LIVE?C_HI:recording==PM_FREEZE?C_AMB:C_WHITE);
    pd_legend(236,184,1,"PLAY:","GRAINS",running?C_WHITE:C_DIM);

    /* --- a message (the version, HOME, SAVE), or else a hint, across the middle --- */
    {
        const char *t=message_until&&(int32_t)(message_until-plat_ms())>0?message:0;
        uint16_t c=C_WHITE;
        if(!t){t=pd_hint();c=C_GRAY;}
        if(t){
            int32_t tw=text_w(&FONT_XS,t);
            pd_box(PD_CX-tw/2-6,91,tw+12,18,C_LINE);
            pd_box(PD_CX-tw/2-5,92,tw+10,16,C_BLACK);
            pd_text(PD_CX-tw/2,93,t,c);
        }
    }

    /* --- the knobs by what they set, where they are on the panel: MASTER SELECT | KNOB1 KNOB2 over
     *     PRESETS ALGORITHM | KNOB3 KNOB4 --- */
    {
        int lit=touched>=0&&plat_ms()-touched_at<900u?touched:-1;
        static const uint8_t COL[7]={2,3,2,3,1,1,0},ROW[7]={0,0,1,1,0,1,1};        /* KNOB1..4, SELECT, ALGORITHM, PRESETS */
        uint16_t fill=rec?C_WHITE:sel?C_HI:C_AMB;
        pd_knob(0,0,"VOLUME",(float)plat_master()*(1.f/4096.f),0,C_GRAY);            /* MASTER */
        for(i=0;i<7;i++)if(pidx(page,i)>=0)
            pd_knob(COL[i],ROW[i],labels[pidx(page,i)],pm_get(pidx(page,i)),lit==i,fill);
        pd_box(119,201,2,38,C_DIM);                                                  /* left knobs | right knobs */
    }
}

static void draw(void){
    int i;
    pm_ring(ring_lo,ring_hi,128);
    pm_scope(pd_scope,32);
    for(i=0;i<PM_GRAINS;i++)g_dir[i]=pm_grain(i,&g_at[i],&g_env[i]);
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
    if(!strcmp(what,"mode"))*got=mode;
    else if(!strcmp(what,"voices"))*got=voices();
    else if(!strcmp(what,"recording"))*got=recording;
    else if(!strcmp(what,"grains"))*got=running;
    else if(!strcmp(what,"filled"))*got=(int)(pm_fill()*100);
    else if(!strcmp(what,"noise"))*got=(int)(pm_get(R_NOISE)*100+.5f);
    else if(!strcmp(what,"gkeys"))*got=gkeys();
    else if(!strcmp(what,"cluster"))*got=(int)(pm_get(S_CLUSTER)*100+.5f);
    else return 0;
    return 1;
}
#endif
