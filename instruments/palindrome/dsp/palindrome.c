/* SPDX-License-Identifier: MPL-2.0 */
/* Shared device / host / WebAssembly audio engine. No allocation in render. */
#include "palindrome.h"
#include "fastmath.h"
#include "plat.h"                        /* PLAT_POOL; its clamp is the integer one: floats use fclamp */
#define MASK (PM_BUFFER-1u)
#define INV_RATE (1.0f/44100.0f)
static int16_t memory[PM_BUFFER] PLAT_POOL;
typedef struct {
    float phase, m1, m2, env, modal, filt, inc;
    int note;
} Voice;
typedef struct {float pos, speed, age, step, pan, amp, atk, up, down; int active;} Grain;
static Voice voice[PM_VOICES];
static Grain grain[PM_GRAINS];
static volatile int requested[PM_VOICES];
static volatile int capture, grains_on;
static volatile uint32_t burst_request;
static volatile int gpitch[PM_PITCHES];        /* the keys held for the grains, semitones (PM_NO_PITCH: none) */
static volatile uint32_t strike_request[PM_PITCHES];
static uint32_t strike_seen[PM_PITCHES];
static int turn;                         /* whose turn among the held pitches */
static float next_at;                    /* the next grain's time, in even pulses (1: on the pulse) */
static uint32_t burst_seen, half, rng;
static volatile uint32_t wr, filled, scope_wr;
static float half_sample, clock_phase, scan_phase, launch, dry_gain, wet_gain;
static float capture_lp, dc_l, dc_r, prev_l, prev_r, scope[64];
static volatile float target[PM_PARAMS];
static float p[PM_PARAMS];
static float mx,my;
static volatile float position_now;
static float gate_env,noise_lp;          /* the gate's ear on the sound; the hiss's low part */
static volatile float write_amt;         /* the head on the memory: 1 writing, 0 lifted (eased over ~2 ms) */
static int dropping;
static const float defaults[PM_PARAMS]={
 .35f,.65f,.35f,.5f,.4f,0,.55f,
 .20f,.35f,.35f,.20f,.5f,0,.25f,
 .65f,.20f,0,.5f,.65f,.35f,.75f,          /* (SHAPE: a bell) */
 0,.5f,0,0,0,                            /* a clean recording: no hiss, no crush, no gate, no dropouts */
 0,0,0,0,0};                             /* grains on an even pulse, all one length and pitch, one at a time */
static float fclamp(float x,float a,float b){return x<a?a:x>b?b:x;}
static float wrap(float x){return x-fm_floorf(x);}
static float rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (float)(rng>>8)*(1.0f/16777216.0f);}
static float soft(float x){return x/(1.0f+(x<0?-x:x));}
void pm_set(int id,float v){if(id>=0&&id<PM_PARAMS&&v>=0&&v<=1)target[id]=v;}
float pm_get(int id){return id>=0&&id<PM_PARAMS?target[id]:0;}
void pm_note(int slot,int note){if(slot>=0&&slot<PM_VOICES)requested[slot]=note>=0&&note<=127?note:-1;}
void pm_record(int mode){if(mode>=0&&mode<=3)capture=mode;}
void pm_grains(int on){if(on&&!grains_on)burst_request++;grains_on=on!=0;}
void pm_burst(void){burst_request++;}
void pm_grain_pitch(int slot,int semis){
    if(slot>=0&&slot<PM_PITCHES)gpitch[slot]=semis>=-48&&semis<=48?semis:PM_NO_PITCH;
}
void pm_strike(int slot){if(slot>=0&&slot<PM_PITCHES)strike_request[slot]++;}
void pm_panic(void){
    for(int i=0;i<PM_VOICES;i++){requested[i]=-1;voice[i].env=0;voice[i].note=-1;}
    for(int i=0;i<PM_GRAINS;i++)grain[i].active=0;
    for(int i=0;i<PM_PITCHES;i++){gpitch[i]=PM_NO_PITCH;strike_seen[i]=strike_request[i];}
    grains_on=0;burst_seen=burst_request; wet_gain=0;dry_gain=1;
    capture_lp=dc_l=dc_r=prev_l=prev_r=0;
}
void pm_init(void){
    /* (plain loops: the FM-1's toolchain has no <string.h>) */
    static const Voice voice0;static const Grain grain0;
    for(uint32_t i=0;i<PM_BUFFER;i++)memory[i]=0;
    for(int i=0;i<PM_VOICES;i++)voice[i]=voice0;
    for(int i=0;i<PM_GRAINS;i++)grain[i]=grain0;
    for(int i=0;i<64;i++)scope[i]=0;
    wr=filled=half=scope_wr=burst_request=burst_seen=0;
    rng=0x9f26ad31u;half_sample=clock_phase=scan_phase=launch=0;turn=0;next_at=1;
    capture=PM_LIVE;pm_panic();
    for(int i=0;i<PM_PARAMS;i++)p[i]=target[i]=defaults[i];
    gate_env=noise_lp=0;write_amt=1;dropping=0;
    mx=p[M_X];my=p[M_Y];
}
float pm_fill(void){return (float)filled/PM_BUFFER;}
float pm_morph_x(void){return mx;}
float pm_morph_y(void){return my;}
int pm_active_grains(void){int n=0;for(int i=0;i<PM_GRAINS;i++)n+=grain[i].active;return n;}
void pm_scope(float *out,uint32_t n){
    if(n>64)n=64;uint32_t end=scope_wr;
    for(uint32_t i=0;i<n;i++)out[i]=scope[(end-n+i)&63u];
}
/* Lightweight preview of audio history, newest at left. Reading aligned
 * 16-bit samples from the UI never blocks the audio writer. */
void pm_memory(float *lo,float *hi,uint32_t n){
    uint32_t end=wr,count=filled;
    if(!n)return;
    for(uint32_t i=0;i<n;i++){
        float a=0,b=0;
        for(uint32_t j=0;j<16;j++){
            uint32_t off=(i*16u+j)*count/(n*16u);
            if(count){float v=(float)memory[(end-1u-off)&MASK]*(1.f/32768.f);
                if(v<a)a=v;if(v>b)b=v;}
        }
        lo[i]=a;hi[i]=b;
    }
}
float pm_head(void){return (float)wr*(1.0f/PM_BUFFER);}
float pm_writing(void){return capture==PM_FREEZE?0.f:write_amt;}
float pm_position(void){return position_now;}
/* The memory where it lies (not by age): a picture that stays put while the head writes over it. */
void pm_ring(float *lo,float *hi,uint32_t n){
    uint32_t count=filled,span;
    if(!n)return;
    span=PM_BUFFER/n;
    for(uint32_t i=0;i<n;i++){
        uint32_t base=i*span;
        float a=0,b=0;
        if(base>=count){lo[i]=1;hi[i]=-1;continue;}   /* (before the first lap: written from 0 up to the head) */
        for(uint32_t j=0;j<8;j++){
            float v=(float)memory[(base+j*span/8u)&MASK]*(1.f/32768.f);
            if(v<a)a=v;
            if(v>b)b=v;
        }
        lo[i]=a;hi[i]=b;
    }
}
/* a grain's loudness now: up over its first part (SHAPE: how much of it), down over the rest, eased at both ends */
static float grain_env(const Grain *g){
    float t=g->age<g->atk?g->age*g->up:(1.f-g->age)*g->down;
    return t*t*(3.f-2.f*t);
}
int pm_grain(int i,float *at,float *env){
    if(i<0||i>=PM_GRAINS||!grain[i].active)return 0;
    *at=grain[i].pos*(1.0f/PM_BUFFER);
    *env=grain_env(&grain[i]);
    return grain[i].speed<0?-1:1;
}
static void spawn(float position,float amp,float semis){
    if(filled<128)return;
    int s=-1;
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
    float speed=fclamp(.5f*fm_exp2f(st*(1.f/12.f)),.03f,4.f);      /* (the memory is at half rate: .5 is as recorded) */
    if(rnd()<p[S_REVERSE])speed=-speed;
    /* Position is time behind the write head. Avoid crossing the writer for live grains. */
    float span=(float)filled-4.f;
    float safety=32.f+(speed>0?speed:0.f)*seconds*44100.f;
    if(capture!=PM_FREEZE && safety>span)safety=span;
    if(capture==PM_FREEZE)safety=2.f;
    float t=fclamp(position+(rnd()-.5f)*p[S_SPRAY],0,1);
    float behind=safety+t*(span-safety);
    g->pos=(float)wr-behind;
    while(g->pos<0)g->pos+=PM_BUFFER;
    g->speed=speed;g->age=0;g->step=1.0f/(seconds*44100.f);
    g->atk=.03f+.94f*p[S_SHAPE];g->up=1.f/g->atk;g->down=1.f/(1.f-g->atk);
    g->pan=.5f+(rnd()-.5f)*p[G_STEREO];
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
static float read_grain(float pos){
    uint32_t a=(uint32_t)pos;float f=pos-(float)a;
    float x=(float)memory[a&MASK],y=(float)memory[(a+1u)&MASK];
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
    float position=fclamp(p[S_POS]+scan*.45f*fm_sin_turns(scan_phase),0,1);
    position_now=position;
    float rate=1.f+39.f*p[S_DENSITY]*p[S_DENSITY];
    float modal_decay=1.f-1.f/(2205.f+132300.f*p[M_EXCITE]*p[M_EXCITE]);
    int mode=capture,on=grains_on;
    /* the recording's noise: put in (NOISE, COLOR, CRUSH), kept out (GATE), and the head's skips (DROP) */
    float hiss=p[R_NOISE]*p[R_NOISE]*.5f,color=p[R_COLOR];
    float gate_at=p[R_GATE]*p[R_GATE]*.25f;
    float steps=p[R_CRUSH]>.01f?fm_exp2f(14.f-12.f*p[R_CRUSH]):0.f;     /* 16384 .. 4 steps a side; 0: untouched */
    float drop_p=p[R_DROP]*p[R_DROP]*.5f;
    uint32_t br=burst_request;
    if(br!=burst_seen){burst_seen=br;for(int j=0;j<3;j++)trigger(position,-1);}
    for(int j=0;j<PM_PITCHES;j++){           /* a key played: its grains now */
        uint32_t sr=strike_request[j];
        if(sr!=strike_seen[j]){strike_seen[j]=sr;trigger(position,j);trigger(position,j);}
    }
    float timing=p[S_TIMING];
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
            float gate=(req>=0&&req==v->note)?1.f:0.f;
            float c=(req>=0&&req!=v->note)?.004f:fade;
            v->env+=(gate-v->env)*c;
            if(v->env<.00003f&&!gate){v->env=0;continue;}
            float a=v->phase, base=fm_sin_turns(a), sig=0;
            float complex=p[M_COMPLEX];
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
            v->filt+=(sig-v->filt)*tone;
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
            float sample=read_grain(g->pos)*grain_env(g)*g->amp;
            /* Suppress a moving-write-head crossing; frozen buffers remain readable. */
            if(mode!=PM_FREEZE){
                float dist=(float)wr-g->pos;
                if(dist<0)dist+=PM_BUFFER;
                if(dist<20.f)sample*=dist*.05f;
                if(dist>PM_BUFFER-20.f)sample*=(PM_BUFFER-dist)*.05f;
            }
            gl+=sample*(1.f-g->pan);gr+=sample*g->pan;
            g->pos+=g->speed;
            if(g->pos>=PM_BUFFER)g->pos-=PM_BUFFER;
            if(g->pos<0)g->pos+=PM_BUFFER;
            g->age+=g->step;if(g->age>=1.f)g->active=0;
        }
        gl=soft(gl*1.4f);gr=soft(gr*1.4f);
        float dry=dl+dr, wet=gl+gr;
        /* Write at half rate, pair averaging then a mild anti-alias lowpass. */
        float rec=dry;
        if(mode==PM_RESAMPLE)rec=soft(dry*.25f+wet*(.55f+.40f*p[S_FEEDBACK]));
        capture_lp+=(rec-capture_lp)*(.25f+.3f*(1-p[G_DAMP]));
        if(!half){half_sample=capture_lp;half=1;}
        else{
            half=0;
            if(mode!=PM_FREEZE){
                float in=(half_sample+capture_lp)*.5f,old=(float)memory[wr]*(1.f/32768.f);
                float loud=in<0?-in:in,n=rnd()*2.f-1.f,want;
                /* GATE listens to the sound itself (not the hiss): quick to open, about 75 ms to close */
                gate_env+=(loud-gate_env)*(loud>gate_env?.2f:.0006f);
                if((wr&255u)==0)dropping=rnd()<drop_p;          /* DROP: every 12 ms the head may skip the next 12 */
                want=(gate_at<=0||gate_env>gate_at)&&!dropping?1.f:0.f;
                write_amt+=(want-write_amt)*.02f;
                noise_lp+=(n-noise_lp)*.04f;                     /* NOISE: a rumble (COLOR 0) .. a hiss (COLOR 100) */
                in+=hiss*((1.f-color)*noise_lp*5.f+color*(n-noise_lp));
                if(mode==PM_OVERDUB)
                    in=.25f*in+old*(.55f+.40f*p[S_FEEDBACK]);
                if(steps>0)in=fm_floorf(in*steps+.5f)/steps;      /* CRUSH */
                in=fclamp(in,-.999f,.999f);
                /* where the head is lifted, what was there stays */
                memory[wr]=(int16_t)((old+(in-old)*write_amt)*32767.f);
                wr=(wr+1u)&MASK;if(filled<PM_BUFFER)filled++;
            }
        }
        /* Turning grains off releases existing grains while returning to dry. */
        float mix=(on||active>0)?p[G_MIX]:0.f;
        wet_gain+=(mix-wet_gain)*.002f;dry_gain+=(1.f-mix-dry_gain)*.002f;
        float l=(dl*dry_gain+gl*wet_gain)*p[G_LEVEL]*2.f;
        float r=(dr*dry_gain+gr*wet_gain)*p[G_LEVEL]*2.f;
        float ol=l-prev_l+.9995f*dc_l,orr=r-prev_r+.9995f*dc_r;
        prev_l=l;prev_r=r;dc_l=ol;dc_r=orr;
        left[i]=soft(ol);right[i]=soft(orr);
        if((i&15u)==0)scope[scope_wr++&63u]=(left[i]+right[i])*.5f;
    }
}
