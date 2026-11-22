/* SPDX-License-Identifier: MPL-2.0 */
/* Shared device / host / WebAssembly audio engine. No allocation in render. */
#include "palimpsest.h"
#include "fastmath.h"
#include "plat.h"
#include <string.h>
#define MASK (PM_BUFFER-1u)
#define INV_RATE (1.0f/44100.0f)
static int16_t memory[PM_BUFFER] PLAT_POOL;
typedef struct {
    float phase, m1, m2, env, modal, filt, inc;
    int note;
} Voice;
typedef struct {float pos, speed, age, step, pan, amp; int active;} Grain;
static Voice voice[PM_VOICES];
static Grain grain[PM_GRAINS];
static volatile int requested[PM_VOICES];
static volatile int capture, grains_on;
static volatile uint32_t burst_request;
static uint32_t burst_seen, half, rng;
static volatile uint32_t wr, filled, scope_wr;
static float half_sample, clock_phase, scan_phase, launch, dry_gain, wet_gain;
static float capture_lp, dc_l, dc_r, prev_l, prev_r, scope[64];
static volatile float target[PM_PARAMS];
static float p[PM_PARAMS];
static float mx,my;
static const float defaults[PM_PARAMS]={
 .35f,.65f,.35f,.5f,.4f,0,.55f,
 .20f,.35f,.35f,.20f,.5f,0,.25f,
 .65f,.20f,0,.4f,.65f,.35f,.75f};
static float clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
static float wrap(float x){return x-fm_floorf(x);}
static float rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (float)(rng>>8)*(1.0f/16777216.0f);}
static float soft(float x){return x/(1.0f+(x<0?-x:x));}
void pm_set(int id,float v){if(id>=0&&id<PM_PARAMS&&v>=0&&v<=1)target[id]=v;}
float pm_get(int id){return id>=0&&id<PM_PARAMS?target[id]:0;}
void pm_note(int slot,int note){if(slot>=0&&slot<PM_VOICES)requested[slot]=note>=0&&note<=127?note:-1;}
void pm_record(int mode){if(mode>=0&&mode<=3)capture=mode;}
void pm_grains(int on){if(on&&!grains_on)burst_request++;grains_on=on!=0;}
void pm_burst(void){burst_request++;}
void pm_panic(void){
    for(int i=0;i<PM_VOICES;i++){requested[i]=-1;voice[i].env=0;voice[i].note=-1;}
    for(int i=0;i<PM_GRAINS;i++)grain[i].active=0;
    grains_on=0;burst_seen=burst_request; wet_gain=0;dry_gain=1;
    capture_lp=dc_l=dc_r=prev_l=prev_r=0;
}
void pm_init(void){
    memset(memory,0,sizeof memory);memset(voice,0,sizeof voice);
    memset(grain,0,sizeof grain);memset(scope,0,sizeof scope);
    wr=filled=half=scope_wr=burst_request=burst_seen=0;
    rng=0x9f26ad31u;half_sample=clock_phase=scan_phase=launch=0;
    capture=PM_LIVE;pm_panic();
    for(int i=0;i<PM_PARAMS;i++)p[i]=target[i]=defaults[i];
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
static void spawn(float position,float amp){
    if(filled<128)return;
    int s=-1;
    for(int i=0;i<PM_GRAINS;i++)if(!grain[i].active){s=i;break;}
    if(s<0)return; /* graceful saturation, never cut a sounding grain */
    Grain *g=&grain[s];
    float seconds=.020f+.280f*p[S_SIZE]*p[S_SIZE];
    float speed=.5f*fm_exp2f((p[S_PITCH]-.5f)*4.0f);
    if(rnd()<p[S_REVERSE])speed=-speed;
    /* Position is time behind the write head. Avoid crossing the writer for live grains. */
    float span=(float)filled-4.f;
    float safety=32.f+(speed>0?speed:0.f)*seconds*44100.f;
    if(capture!=PM_FREEZE && safety>span)safety=span;
    if(capture==PM_FREEZE)safety=2.f;
    float t=clamp(position+(rnd()-.5f)*p[S_SPRAY],0,1);
    float behind=safety+t*(span-safety);
    g->pos=(float)wr-behind;
    while(g->pos<0)g->pos+=PM_BUFFER;
    g->speed=speed;g->age=0;g->step=1.0f/(seconds*44100.f);
    g->pan=.5f+(rnd()-.5f)*p[G_STEREO];
    g->amp=amp;g->active=1;
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
    mx=clamp(p[M_X]+motion*.35f*fm_sin_turns(clock_phase),0,1);
    my=clamp(p[M_Y]+motion*.35f*fm_sin_turns(clock_phase+.25f),0,1);
    w[0]=(1-mx)*my;w[1]=mx*my;w[2]=(1-mx)*(1-my);w[3]=mx*(1-my);
    float fade=1.0f/(441.f+88200.f*p[G_FADE]*p[G_FADE]);
    float tone=.02f+.62f*p[M_TONE]*p[M_TONE];
    float scan=p[G_SCAN];scan_phase=wrap(scan_phase+scan*.3f*(float)n*INV_RATE);
    float position=clamp(p[S_POS]+scan*.45f*fm_sin_turns(scan_phase),0,1);
    float rate=1.f+39.f*p[S_DENSITY]*p[S_DENSITY];
    float modal_decay=1.f-1.f/(2205.f+132300.f*p[M_EXCITE]*p[M_EXCITE]);
    int mode=capture,on=grains_on;
    uint32_t br=burst_request;
    if(br!=burst_seen){burst_seen=br;for(int j=0;j<3;j++)spawn(position,.75f);}
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
        if(on){launch+=rate*INV_RATE;if(launch>=1.f){launch-=1.f;spawn(position,.75f);}}
        for(int j=0;j<PM_GRAINS;j++){
            Grain *g=&grain[j];if(!g->active)continue;active++;
            float env=fm_sin_turns(g->age*.5f);
            env=env*(1.f-p[G_WINDOW])+env*env*p[G_WINDOW];
            float sample=read_grain(g->pos)*env*g->amp;
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
                float in=(half_sample+capture_lp)*.5f;
                if(mode==PM_OVERDUB)
                    in=.25f*in+(float)memory[wr]*(1.f/32768.f)*(.55f+.40f*p[S_FEEDBACK]);
                memory[wr]=(int16_t)(clamp(in,-.999f,.999f)*32767.f);
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
