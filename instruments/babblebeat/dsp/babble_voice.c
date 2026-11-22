/* SPDX-License-Identifier: MPL-2.0
 * Basic robotic allophone/formant voice. This is intentionally NOT full TTS:
 * a curated set of phonemized words, pulse/noise source, three resonators. */
#include "babble_voice.h"
#include "fastmath.h"
#define BB_SR 44100.f
static float clip(float x,float a,float b){return x<a?a:x>b?b:x;}
static void formant(bb_voice*v,int i,float hz,float r){
 v->coef[i]=2.f*r*fm_sin_turns(hz/BB_SR+.25f);
 v->r2[i]=r*r;v->input[i]=1.f-r;
}
void bb_voice_init(bb_voice*v){
 v->used=v->pos=0;v->remaining=v->total=0;v->noise=0xBABB1Eu;
 v->phase=0;v->freq=130;v->source_gain=v->noise_gain=v->noise_lp=0;
 v->pitch=.32f;v->rate=.52f;v->tone=.5f;v->breath=.18f;
 v->volume=.78f;v->stutter=0;v->inflect=.3f;v->note=-1;v->word_number=0;
 for(int i=0;i<3;i++){
  v->state[i][0]=v->state[i][1]=0;v->coef[i]=v->r2[i]=v->input[i]=0;
 }
}
void bb_voice_stop(bb_voice*v){v->used=v->pos=0;v->remaining=0;}
void bb_voice_controls(bb_voice*v,const float k[7]){
 v->pitch=k[0];v->rate=k[1];v->tone=k[2];v->breath=k[3];
 v->volume=k[4];v->stutter=k[5];v->inflect=k[6];
}
static void enqueue(bb_voice*v,const char*s){
 if(!s)return;
 while(*s&&v->used<BB_QUEUE-2){
  char c=*s++;v->queue[v->used++]=c;
  v->noise=v->noise*1664525u+1013904223u;
  if(c!=' '&&(v->noise>>24)<(unsigned)(v->stutter*50.f)&&v->used<BB_QUEUE-2)
   v->queue[v->used++]=c;
 }
}
static void prepare(bb_voice*v,int note){
 v->pos=0;v->remaining=0;v->note=note;v->word_number=0;
 v->queue[v->used]=0;
}
void bb_voice_say(bb_voice*v,const bb_mind*m,int note){
 v->used=0;
 for(unsigned i=0;i<m->count;i++)if(m->tok[i].phones&&m->tok[i].phones[0]){
  enqueue(v,m->tok[i].phones);
  if(v->used<BB_QUEUE-2)v->queue[v->used++]=' ';
 }
 prepare(v,note);
}
void bb_voice_word(bb_voice*v,const bb_token*w,int note){
 v->used=0;if(w)enqueue(v,w->phones);prepare(v,note);
}
int bb_voice_active(const bb_voice*v){return v->remaining>0||v->pos<v->used;}
static void new_phone(bb_voice*v){
 if(v->pos>=v->used){v->remaining=0;return;}
 char c=v->queue[v->pos++];
 if(c==' '){
  v->word_number++;v->remaining=(uint32_t)(1000.f*(1.3f-v->rate*.6f));
  v->total=v->remaining;v->source_gain=v->noise_gain=0;return;
 }
 float f1=500,f2=1600,f3=2800,ms=75,voiced=.8f,hiss=.16f;
 switch(c){
 case 'a':f1=780;f2=1240;f3=2700;ms=145;break;
 case 'A':f1=720;f2=1710;f3=2550;ms=145;break;
 case 'e':f1=570;f2=1860;f3=2650;ms=140;break;
 case 'i':f1=300;f2=2340;f3=3000;ms=155;break;
 case 'I':f1=390;f2=1950;f3=2620;ms=110;break;
 case 'o':f1=460;f2=900;f3=2550;ms=150;break;
 case 'O':f1=630;f2=1120;f3=2600;ms=140;break;
 case 'u':f1=340;f2=880;f3=2280;ms=150;break;
 case 'U':f1=520;f2=1450;f3=2450;ms=105;break;
 case 'E':f1=490;f2=1390;f3=1750;ms=125;break;
 case 'm':case 'n':case 'N':f1=350;f2=1050;f3=2200;ms=85;voiced=.65f;hiss=.05f;break;
 case 'l':case 'r':f1=420;f2=1250;f3=1950;ms=75;break;
 case 'w':case 'y':f1=410;f2=1700;f3=2700;ms=68;break;
 case 'v':case 'z':case 'D':f1=450;f2=2200;f3=3300;ms=75;voiced=.55f;hiss=.55f;break;
 case 'b':case 'd':case 'g':case 'J':f1=530;f2=1650;f3=2800;ms=55;voiced=.6f;hiss=.7f;break;
 case 's':case 'S':case 'f':case 'T':f1=700;f2=3100;f3=4150;ms=88;voiced=0;hiss=1;break;
 case 'p':case 't':case 'k':case 'C':f1=800;f2=2600;f3=3500;ms=48;voiced=0;hiss=1;break;
 case 'h':f1=580;f2=1500;f3=2700;ms=80;voiced=0;hiss=.75f;break;
 default:break;
 }
 v->source_gain=voiced;v->noise_gain=hiss;
 v->remaining=(uint32_t)(ms*(1.7f-v->rate*1.1f)*44.1f);
 if(v->remaining<120u)v->remaining=120u;
 v->total=v->remaining;
 float bend=(float)((v->word_number%5)-2)*v->inflect*.055f;
 v->freq=v->note>=0?440.f*fm_exp2f((float)(v->note-69)/12.f):(95.f+165.f*v->pitch);
 v->freq=clip(v->freq*(1.f+bend),65.f,1000.f);
 float bright=.8f+v->tone*.45f;
 formant(v,0,f1*bright,.971f);formant(v,1,f2*bright,.961f);formant(v,2,f3*bright,.944f);
}
float bb_voice_sample(bb_voice*v){
 if(!v->remaining){
  if(v->pos>=v->used)return 0;
  new_phone(v);
 }
 if(!v->remaining)return 0;
 v->remaining--;
 if(!v->source_gain&&!v->noise_gain)return 0;
 v->phase+=v->freq/BB_SR;if(v->phase>=1)v->phase-=1;
 float pulse=v->phase<.35f?(-.65f+v->phase*4.7f):(1.f-(v->phase-.35f)*2.3f);
 v->noise=v->noise*1664525u+1013904223u;
 float white=(float)((v->noise>>9)&65535u)*(2.f/65535.f)-1.f;
 v->noise_lp+=(white-v->noise_lp)*.08f;
 float source=(pulse*v->source_gain+(.2f+v->breath)*v->noise_gain*(white-v->noise_lp))*.4f;
 float s=0;
 for(int i=0;i<3;i++){
  float z=v->input[i]*source+v->coef[i]*v->state[i][0]-v->r2[i]*v->state[i][1];
  v->state[i][1]=v->state[i][0];v->state[i][0]=z;
  s+=z*(i==0?.48f:i==1?.37f:.21f);
 }
 float attack=(float)(v->total-v->remaining)/500.f;
 float release=(float)v->remaining/550.f;
 s*=clip(attack,0,1)*clip(release,0,1)*v->volume*.45f;
 return clip(s,-.85f,.85f);
}
