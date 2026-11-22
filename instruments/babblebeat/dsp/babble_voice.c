/* SPDX-License-Identifier: MPL-2.0
 * BabbleBeat's voice: a small formant synthesizer in the manner of Klatt's.
 *
 *   voiced sounds   a glottal pulse (the flow's derivative, its closing edge band-limited) and breath noise through
 *                   four formant resonators in cascade, so each vowel's formants come out at their natural levels
 *   hisses, bursts  noise through one resonator of its own, beside the cascade
 *   between sounds  nothing switches: the formants and the levels move toward each phone's targets over a few
 *                   milliseconds, which is most of what makes speech out of a row of vowels. A stop is a closure,
 *                   a burst and (unvoiced) a breath; its formants start from where the lips or the tongue were
 *   pitch           one line over the sentence: a little higher at the start, a lift on each word, a fall at the
 *                   end or a rise for a question (INFLECT). A word played from a key holds the key's pitch
 *
 * What to say arrives from the panel while the sound is being made (on the FM-1, in an interrupt): it is written
 * into a slot the voice is not reading and taken over at a control step, the voice fading first, so nothing is
 * read half written and nothing is cut with a click. Single-precision floats, no library. */
#include "babble_voice.h"
#include "fastmath.h"
#define BB_SR 44100.f
#define CTL 64u                          /* samples in a control step (1.45 ms) */
#define STEP_MS (1000.f*(float)CTL/BB_SR)
#define GAIN_VOICE .9f                   /* the cascade's and the hiss's levels (set by ear and by tests/voice.c) */
#define GAIN_HISS 2.2f
/* Set against a speech recogniser's score on the lexicon's words (and checked by ear): the table's lengths are
 * spoken at three quarters, the formants glide a little slower and ring a little longer than the textbook's */
#define DUR .75f
#define GLIDE .8f
#define BW .8f
#define NASAL 200.f                      /* Hz added to the upper formants' widths while a nasal sounds */
static float clip(float x,float a,float b){return x<a?a:x>b?b:x;}

/* ----------------------------------------------------------------------------- the phones --- */
enum {K_VOW,K_DIP,K_GLI,K_NAS,K_FRI,K_VFR,K_ASP,K_STP,K_VST,K_AFF,K_VAF};
typedef struct {char c;uint8_t kind;float f1,f2,f3,g1,g2,g3,ms,amp,ff,fbw;} phone;
/* Formants in Hz (a diphthong: where it starts, then where it ends); ms: its length at RATE's middle; amp: how
 * loud its voice (vowels, glides, nasals) or its hiss or burst (the rest) is; ff, fbw: the hiss's centre and width.
 * A stop's f1..f3 are its locus: where the formants start from when it opens.
 * The spellings (lang/babble_lang.c): i ee, I ih, e eh, A a(t), a ah, O aw, U uh, V oo(k), u oo, E er, o o(r);
 * 1 ay, 2 eye, 3 ow, 4 oy, 5 oh; S sh, T th(in), D th(e), C ch, J j, N ng; the others as written. */
static const phone PH[]={
 {'i',K_VOW,280,2250,2950,0,0,0,115,1.f,0,0},
 {'I',K_VOW,400,1950,2550,0,0,0,75,1.f,0,0},
 {'e',K_VOW,540,1800,2500,0,0,0,90,1.f,0,0},
 {'A',K_VOW,680,1700,2450,0,0,0,115,1.f,0,0},
 {'a',K_VOW,740,1120,2500,0,0,0,125,1.f,0,0},
 {'O',K_VOW,590,880,2450,0,0,0,125,1.f,0,0},
 {'U',K_VOW,620,1220,2450,0,0,0,70,.95f,0,0},
 {'V',K_VOW,450,1050,2300,0,0,0,80,1.f,0,0},
 {'u',K_VOW,310,900,2250,0,0,0,115,1.f,0,0},
 {'E',K_VOW,490,1350,1700,0,0,0,115,1.f,0,0},
 {'o',K_VOW,480,860,2450,0,0,0,110,1.f,0,0},
 {'1',K_DIP,520,1850,2500,330,2200,2750,150,1.f,0,0},
 {'2',K_DIP,740,1200,2500,360,2100,2700,165,1.f,0,0},
 {'3',K_DIP,740,1250,2500,400,900,2300,165,1.f,0,0},
 {'4',K_DIP,560,860,2450,350,2000,2600,170,1.f,0,0},
 {'5',K_DIP,540,1000,2450,360,820,2300,150,1.f,0,0},
 {'w',K_GLI,300,650,2200,0,0,0,60,.8f,0,0},
 {'y',K_GLI,270,2250,3050,0,0,0,55,.8f,0,0},
 {'r',K_GLI,330,1150,1500,0,0,0,65,.8f,0,0},
 {'l',K_GLI,370,1250,2750,0,0,0,60,.8f,0,0},
 {'m',K_NAS,270,1000,2200,0,0,0,65,.5f,0,0},
 {'n',K_NAS,270,1650,2600,0,0,0,60,.5f,0,0},
 {'N',K_NAS,270,2100,2700,0,0,0,70,.5f,0,0},
 {'s',K_FRI,0,0,0,0,0,0,100,.55f,5800,2600},
 {'S',K_FRI,0,0,0,0,0,0,105,.70f,2900,1400},
 {'f',K_FRI,0,0,0,0,0,0,85,.20f,4600,6000},
 {'T',K_FRI,0,0,0,0,0,0,80,.16f,5200,6000},
 {'z',K_VFR,260,1700,2700,0,0,0,75,.40f,5800,2600},
 {'v',K_VFR,250,1100,2300,0,0,0,55,.12f,4600,6000},
 {'D',K_VFR,270,1500,2600,0,0,0,45,.10f,5200,6000},
 {'h',K_ASP,0,0,0,0,0,0,55,.55f,0,0},
 {'p',K_STP,200,900,2200,0,0,0,55,.40f,900,1600},
 {'t',K_STP,200,1750,2700,0,0,0,50,.75f,4600,2600},
 {'k',K_STP,200,1950,2300,0,0,0,55,.65f,2100,1000},
 {'b',K_VST,200,900,2200,0,0,0,50,.25f,900,1600},
 {'d',K_VST,200,1750,2700,0,0,0,45,.45f,4600,2600},
 {'g',K_VST,200,1950,2300,0,0,0,50,.40f,2100,1000},
 {'C',K_AFF,200,1850,2700,0,0,0,45,.70f,3000,1500},
 {'J',K_VAF,200,1850,2700,0,0,0,35,.50f,3000,1500},
};
#define NPH ((int)(sizeof(PH)/sizeof(PH[0])))
static const phone *find(char c){
 for(int i=0;i<NPH;i++)if(PH[i].c==c)return &PH[i];
 return 0;
}
/* the next vowel in the queue (within the next few sounds): the formants a hiss, a breath or an opening stop is
 * already heading for */
static void ahead(const bb_voice*v,float f[3]){
 f[0]=500;f[1]=1500;f[2]=2500;
 for(unsigned i=v->pos,n=0;i<v->used&&n<4;i++){
  const phone*p=find(v->queue[i]);
  if(!p){if(v->queue[i]!=' ')break;continue;}
  n++;
  if(p->kind==K_VOW||p->kind==K_DIP){f[0]=p->f1;f[1]=p->f2;f[2]=p->f3;return;}
 }
}

/* ----------------------------------------------------------------------------- the panel's side --- */
void bb_voice_init(bb_voice*v){
 /* (field by field: no library to copy or clear with) */
 for(int k=0;k<2;k++){v->slot[k][0]=0;v->slot_used[k]=0;v->slot_note[k]=-1;}
 v->request=v->seen=0;v->shift=0;v->queue[0]=0;v->used=v->pos=0;
 v->question=v->fading=v->talking=0;v->nseg=v->iseg=0;v->left=0;
 v->av=v->ah=v->af=v->accent=v->nz=0;
 v->ctl=0;v->phase=v->inc=v->flutter=0;v->hw1=v->hw2=0;v->out_gain=0;
 for(int i=0;i<5;i++)v->ra[i]=v->rb[i]=v->rc[i]=v->y1[i]=v->y2[i]=0;
 for(int i=0;i<BB_LEVELS;i++)v->level[i]=0;
 v->level_at=0;v->level_peak=0;v->level_n=0;
 v->noise=0xBABB1Eu;v->note=-1;
 v->pitch=.32f;v->rate=.52f;v->tone=.5f;v->breath=.18f;v->volume=.78f;v->stutter=0;v->inflect=.3f;
 v->f1=500;v->f2=1500;v->f3=2500;v->f0=120;v->ff=4000;v->fbw=2000;
 v->closing=.16f;v->inv_closing=1.f/.16f;v->peak=1;
}
void bb_voice_controls(bb_voice*v,const float k[7]){
 v->pitch=k[0];v->rate=k[1];v->tone=k[2];v->breath=k[3];
 v->volume=k[4];v->stutter=k[5];v->inflect=k[6];
}
/* (a private generator for STUTTER's doubling: the sound's own belongs to the other side) */
static uint32_t stutter_rng=0x51A77E5u;
static uint16_t put(char *q,uint16_t n,const char*s,float stutter){
 if(!s)return n;
 while(*s&&n<BB_QUEUE-3){
  char c=*s++;q[n++]=c;
  stutter_rng=stutter_rng*1664525u+1013904223u;
  if((stutter_rng>>24)<(unsigned)(stutter*50.f)&&n<BB_QUEUE-3)q[n++]=c;
 }
 return n;
}
static void hand_over(bb_voice*v,uint32_t req,uint16_t used,int note){
 v->slot[req&1u][used]=0;v->slot_used[req&1u]=used;v->slot_note[req&1u]=note;
 v->request=req;                         /* (last: from here the voice may take it) */
}
void bb_voice_say(bb_voice*v,const bb_mind*m,int note){
 uint32_t req=v->request+1u;
 char *q=v->slot[req&1u];
 uint16_t n=0;
 for(unsigned i=0;i<m->count;i++){
  const bb_token*t=&m->tok[i];
  if(t->role==BB_PUNCT){                 /* a comma is a pause; the last mark says how the pitch ends */
   if(t->text&&n<BB_QUEUE-3&&(t->text[0]==','||t->text[0]=='?'||t->text[0]=='.'))q[n++]=t->text[0];
  }else if(t->phones&&t->phones[0]){
   n=put(q,n,t->phones,v->stutter);
   if(n<BB_QUEUE-3)q[n++]=' ';
  }
 }
 hand_over(v,req,n,note);
}
void bb_voice_word(bb_voice*v,const bb_token*w,int note){
 uint32_t req=v->request+1u;
 hand_over(v,req,w?put(v->slot[req&1u],0,w->phones,v->stutter):0,note);
}
void bb_voice_transpose(bb_voice*v,int semis){v->shift=semis<-36?-36:semis>36?36:semis;}
void bb_voice_stop(bb_voice*v){uint32_t req=v->request+1u;hand_over(v,req,0,-1);}
int bb_voice_active(const bb_voice*v){
 uint32_t req=v->request;
 if(req!=v->seen)return v->slot_used[req&1u]>0;
 return v->talking;
}
float bb_voice_progress(const bb_voice*v){return v->used?clip((float)v->pos/(float)v->used,0,1):0.f;}
void bb_voice_levels(const bb_voice*v,float *out,int n){
 uint32_t at=v->level_at;
 if(n>BB_LEVELS)n=BB_LEVELS;
 for(int i=0;i<n;i++)out[i]=(float)v->level[(at+(uint32_t)(BB_LEVELS-n+i))%BB_LEVELS]*(1.f/255.f);
}

/* ----------------------------------------------------------------------------- the voice's side --- */
static bb_seg *stretch(bb_voice*v,float ms,float f1,float f2,float f3,float av,float ah,float af,float kf,float ka){
 bb_seg *s=&v->seg[v->nseg++];
 float scale=1.6f-v->rate;               /* RATE: slow .. fast */
 s->n=ms*scale*DUR/STEP_MS;
 if(s->n<3)s->n=3;
 s->nz=0;
 s->f1=f1;s->f2=f2;s->f3=f3;s->av=av;s->ah=ah;s->af=af;s->kf=kf*GLIDE;s->ka=ka;
 return s;
}
/* the next thing in the queue, as one to three stretches. 0: nothing left */
static int next_phone(bb_voice*v){
 float f[3];
 v->nseg=v->iseg=0;
 while(v->pos<v->used){
  char c=v->queue[v->pos++];
  const phone*p=find(c);
  if(c==' '){                            /* between words: a breath of quiet, and the next word's lift in pitch */
   stretch(v,34,v->f1,v->f2,v->f3,0,0,0,.1f,.22f);
   v->accent=1;
   return 1;
  }
  if(c==','){stretch(v,200,v->f1,v->f2,v->f3,0,0,0,.1f,.2f);return 1;}
  if(!p)continue;                        /* (the final mark, or a letter the table lacks) */
  ahead(v,f);
  switch(p->kind){
  case K_VOW:stretch(v,p->ms,p->f1,p->f2,p->f3,p->amp,0,0,.11f,.2f);break;
  case K_DIP:
   stretch(v,p->ms*.35f,p->f1,p->f2,p->f3,p->amp,0,0,.11f,.2f);
   stretch(v,p->ms*.65f,p->g1,p->g2,p->g3,p->amp,0,0,.035f,.2f);
   break;
  case K_GLI:stretch(v,p->ms,p->f1,p->f2,p->f3,p->amp,0,0,.07f,.2f);break;
  case K_NAS:stretch(v,p->ms,p->f1,p->f2,p->f3,p->amp,0,0,.2f,.25f)->nz=1;break;
  case K_FRI:v->ff=p->ff;v->fbw=p->fbw;stretch(v,p->ms,f[0],f[1],f[2],0,0,p->amp,.08f,.14f);break;
  case K_VFR:v->ff=p->ff;v->fbw=p->fbw;stretch(v,p->ms,p->f1,p->f2,p->f3,.55f,0,p->amp,.12f,.2f);break;
  case K_ASP:stretch(v,p->ms,f[0],f[1],f[2],0,p->amp,0,.2f,.2f);break;
  case K_STP:                            /* closed and silent; the burst; then a breath gliding into the vowel */
   v->ff=p->ff;v->fbw=p->fbw;
   stretch(v,p->ms,p->f1,p->f2,p->f3,0,0,0,.25f,.35f);
   stretch(v,10,p->f1,p->f2,p->f3,0,0,p->amp,.25f,.6f);
   stretch(v,24,f[0],.5f*(f[1]+p->f2),f[2],0,.45f,0,.1f,.3f);
   break;
  case K_VST:                            /* closed with a murmur; a smaller burst; the vowel does the gliding */
   v->ff=p->ff;v->fbw=p->fbw;
   stretch(v,p->ms,p->f1,p->f2,p->f3,.16f,0,0,.25f,.35f);
   stretch(v,8,p->f1,p->f2,p->f3,.3f,0,p->amp,.25f,.6f);
   break;
  case K_AFF:
   v->ff=p->ff;v->fbw=p->fbw;
   stretch(v,p->ms,p->f1,p->f2,p->f3,0,0,0,.25f,.35f);
   stretch(v,75,f[0],f[1],f[2],0,0,p->amp,.08f,.4f);
   break;
  default:
   v->ff=p->ff;v->fbw=p->fbw;
   stretch(v,p->ms,p->f1,p->f2,p->f3,.16f,0,0,.25f,.35f);
   stretch(v,55,p->f1,p->f2,p->f3,.5f,0,p->amp,.12f,.4f);
   break;
  }
  return 1;
 }
 return 0;
}
/* a resonator at f Hz, bw Hz wide, with unity gain at 0 Hz (Klatt's): y = a x + b y1 + c y2 */
static void resonator(bb_voice*v,int i,float f,float bw){
 float r=fm_exp2f(-4.5324f*bw/BB_SR);    /* exp(-pi bw / sr) */
 v->rc[i]=-r*r;
 v->rb[i]=2.f*r*fm_sin_turns(f/BB_SR+.25f);
 v->ra[i]=1.f-v->rb[i]-v->rc[i];
}
/* every CTL samples: what is said next taken over, the stretch stepped, the voice moved toward its targets, the
 * pitch and the resonators set for the samples to come */
static void control(bb_voice*v){
 uint32_t req=v->request;
 bb_seg *s;
 float base,goal,scale,prog,t;
 if(req!=v->seen){                       /* something new to say: what is sounding fades (4 steps, 6 ms), then it */
  if(v->talking&&v->fading<4){v->fading++;v->av*=.45f;v->ah*=.45f;v->af*=.45f;}
  else{
   uint16_t n=v->slot_used[req&1u];
   for(uint16_t i=0;i<=n;i++)v->queue[i]=v->slot[req&1u][i];
   v->used=n;v->pos=0;v->note=v->slot_note[req&1u];
   v->question=0;
   for(uint16_t i=0;i<n;i++)if(v->queue[i]=='?')v->question=1;
   v->seen=req;v->fading=0;v->nseg=v->iseg=0;v->left=0;v->accent=1;
   v->talking=n>0;
   if(!n)v->av=v->ah=v->af=0;
  }
 }
 if(v->talking&&v->seen==req){
  if(v->left<=0){
   if(++v->iseg>=v->nseg&&!next_phone(v)){
    /* said: the voice dies away, and then it is not talking */
    v->nseg=v->iseg=0;
    v->av*=.6f;v->ah*=.6f;v->af*=.6f;
    if(v->av<.002f&&v->ah<.002f&&v->af<.002f){v->av=v->ah=v->af=0;v->talking=0;}
   }else v->left=v->seg[v->iseg].n;
  }
  if(v->iseg<v->nseg){
   s=&v->seg[v->iseg];
   v->left-=1.f;
   v->f1+=(s->f1-v->f1)*s->kf;v->f2+=(s->f2-v->f2)*s->kf;v->f3+=(s->f3-v->f3)*s->kf;
   v->av+=(s->av-v->av)*s->ka;v->ah+=(s->ah-v->ah)*s->ka;v->af+=(s->af-v->af)*s->ka;
   v->nz+=(s->nz-v->nz)*.3f;
  }
 }
 /* the pitch: a key's, held; or the PITCH knob's, a little higher at the start than at the end, lifted on each word,
  * and over the last quarter falling, or for a question rising */
 base=v->note>=0?440.f*fm_exp2f((float)(v->note-69)*(1.f/12.f))
             :(95.f+165.f*v->pitch)*fm_exp2f((float)v->shift*(1.f/12.f));
 goal=base;
 if(v->note<0){
  prog=bb_voice_progress(v);
  t=clip((prog-.75f)*4.f,0,1);
  goal*=(1.05f-.10f*prog)*(1.f+.25f*v->inflect*v->accent);
  goal*=v->question?1.f+.6f*v->inflect*t*t:1.f-.3f*v->inflect*t;
 }
 v->accent*=.985f;
 v->flutter+=5.3f*(float)CTL/BB_SR;
 if(v->flutter>=1)v->flutter-=1;
 goal*=1.f+.006f*fm_sin_turns(v->flutter);
 v->f0+=(clip(goal,55.f,1100.f)-v->f0)*.2f;
 v->inc=v->f0/BB_SR;
 /* TONE: a darker, larger voice .. a brighter, smaller one (the formants moved together, the pulse's edge) */
 scale=.88f+.26f*v->tone;
 v->closing=.24f-.14f*v->tone;v->inv_closing=1.f/v->closing;
 v->peak=2.f*v->inv_closing;
 resonator(v,0,v->f1*scale,BW*(70.f+(v->f1<300.f?60.f:0.f)));          /* (a nasal's or a closure's low F1: wider, duller) */
 resonator(v,1,v->f2*scale,BW*95.f+NASAL*v->nz);      /* (a nasal: the upper formants damped to a murmur) */
 resonator(v,2,v->f3*scale,BW*140.f+NASAL*1.3f*v->nz);
 resonator(v,3,3500.f*scale,250.f);
 resonator(v,4,v->ff,v->fbw);
 v->ra[4]=(1.f+v->rc[4])*.5f;             /* (the hiss's: a band-pass, unity at its centre, nothing at 0 Hz) */
 v->out_gain=v->volume*1.5f;
}
float bb_voice_sample(bb_voice*v){
 float white,src,x,hiss,out,a;
 if(!v->ctl){
  v->ctl=CTL;
  if(v->talking||v->request!=v->seen)control(v);
 }
 v->ctl--;
 if(!v->talking){
  if(++v->level_n>=1024u){v->level_n=0;v->level[v->level_at%BB_LEVELS]=0;v->level_at=v->level_at+1u;v->level_peak=0;}
  return 0;
 }
 /* the glottal pulse, as the flow's derivative: a slow rise, a quicker fall to a sharp closing edge (TONE: how
  * sharp), then closed. The edge is band-limited (polyBLEP), so a high key's pitch does not alias */
 v->phase+=v->inc;
 if(v->phase>=1)v->phase-=1;
 x=v->phase;
 if(x<.40f){float u=x*2.5f;src=15.f*u*(1.f-u);}
 else if(x<.40f+v->closing){float u=(x-.40f)*v->inv_closing;src=-v->peak*u;}
 else src=0;
 {
  float edge=.40f+v->closing,d=x-edge;
  if(d>=0&&d<v->inc){float u=d/v->inc-1.f;src-=v->peak*.5f*u*u;}
  else if(d<0&&d>-v->inc){float u=d/v->inc+1.f;src+=v->peak*.5f*u*u;}
 }
 src*=1.f/12.5f;
 v->noise=v->noise*1664525u+1013904223u;
 white=(float)((v->noise>>8)&0xffffu)*(2.f/65535.f)-1.f;
 /* voiced: the pulse, and BREATH's noise in it; a breath (h, an opening stop): noise alone */
 x=src*v->av*(1.f-.4f*v->breath)+white*(v->ah*.35f+v->av*v->breath*.22f);
 for(int i=0;i<4;i++){                   /* F1..F4, one after another */
  float y=v->ra[i]*x+v->rb[i]*v->y1[i]+v->rc[i]*v->y2[i];
  v->y2[i]=v->y1[i];v->y1[i]=y;x=y;
 }
 /* a hiss or a burst: noise through its own resonator (less its low end), beside the cascade */
 hiss=v->ra[4]*(white-v->hw2)+v->rb[4]*v->y1[4]+v->rc[4]*v->y2[4];
 v->y2[4]=v->y1[4];v->y1[4]=hiss;v->hw2=v->hw1;v->hw1=white;
 out=(x*GAIN_VOICE+hiss*v->af*GAIN_HISS)*v->out_gain;
 out=out/(1.f+(out<0?-out:out)*.7f);      /* loud, and never past full scale */
 out=clip(out,-.98f,.98f);
 a=out<0?-out:out;
 if(a>v->level_peak)v->level_peak=a;
 if(++v->level_n>=1024u){
  v->level_n=0;
  v->level[v->level_at%BB_LEVELS]=(uint8_t)(v->level_peak*255.f);
  v->level_at=v->level_at+1u;v->level_peak=0;
 }
 return out;
}
