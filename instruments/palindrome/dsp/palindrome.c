/* SPDX-License-Identifier: MPL-2.0 */
/* Shared device / host / WebAssembly audio engine. No allocation in render.
 *
 * Who touches what: the render (the audio interrupt on the FM-1) writes a capture's samples, the grains and the
 * voices; the UI (pm_ring_*, pm_capture, pm_service) owns the rings' records, which ring is selected and the
 * controls' targets. A ring's samples are written only while it is being captured, when nothing may read it; once
 * pm_service has made it a ring they are never written again. */
#include "palindrome.h"
#include "fastmath.h"
#include "plat.h"                        /* PLAT_POOL; its clamp is the integer one: floats use fclamp */
#define MASK (PM_BUFFER-1u)
#define INV_RATE (1.0f/44100.0f)
#ifndef GEN_LEVEL
#define GEN_LEVEL .2f                     /* how loud (rms) a generation is made */
#endif
#ifndef GEN_GAIN
#define GEN_GAIN 1.f                      /* a generation's grains against the ring kept under them (set by measurement) */
#endif
#define SEAM 256u                        /* a capture's ends are eased over this many samples: the loop closes clean */
static int16_t memory[PM_RINGS][PM_BUFFER] PLAT_POOL;
typedef struct {
    float phase, m1, m2, env, modal, filt, inc;
    float life;                          /* 1 as it is struck, dying away from there (DECAY); held at 1 at its top */
    int note;
} Voice;
typedef struct {
    float pos, speed, age, step, pan, amp, atk, up, down, gain;
    const int16_t *mem;                  /* the ring it reads: its own to the end, whatever is selected meanwhile */
    int slot, active, dying;
} Grain;
/* the SILT controls each ring keeps for itself */
static const uint8_t RING_PAR[]={S_POS,S_SIZE,S_DENSITY,S_SPRAY,S_PITCH,S_REVERSE,S_SHAPE,
                                 S_TIMING,S_VARY,S_SCATTER,S_DETUNE,S_CLUSTER,G_STEREO,G_SCAN,S_LOOP};
#define NPAR ((int)sizeof RING_PAR)
typedef struct {
    volatile int ready;                  /* a finished ring: its samples are final */
    int id, parent, source;
    float par[sizeof RING_PAR];
} Ring;
static Ring ring[PM_RINGS];
static volatile int selected;            /* the slot the grains start in; -1 none */
static int next_id;
/* a capture */
static volatile int cap_state,cap_event,cap_abort;
static volatile uint32_t cap_i;          /* samples written so far */
static int cap_slot,cap_source,cap_parent,cap_carry;   /* (cap_carry: the slot copied under it, -1 none) */
static uint32_t norm_i;
static float cap_peak,cap_gain,cap_sq;   /* (cap_sq: the sum of its samples' squares, for a generation's level) */
static float loop_phase,cap_dc;
static volatile uint32_t kill_req[PM_RINGS];
static uint32_t kill_seen[PM_RINGS];

static Voice voice[PM_VOICES];
static Grain grain[PM_GRAINS];
static volatile int requested[PM_VOICES];
static volatile uint32_t struck[PM_VOICES];   /* counts each pm_note with a note: a strike, even of the same note */
static uint32_t struck_seen[PM_VOICES];
static volatile int monitor;
static int dropped_id;
static volatile int grains_on;
static volatile uint32_t burst_request;
static volatile int gpitch[PM_PITCHES];        /* the keys held for the grains, semitones (PM_NO_PITCH: none) */
static volatile uint32_t strike_request[PM_PITCHES];
static uint32_t strike_seen[PM_PITCHES];
static int turn;                         /* whose turn among the held pitches */
static float next_at;                    /* the next grain's time, in even pulses (1: on the pulse) */
static uint32_t burst_seen, half, rng;
static volatile uint32_t scope_wr;
static float half_sample, clock_phase, scan_phase, launch, dry_gain, wet_gain;
static float capture_lp, dc_l, dc_r, prev_l, prev_r, scope[64];
static volatile float target[PM_PARAMS];
static float p[PM_PARAMS];
static float mx,my;
static volatile float position_now;
static float gate_env,noise_lp,write_amt;   /* the gate's ear on the sound; the hiss's low part; the gate and
                                          * dropouts' hand on the level (eased over ~2 ms) */
static int dropping;
static const float defaults[PM_PARAMS]={
 .35f,.65f,.35f,.5f,.4f,0,.55f,
 .0f,.55f,.42f,.08f,.5f,0,0,             /* (grains long and close enough to run together, hardly scattered: the
                                          * ring is heard as the loop it is. CARRY 0: a capture is the source alone) */
 1.f,.20f,0,.5f,.65f,.35f,.75f,          /* (MIX: while grains sound they are what is heard; SHAPE: a bell) */
 0,.5f,0,0,0,                            /* a clean recording: no hiss, no crush, no gate, no dropouts */
 0,0,0,0,0,                              /* grains on an even pulse, all one length and pitch, one at a time */
 .7f,                                    /* DECAY: a struck note, gone in a few seconds */
 .3f,                                    /* DRIFT: a generation every few seconds, about half of it kept */
 .5f};                                   /* LOOP: the play point round the ring at the speed it was recorded */
static float fclamp(float x,float a,float b){return x<a?a:x>b?b:x;}
static float wrap(float x){return x-fm_floorf(x);}
static float rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (float)(rng>>8)*(1.0f/16777216.0f);}
static float soft(float x){return x/(1.0f+(x<0?-x:x));}
void pm_set(int id,float v){if(id>=0&&id<PM_PARAMS&&v>=0&&v<=1)target[id]=v;}
float pm_get(int id){return id>=0&&id<PM_PARAMS?target[id]:0;}
void pm_note(int slot,int note){
    if(slot<0||slot>=PM_VOICES)return;
    requested[slot]=note>=0&&note<=127?note:-1;
    if(note>=0&&note<=127)struck[slot]++;
}
int pm_voice_sounding(int slot){
    if(slot<0||slot>=PM_VOICES||requested[slot]<0)return 0;
    return struck[slot]!=struck_seen[slot]||voice[slot].life>.003f;      /* (just struck, or not yet died away) */
}
void pm_monitor(int on){monitor=on!=0;}
int pm_capture_dropped(void){return dropped_id;}
void pm_grains(int on){if(on&&!grains_on)burst_request++;grains_on=on!=0;}
void pm_burst(void){burst_request++;}
void pm_grain_pitch(int slot,int semis){
    if(slot>=0&&slot<PM_PITCHES)gpitch[slot]=semis>=-48&&semis<=48?semis:PM_NO_PITCH;
}
void pm_strike(int slot){if(slot>=0&&slot<PM_PITCHES)strike_request[slot]++;}
void pm_panic(void){
    for(int i=0;i<PM_VOICES;i++){requested[i]=-1;voice[i].env=0;voice[i].note=-1;voice[i].life=0;struck_seen[i]=struck[i];}
    for(int i=0;i<PM_GRAINS;i++)grain[i].active=0;
    for(int i=0;i<PM_PITCHES;i++){gpitch[i]=PM_NO_PITCH;strike_seen[i]=strike_request[i];}
    grains_on=0;burst_seen=burst_request; wet_gain=0;dry_gain=1;
    capture_lp=dc_l=dc_r=prev_l=prev_r=0;
    cap_state=PM_CAP_IDLE;cap_event=0;cap_abort=0;      /* a running capture is dropped; the rings stay */
}
void pm_init(void){
    /* (plain loops: the FM-1's toolchain has no <string.h>) */
    static const Voice voice0;static const Grain grain0;
    for(int r=0;r<PM_RINGS;r++){
        for(uint32_t i=0;i<PM_BUFFER;i++)memory[r][i]=0;
        ring[r].ready=0;ring[r].id=ring[r].parent=ring[r].source=0;
        kill_seen[r]=kill_req[r]=0;
    }
    for(int i=0;i<PM_VOICES;i++)voice[i]=voice0;
    for(int i=0;i<PM_GRAINS;i++)grain[i]=grain0;
    for(int i=0;i<64;i++)scope[i]=0;
    half=scope_wr=burst_request=burst_seen=0;
    selected=-1;next_id=1;cap_i=0;cap_slot=cap_carry=-1;monitor=0;dropped_id=0;
    rng=0x9f26ad31u;half_sample=clock_phase=scan_phase=launch=0;loop_phase=0;turn=0;next_at=1;
    pm_panic();
    for(int i=0;i<PM_PARAMS;i++)p[i]=target[i]=defaults[i];
    gate_env=noise_lp=0;write_amt=1;dropping=0;
    mx=p[M_X];my=p[M_Y];
}
float pm_morph_x(void){return mx;}
float pm_morph_y(void){return my;}
int pm_active_grains(void){int n=0;for(int i=0;i<PM_GRAINS;i++)n+=grain[i].active;return n;}
void pm_scope(float *out,uint32_t n){
    if(n>64)n=64;uint32_t end=scope_wr;
    for(uint32_t i=0;i<n;i++)out[i]=scope[(end-n+i)&63u];
}
float pm_position(void){return position_now;}

/* ------------------------------------------------------------------ the rings (the UI's side) --- */
static void keep(int slot){for(int k=0;k<NPAR;k++)ring[slot].par[k]=target[RING_PAR[k]];}
static void bring(int slot){for(int k=0;k<NPAR;k++)target[RING_PAR[k]]=ring[slot].par[k];}
int pm_ring_count(void){int n=0;for(int r=0;r<PM_RINGS;r++)n+=ring[r].ready;return n;}
int pm_ring_selected(void){return selected;}
int pm_ring_info(int slot,int *id,int *parent,int *source){
    if(slot<0||slot>=PM_RINGS||!ring[slot].ready)return 0;
    if(id)*id=ring[slot].id;
    if(parent)*parent=ring[slot].parent;
    if(source)*source=ring[slot].source;
    return 1;
}
int pm_ring_select(int slot){
    int old=selected;
    if(slot<0||slot>=PM_RINGS||!ring[slot].ready||cap_state!=PM_CAP_IDLE)return -1;
    if(slot==old)return 0;
    if(old>=0&&ring[old].ready)keep(old);
    bring(slot);
    selected=slot;
    return 0;
}
int pm_ring_delete(int slot){
    int best=-1;
    if(slot<0||slot>=PM_RINGS||!ring[slot].ready)return -1;
    if(cap_state!=PM_CAP_IDLE&&cap_carry==slot)return -1;   /* (a running capture is reading it) */
    ring[slot].ready=0;
    kill_req[slot]++;                    /* its grains fade out */
    if(selected==slot){                  /* the newest of those left */
        for(int r=0;r<PM_RINGS;r++)if(ring[r].ready&&(best<0||ring[r].id>ring[best].id))best=r;
        if(best>=0)bring(best);
        selected=best;
    }
    return 0;
}
void pm_ring_clear(void){
    if(cap_state!=PM_CAP_IDLE)cap_abort=1;
    for(int r=0;r<PM_RINGS;r++)if(ring[r].ready){ring[r].ready=0;kill_req[r]++;}
    selected=-1;
}
void pm_ring_wave(int slot,float *lo,float *hi,uint32_t n){
    uint32_t count,span;
    if(!n)return;
    if(slot<0||slot>=PM_RINGS){for(uint32_t i=0;i<n;i++){lo[i]=1;hi[i]=-1;}return;}
    /* a finished ring: all of it; the one being captured: as far as it has been written; else nothing */
    count=ring[slot].ready?PM_BUFFER:(cap_state!=PM_CAP_IDLE&&cap_slot==slot)?cap_i:0u;
    if(count>PM_BUFFER)count=PM_BUFFER;
    span=PM_BUFFER/n;
    for(uint32_t i=0;i<n;i++){
        uint32_t base=i*span;
        float a=0,b=0;
        if(base+span>count){lo[i]=1;hi[i]=-1;continue;}
        for(uint32_t j=0;j<8;j++){
            float v=(float)memory[slot][(base+j*span/8u)&MASK]*(1.f/32768.f);
            if(v<a)a=v;
            if(v>b)b=v;
        }
        lo[i]=a;hi[i]=b;
    }
}
/* ------------------------------------------------------------------ a capture (the UI's side) --- */
static float cap_keep=-1.f;              /* a generation's share of the old ring (-1: an ordinary capture, CARRY's) */
int pm_capture_keep(int source,float keep){
    int r=pm_capture(source);
    if(!r)cap_keep=keep<0?0:keep>1?1:keep;   /* (the render reads it with the first sample it writes: set at once) */
    return r;
}
#ifdef PLAT_HOST
const int16_t *pm_ring_pcm(int slot){return slot>=0&&slot<PM_RINGS&&ring[slot].ready?memory[slot]:0;}
#endif
int pm_capture(int source){
    int slot=-1,sel=selected;
    if(cap_state!=PM_CAP_IDLE)return PM_ERR_BUSY;
    if(source==PM_SRC_SILT&&(sel<0||!ring[sel].ready))return PM_ERR_NOSRC;
    dropped_id=0;
    for(int r=0;r<PM_RINGS;r++)if(!ring[r].ready){slot=r;break;}
    if(slot<0){
        /* no room: the oldest ring gives up its place. Not the selected one: the capture is about to read it (as
         * SILT's source, or under CARRY) while it writes */
        for(int r=0;r<PM_RINGS;r++)if(r!=sel&&(slot<0||ring[r].id<ring[slot].id))slot=r;
        if(slot<0)return PM_ERR_FULL;    /* (one ring only, and it selected: cannot be) */
        dropped_id=ring[slot].id;
        ring[slot].ready=0;
        kill_req[slot]++;                /* its grains fade out */
    }
    cap_slot=slot;cap_source=source==PM_SRC_SILT?PM_SRC_SILT:PM_SRC_MORPH;
    cap_parent=cap_source==PM_SRC_SILT?ring[sel].id:0;
    cap_carry=sel>=0&&ring[sel].ready?sel:-1;
    cap_i=0;norm_i=0;cap_peak=0;cap_sq=0;cap_dc=0;cap_event=0;cap_abort=0;cap_keep=-1.f;
    gate_env=0;write_amt=1;dropping=0;
    cap_state=PM_CAP_RECORDING;          /* (last: from here the render writes it) */
    return 0;
}
int pm_capture_state(void){return cap_state;}
float pm_capture_progress(void){
    int st=cap_state;
    return st==PM_CAP_IDLE?0.f:st==PM_CAP_FINISHING?1.f:(float)cap_i*(1.f/PM_BUFFER);
}
int pm_capture_slot(void){return cap_state==PM_CAP_IDLE?-1:cap_slot;}
int pm_capture_source(void){return cap_source;}
int pm_service(int *slot){
    int ev=cap_event,old=selected;
    if(!ev)return PM_EV_NONE;
    if(ev==PM_EV_COMMITTED){
        Ring *r=&ring[cap_slot];
        if(old>=0&&ring[old].ready)keep(old);
        /* the new ring starts with the settings in use. Made from SILT's grains, what those already did to its
         * sound is set back, so that it is not done twice over */
        if(cap_source==PM_SRC_SILT){target[S_PITCH]=.5f;target[S_REVERSE]=0;target[S_SCATTER]=0;target[S_DETUNE]=0;}
        r->id=next_id++;r->parent=cap_parent;r->source=cap_source;
        keep(cap_slot);
        r->ready=1;
        selected=cap_slot;
        if(slot)*slot=cap_slot;
    }
    cap_event=0;
    cap_state=PM_CAP_IDLE;
    return ev;
}
#ifdef PLAT_WEB
/* For the browser emulator's guide (web/emu/app_worklet.js reads it, scripts/tutorial/tutorial.js follows it): the
 * rings there are (bits 0..3), the selected ring's number (4..7, 0 none), a capture running (8), how far it is in
 * percent (9..15), grains sounding (16..20). Only in the WebAssembly build. */
__attribute__((used, visibility("default"))) uint32_t pm_web_state(void){
    int sel=selected,id=sel>=0&&ring[sel].ready?ring[sel].id:0;
    return (uint32_t)pm_ring_count()|((uint32_t)(id&15)<<4)|((uint32_t)(cap_state!=PM_CAP_IDLE)<<8)|
           ((uint32_t)(pm_capture_progress()*100.f)<<9)|((uint32_t)(pm_active_grains()&31)<<16);
}
#endif
/* ------------------------------------------------------------------ the grains --- */
/* a grain's loudness now: up over its first part (SHAPE: how much of it), down over the rest, eased at both ends */
static float grain_env(const Grain *g){
    float t=g->age<g->atk?g->age*g->up:(1.f-g->age)*g->down;
    return t*t*(3.f-2.f*t);
}
int pm_grain(int i,float *at,float *env,int *slot){
    if(i<0||i>=PM_GRAINS||!grain[i].active)return 0;
    *at=grain[i].pos*(1.0f/PM_BUFFER);
    *env=grain_env(&grain[i])*grain[i].gain;
    if(slot)*slot=grain[i].slot;
    return grain[i].speed<0?-1:1;
}
static void spawn(float position,float amp,float semis){
    int sel=selected,s=-1;
    if(sel<0||!ring[sel].ready)return;       /* no ring: nothing to play */
    for(int i=0;i<PM_GRAINS;i++)if(!grain[i].active){s=i;break;}
    if(s<0)return; /* graceful saturation, never cut a sounding grain */
    Grain *g=&grain[s];
    float seconds=.020f+.480f*p[S_SIZE]*p[S_SIZE];
    if(p[S_VARY]>.005f)seconds=fclamp(seconds*fm_exp2f((rnd()-.5f)*4.f*p[S_VARY]),.010f,.75f);
    /* its pitch: the PITCH knob and its key; SCATTER's jump (octaves first, then a fifth, two octaves); DETUNE */
    float st=(p[S_PITCH]-.5f)*48.f+semis;
    if(p[S_SCATTER]>.005f&&rnd()<p[S_SCATTER]){
        static const int8_t JUMP[7]={12,-12,7,-5,24,19,-24};
        st+=(float)JUMP[(int)(rnd()*(2.f+p[S_SCATTER]*4.99f))];
    }
    if(p[S_DETUNE]>.005f)st+=(rnd()-.5f)*p[S_DETUNE];
    float speed=fclamp(.5f*fm_exp2f(st*(1.f/12.f)),.03f,4.f);      /* (a ring is at half rate: .5 is as recorded) */
    if(rnd()<p[S_REVERSE])speed=-speed;
    /* where it starts: POSITION round the ring, SPRAY's scatter about it (a ring is a loop: it reads on round) */
    g->pos=wrap(position+(rnd()-.5f)*p[S_SPRAY])*(float)(PM_BUFFER-1u);
    g->speed=speed;g->age=0;g->step=1.0f/(seconds*44100.f);
    g->atk=.03f+.94f*p[S_SHAPE];g->up=1.f/g->atk;g->down=1.f/(1.f-g->atk);
    g->pan=.5f+(rnd()-.5f)*p[G_STEREO];
    g->mem=memory[sel];g->slot=sel;g->gain=1;g->dying=0;
    g->amp=amp;g->active=1;
}
/* the next held key's pitch, in turn (none held: the recorded pitch) */
static float next_pitch(void){
    for(int k=0;k<PM_PITCHES;k++){
        int j=(turn+k)%PM_PITCHES,st=gpitch[j];
        if(st!=PM_NO_PITCH){turn=j+1;return (float)st;}
    }
    return 0.f;
}
/* one start: CLUSTER grains together, each on the next held key (slot >= 0: all on that key, just played) */
static void trigger(float position,int slot){
    static const float AMP[4]={.75f,.58f,.50f,.45f};
    int k=1+(int)(p[S_CLUSTER]*3.99f),st=slot>=0?gpitch[slot]:PM_NO_PITCH;
    for(int j=0;j<k;j++)spawn(position,AMP[k-1],slot<0?next_pitch():st==PM_NO_PITCH?0.f:(float)st);
}
static float read_grain(const int16_t *mem,float pos){
    uint32_t a=(uint32_t)pos;float f=pos-(float)a;
    float x=(float)mem[a&MASK],y=(float)mem[(a+1u)&MASK];
    return (x+(y-x)*f)*(1.0f/32768.f);
}
void pm_render(float *left,float *right,uint32_t n){
    if(!n)return;
    for(int j=0;j<PM_PARAMS;j++)p[j]+=(target[j]-p[j])*.15f;
    float w[4], motion=p[M_MOVE];
    clock_phase=wrap(clock_phase+(float)n*INV_RATE*(.015f+motion*motion*1.2f));
    mx=fclamp(p[M_X]+motion*.35f*fm_sin_turns(clock_phase),0,1);
    my=fclamp(p[M_Y]+motion*.35f*fm_sin_turns(clock_phase+.25f),0,1);
    w[0]=(1-mx)*my;w[1]=mx*my;w[2]=(1-mx)*(1-my);w[3]=mx*(1-my);
    float fade=1.0f/(441.f+88200.f*p[G_FADE]*p[G_FADE]);
    float tone=.02f+.62f*p[M_TONE]*p[M_TONE];
    float scan=p[G_SCAN];scan_phase=wrap(scan_phase+scan*.3f*(float)n*INV_RATE);
    /* LOOP: the play point's own way round the ring (a ring is 2 x PM_BUFFER samples long) */
    float loop=p[S_LOOP]<.03f?0.f:fm_exp2f(4.f*(p[S_LOOP]-.5f));
    loop_phase=wrap(loop_phase+loop*(float)n*(1.f/(2.f*PM_BUFFER)));
    float position=loop>0?wrap(p[S_POS]+loop_phase+scan*.45f*fm_sin_turns(scan_phase))
                         :fclamp(p[S_POS]+scan*.45f*fm_sin_turns(scan_phase),0,1);
    position_now=position;
    float rate=1.f+39.f*p[S_DENSITY]*p[S_DENSITY];
    float modal_decay=1.f-1.f/(2205.f+132300.f*p[M_EXCITE]*p[M_EXCITE]);
    int on=grains_on;
    /* what a capture does to its sound: put in (NOISE, COLOR, CRUSH), kept out (GATE), left out (DROP) */
    float hiss=p[R_NOISE]*p[R_NOISE]*.5f,color=p[R_COLOR];
    float gate_at=p[R_GATE]*p[R_GATE]*.25f;
    float steps=p[R_CRUSH]>.01f?fm_exp2f(14.f-12.f*p[R_CRUSH]):0.f;     /* 16384 .. 4 steps a side; 0: untouched */
    float drop_p=p[R_DROP]*p[R_DROP]*.5f;
    /* --- the capture's state for this block --- */
    if(cap_abort){cap_abort=0;cap_event=0;cap_state=PM_CAP_IDLE;}
    int capturing=cap_state==PM_CAP_RECORDING;
    int16_t *dest=memory[cap_slot>=0?cap_slot:0];
    const int16_t *under=capturing&&cap_carry>=0&&(cap_keep>=0?cap_keep>.005f:p[R_CARRY]>.005f)?memory[cap_carry]:0;
    int from_silt=cap_source==PM_SRC_SILT;
    if(cap_state==PM_CAP_FINISHING&&!cap_event){
        /* finishing: its level evened (the loudest sample to about .7; never more than four times up), a stretch a
         * block, then it is offered to pm_service. Nothing to speak of in it: dropped */
        if(!norm_i){
            if(cap_peak<.004f)cap_gain=0;
            else if(cap_keep<0)cap_gain=fclamp(.7f/cap_peak,.5f,4.f);
            else{
                /* a generation: evened by how loud it is throughout, so that one after another neither swells
                 * nor dies away (as far up as its loudest sample has room for) */
                float rms=fm_sqrtf(cap_sq*(1.f/PM_BUFFER));
                cap_gain=fclamp(fm_minf(GEN_LEVEL/(rms+1e-6f),.95f/cap_peak),.25f,16.f);
            }
        }
        if(cap_gain>0){
            uint32_t end=norm_i+2048u;
            for(uint32_t i=norm_i;i<end;i++)dest[i]=(int16_t)fclamp((float)dest[i]*cap_gain,-32767.f,32767.f);
            norm_i=end;
            if(norm_i>=PM_BUFFER)cap_event=PM_EV_COMMITTED;
        }else cap_event=PM_EV_SILENT;
    }
    for(int r=0;r<PM_RINGS;r++){             /* a deleted ring's grains: out, quickly */
        uint32_t k=kill_req[r];
        if(k!=kill_seen[r]){kill_seen[r]=k;for(int j=0;j<PM_GRAINS;j++)if(grain[j].active&&grain[j].slot==r)grain[j].dying=1;}
    }
    uint32_t br=burst_request;
    if(br!=burst_seen){burst_seen=br;for(int j=0;j<3;j++)trigger(position,-1);}
    for(int j=0;j<PM_PITCHES;j++){           /* a key played: its grains now */
        uint32_t sr=strike_request[j];
        if(sr!=strike_seen[j]){strike_seen[j]=sr;trigger(position,j);trigger(position,j);}
    }
    float timing=p[S_TIMING];
    /* DECAY: how long a struck MORPH note lives: 80 ms .. 10 s to a third of its level; at the top, held */
    float life_k=p[M_SUSTAIN]>=.97f?1.f:1.f-INV_RATE/(.08f*fm_exp2f(p[M_SUSTAIN]*7.f));
    for(int j=0;j<PM_VOICES;j++){            /* a note struck (again): its life from the top, its bell rung again */
        uint32_t k=struck[j];
        if(k!=struck_seen[j]){struck_seen[j]=k;voice[j].life=1;voice[j].modal=1;}
    }
    int sounding=0;
    for(int j=0;j<PM_VOICES;j++){
        Voice *v=&voice[j];int req=requested[j];
        if(req>=0&&v->note!=req&&v->env<.002f){
            v->note=req;v->inc=440.f*fm_exp2f((req-69.f)/12.f)*INV_RATE;
            v->phase=v->m1=v->m2=0;v->modal=1;v->filt=0;
        }
        if(req>=0||v->env>.0001f)sounding++;
    }
    float norm=sounding>1?1.f/(.7f+.3f*sounding):1.f;
    for(uint32_t i=0;i<n;i++){
        float dl=0,dr=0,gl=0,gr=0;int active=0;
        for(int j=0;j<PM_VOICES;j++){
            Voice *v=&voice[j];int req=requested[j];
            if(req>=0&&v->note!=req&&v->env<.002f){
                v->note=req;v->inc=440.f*fm_exp2f((req-69.f)/12.f)*INV_RATE;
                v->phase=v->m1=v->m2=0;v->modal=1;v->filt=0;
            }
            /* its level follows its life: 1 while held (DECAY at the top), else dying away from the strike */
            v->life*=life_k;
            float gate=(req>=0&&req==v->note&&v->life>.003f)?v->life:0.f;
            float c=(req>=0&&req!=v->note)?.004f:fade;
            v->env+=(gate-v->env)*c;
            if(v->env<.00003f&&!gate){v->env=0;continue;}
            float a=v->phase, base=fm_sin_turns(a), sig=0;
            /* a struck note is at its richest and brightest as it is struck, and plainer and darker as it dies
             * away (held, DECAY at the top: as the knobs say throughout) */
            float complex=p[M_COMPLEX]*(.3f+.7f*v->life);
            if(w[0]>.0001f)
                sig+=w[0]*fm_sin_turns(a+(.06f+.65f*complex)*fm_sin_turns(a*2.f));
            if(w[1]>.0001f)
                sig+=w[1]*(.55f*base+.28f*fm_sin_turns(v->m1)+.17f*fm_sin_turns(v->m2))*v->modal;
            if(w[2]>.0001f)
                sig+=w[2]*(base+complex*(.55f*fm_sin_turns(a*2.f)+.32f*fm_sin_turns(a*3.f)+.13f*fm_sin_turns(a*5.f)))/(1.f+complex);
            if(w[3]>.0001f){
                float x=base*(1.f+3.f*complex);
                /* Continuous triangle fold. */
                float q=wrap(x*.25f+.25f);
                float folded=1.f-4.f*(q>.5f?q-.5f:.5f-q);
                sig+=w[3]*((1-complex)*base+complex*folded);
            }
            v->modal*=modal_decay;
            v->phase=wrap(a+v->inc);
            v->m1=wrap(v->m1+v->inc*2.756f);
            v->m2=wrap(v->m2+v->inc*5.404f);
            v->filt+=(sig-v->filt)*tone*(.45f+.55f*v->life);
            float val=v->filt*v->env*.32f*norm;
            float pan=.5f+((float)j-1.5f)*p[M_SPREAD]*.27f;
            dl+=val*(1.f-pan);dr+=val*pan;
        }
        if(on){
            launch+=rate*INV_RATE;
            if(launch>=next_at){             /* TIMING: on the pulse .. anywhere (short gaps and long, the same on average) */
                launch-=next_at;
                next_at=timing>.005f?(1.f-timing)+timing*4.f*rnd()*rnd():1.f;
                if(next_at<.05f)next_at=.05f;
                trigger(position,-1);
            }
        }
        for(int j=0;j<PM_GRAINS;j++){
            Grain *g=&grain[j];if(!g->active)continue;active++;
            float sample=read_grain(g->mem,g->pos)*grain_env(g)*g->amp*g->gain;
            gl+=sample*(1.f-g->pan);gr+=sample*g->pan;
            g->pos+=g->speed;
            if(g->pos>=PM_BUFFER)g->pos-=PM_BUFFER;
            if(g->pos<0)g->pos+=PM_BUFFER;
            g->age+=g->step;if(g->age>=1.f)g->active=0;
            if(g->dying){g->gain-=1.f/128.f;if(g->gain<=0)g->active=0;}
        }
        gl=soft(gl*1.4f);gr=soft(gr*1.4f);
        float dry=dl+dr, wet=gl+gr;
        /* The capture's source: MORPH alone, or SILT's grains alone (never the two by accident). Written at half
         * rate: pair averaging after a mild lowpass (DAMP darkens it). */
        float rec=from_silt?wet:dry;
        capture_lp+=(rec-capture_lp)*(.25f+.3f*(1-p[G_DAMP]));
        if(!half){half_sample=capture_lp;half=1;}
        else{
            half=0;
            if(capturing){
                uint32_t ci=cap_i;
                float in=(half_sample+capture_lp)*.5f;
                cap_dc+=(in-cap_dc)*.006f;in-=cap_dc;            /* nothing under about 20 Hz: generation on
                                                                  * generation it would pile up */
                float loud=in<0?-in:in,nz=rnd()*2.f-1.f,want,out;
                /* GATE listens to the sound itself (not the hiss): quick to open, about 75 ms to close */
                gate_env+=(loud-gate_env)*(loud>gate_env?.2f:.0006f);
                if((ci&255u)==0)dropping=rnd()<drop_p;          /* DROP: every 12 ms the next 12 may be left out */
                want=(gate_at<=0||gate_env>gate_at)&&!dropping?1.f:0.f;
                write_amt+=(want-write_amt)*.02f;
                noise_lp+=(nz-noise_lp)*.04f;                    /* NOISE: a rumble (COLOR 0) .. a hiss (COLOR 100) */
                in+=hiss*((1.f-color)*noise_lp*5.f+color*(nz-noise_lp));
                out=in*write_amt;                                /* (gated or dropped: silence) */
                if(under){
                    float old=(float)under[ci]*(1.f/32768.f),k=cap_keep;
                    if(k<0)out+=old*p[R_CARRY];                  /* CARRY: the selected ring under it */
                    else out=out*(1.f-k)*GEN_GAIN+old*k;         /* a generation: so much of it the ring as it was */
                }
                if(steps>0)out=fm_floorf(out*steps+.5f)/steps;    /* CRUSH */
                if(ci<SEAM)out*=(float)ci*(1.f/SEAM);            /* the ends eased: the loop closes without a click */
                else if(ci>=PM_BUFFER-SEAM)out*=(float)(PM_BUFFER-1u-ci)*(1.f/SEAM);
                out=fclamp(out,-.999f,.999f);
                if(out>cap_peak)cap_peak=out;
                if(-out>cap_peak)cap_peak=-out;
                cap_sq+=out*out;
                dest[ci]=(int16_t)(out*32767.f);
                cap_i=++ci;
                if(ci>=PM_BUFFER){capturing=0;cap_state=PM_CAP_FINISHING;}
            }
        }
        /* Turning grains off releases existing grains while returning to dry. */
        float mix=(on||active>0)?p[G_MIX]:0.f;
        if(monitor&&mix>.3f)mix=.3f;         /* MORPH's own page, or a capture of it: MORPH in front, whatever MIX says */
        wet_gain+=(mix-wet_gain)*.002f;dry_gain+=(1.f-mix-dry_gain)*.002f;
        float l=(dl*dry_gain+gl*wet_gain)*p[G_LEVEL]*2.f;
        float r=(dr*dry_gain+gr*wet_gain)*p[G_LEVEL]*2.f;
        float ol=l-prev_l+.9995f*dc_l,orr=r-prev_r+.9995f*dc_r;
        prev_l=l;prev_r=r;dc_l=ol;dc_r=orr;
        left[i]=soft(ol);right[i]=soft(orr);
        if((i&15u)==0)scope[scope_wr++&63u]=(left[i]+right[i])*.5f;
    }
}
