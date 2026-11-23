/* SPDX-License-Identifier: MPL-2.0 */
/* PALINDROME's engine: the rings (a capture makes a new one and touches no other), the grains, the keys. */
#include "palindrome.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static float l[256],r[256];
static double run(int blocks){
 double sq=0;
 for(int b=0;b<blocks;b++){
  pm_render(l,r,256);
  for(int i=0;i<256;i++){
   assert(isfinite(l[i])&&isfinite(r[i]));
   assert(fabsf(l[i])<=1&&fabsf(r[i])<=1);
   sq+=l[i]*l[i]+r[i]*r[i];
  }
 }
 return sqrt(sq/(blocks*512.0));
}
/* a capture from start to what pm_service says of it (the event), and where it went */
static int capture(int source,int *slot){
 int ev=PM_EV_NONE,guard=0;
 assert(pm_capture(source)==0);
 assert(pm_capture_state()==PM_CAP_RECORDING);
 while(!(ev=pm_service(slot))){run(1);assert(++guard<400);}
 assert(pm_capture_state()==PM_CAP_IDLE);
 return ev;
}
/* a ring's sound, as a number that changes if any of it does */
static double sum_of(int slot){
 float lo[256],hi[256];double s=0;
 pm_ring_wave(slot,lo,hi,256);
 for(int k=0;k<256;k++)s+=(k+1)*(double)(hi[k]-lo[k])+(k+3)*(double)hi[k];
 return s;
}
int main(void){
 int slot=-1,id,parent,source;
 /* Nothing captured: no rings, silence, and PLAY has nothing to play. */
 pm_init();pm_set(M_SUSTAIN,1);assert(run(20)<.00001);
 assert(pm_ring_count()==0&&pm_ring_selected()==-1);
 pm_set(G_MIX,1);pm_grains(1);run(40);assert(pm_active_grains()==0);pm_grains(0);
 assert(pm_capture(PM_SRC_SILT)==PM_ERR_NOSRC);
 assert(pm_ring_select(0)==-1&&pm_ring_delete(0)==-1);
 /* MORPH alone with nothing played: there is nothing to record, and no ring is made. */
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_SILENT);assert(pm_ring_count()==0);

 /* RING 01, from MORPH: a finite capture that finishes by itself. Until then there is no ring; a second REC makes
  * no second one. */
 pm_note(0,48);pm_note(1,55);assert(run(60)>.005);
 assert(pm_capture(PM_SRC_MORPH)==0);
 run(100);
 assert(pm_capture_state()==PM_CAP_RECORDING&&pm_capture_progress()>.3f&&pm_capture_progress()<.5f);
 assert(pm_capture(PM_SRC_MORPH)==PM_ERR_BUSY&&pm_capture(PM_SRC_SILT)==PM_ERR_BUSY);
 assert(pm_ring_count()==0&&pm_ring_selected()==-1);
 {float lo[8],hi[8];pm_ring_wave(pm_capture_slot(),lo,hi,8);assert(hi[0]>=lo[0]&&hi[7]<lo[7]);}   /* (filling) */
 {int ev=0,guard=0;while(!(ev=pm_service(&slot))){run(1);assert(++guard<300);}assert(ev==PM_EV_COMMITTED);}
 assert(pm_ring_count()==1&&pm_ring_selected()==slot);
 assert(pm_ring_info(slot,&id,&parent,&source)&&id==1&&parent==0&&source==PM_SRC_MORPH);
 int r1=slot;double sum1=sum_of(r1);
 assert(sum1!=0);
 /* its level is evened: the loudest part is near .7 whatever MORPH's level was */
 {float lo[64],hi[64],top=0;pm_ring_wave(r1,lo,hi,64);
  for(int k=0;k<64;k++){if(hi[k]>top)top=hi[k];if(-lo[k]>top)top=-lo[k];}assert(top>.5f&&top<.75f);}

 /* SILT plays it: grains, each on its ring. */
 pm_set(S_DENSITY,1);pm_grains(1);run(100);
 assert(pm_active_grains()>0);assert(run(100)>.002);
 {float at,e;int gs=-1,any=0;for(int k=0;k<PM_GRAINS;k++)if(pm_grain(k,&at,&e,&gs)){any=1;assert(gs==r1&&at>=0&&at<1);}assert(any);}

 /* SILT's source is the grains alone. MORPH still sounds, but with the grains stopped there is nothing of SILT's
  * to record: dropped, not a ring of MORPH by accident. */
 pm_grains(0);run(200);assert(pm_active_grains()==0);
 assert(capture(PM_SRC_SILT,&slot)==PM_EV_SILENT);assert(pm_ring_count()==1);
 /* RING 02, from SILT's grains of RING 01, changed on the way (an octave up): a new ring, its parent 01, and 01 as
  * it was. Its own PITCH starts level again, so the octave is not applied twice. */
 pm_set(S_PITCH,.75f);pm_set(S_SIZE,.6f);pm_grains(1);run(120);
 assert(capture(PM_SRC_SILT,&slot)==PM_EV_COMMITTED);
 int r2=slot;assert(r2!=r1&&pm_ring_count()==2&&pm_ring_selected()==r2);
 assert(fabsf(pm_get(S_WEAVE))<1e-6f); /* the printed mix starts solo, no ancestors doubled */
 assert(pm_ring_info(r2,&id,&parent,&source)&&id==2&&parent==1&&source==PM_SRC_SILT);
 assert(sum_of(r1)==sum1);
 double sum2=sum_of(r2);assert(sum2!=sum1);
 assert(fabsf(pm_get(S_PITCH)-.5f)<1e-6f&&fabsf(pm_get(S_SIZE)-.6f)<1e-6f);

 /* Each ring keeps its own settings: 01 has the PITCH and SIZE it was left with, 02 its own. */
 pm_set(S_SIZE,.9f);
 assert(pm_ring_select(r1)==0&&pm_ring_selected()==r1);
 assert(fabsf(pm_get(S_PITCH)-.75f)<1e-6f&&fabsf(pm_get(S_SIZE)-.6f)<1e-6f);
 assert(pm_ring_select(r2)==0);assert(fabsf(pm_get(S_SIZE)-.9f)<1e-6f);
 /* Changing ring while grains sound: those sounding stay on the ring they started on. */
 pm_set(S_SIZE,1);run(60);
 {float at,e;int gs,on2=0,on1=0;
  assert(pm_ring_select(r1)==0);run(4);
  for(int k=0;k<PM_GRAINS;k++)if(pm_grain(k,&at,&e,&gs)){if(gs==r2)on2++;if(gs==r1)on1++;}
  assert(on2>0&&on1>0);
  for(int k=0;k<40;k++){pm_ring_select(k&1?r1:r2);run(3);}}
 assert(sum_of(r1)==sum1&&sum_of(r2)==sum2);

 /* Independent ring scheduling: WEAVE mixes two preserved sources at bounded
  * density while each ring keeps its own read position and playback mode. */
 assert(pm_ring_select(r2)==0);
 pm_set(S_DENSITY,1);pm_set(S_WEAVE,1);pm_set(S_MODE,0);
 run(270);
 assert(pm_grains_on_ring(r1)>0&&pm_grains_on_ring(r2)>0);
 for(int mode=0;mode<4;mode++){
  pm_set(S_MODE,(float)mode/3.f);run(170);
  assert(pm_active_grains()<=PM_GRAINS);
 }
 pm_set(S_WEAVE,0);pm_set(S_MODE,.375f);run(160);
 /* Manual SCAN and automatic FLOW: both move the read head without changing audio PCM. */
 assert(pm_ring_select(r1)==0);
 pm_set(S_LOOP,0);pm_set(G_SCAN,.5f);pm_set(S_POS,.15f);run(100);
 float scan_a=pm_position();run(800);float scan_b=pm_position();
 assert(fabsf(scan_a-scan_b)>.002f);
 pm_set(S_POS,.85f);run(50);assert(pm_position()>.5f);
 pm_set(S_LOOP,.15f);pm_set(G_SCAN,.18f);run(50);
 assert(sum_of(r1)==sum1&&sum_of(r2)==sum2);
 /* A branch: back on 01, changed again and captured: RING 03's parent is 01, and 02 is untouched. */
 assert(pm_ring_select(r1)==0);pm_set(S_REVERSE,1);run(60);
 assert(capture(PM_SRC_SILT,&slot)==PM_EV_COMMITTED);
 int r3=slot;assert(pm_ring_count()==3&&pm_ring_info(r3,&id,&parent,&source)&&id==3&&parent==1);
 assert(sum_of(r1)==sum1&&sum_of(r2)==sum2);
 double sum3=sum_of(r3);
 /* Graduated WEAVE: at 0 the selected ring plays solo; halfway it brings
  * the previous parent ring; at 100 it includes all three. */
 pm_set(S_WEAVE,0);run(100);
 assert(pm_ring_mix_weight(r3)>.99f&&pm_ring_mix_weight(r1)<.001f&&pm_ring_mix_weight(r2)<.001f);
 pm_set(S_WEAVE,.5f);run(100);
 assert(pm_ring_mix_weight(r1)>.30f&&pm_ring_mix_weight(r2)<.001f);
 pm_set(S_WEAVE,1);run(100);
 assert(pm_ring_mix_weight(r1)>.15f&&pm_ring_mix_weight(r2)>.15f&&pm_ring_mix_weight(r3)>.15f);
 /* Fresh generations start solo. Reset WEAVE before continuing old replacement tests. */
 pm_set(S_WEAVE,0);run(80);
 /* Full: the oldest ring, the first recording, makes room for the new one. The others are as they were; the new
  * ring, 04, has 01's place. */
 assert(pm_ring_selected()==r3);
 assert(capture(PM_SRC_SILT,&slot)==PM_EV_COMMITTED);
 assert(pm_capture_dropped()==1&&slot==r1&&pm_ring_count()==3);
 assert(pm_ring_info(slot,&id,&parent,&source)&&id==4&&parent==3);
 assert(sum_of(r2)==sum2&&sum_of(r3)==sum3);
 /* ...but never the selected ring, which the capture reads while it writes: with the oldest (02) selected, it is
  * the next oldest, 03, that goes. */
 assert(pm_ring_select(r2)==0);run(60);
 assert(capture(PM_SRC_SILT,&slot)==PM_EV_COMMITTED);
 assert(pm_capture_dropped()==3&&slot==r3&&pm_ring_count()==3);
 assert(pm_ring_info(slot,&id,&parent,&source)&&id==5&&parent==2);
 assert(sum_of(r2)==sum2);
 /* A ring deleted on purpose: its grains fade, the others stay, and the next capture takes the room with a new
  * number and gives nothing up. */
 sum1=sum_of(r1);sum3=sum_of(r3);
 assert(pm_ring_select(r2)==0);run(40);assert(pm_ring_delete(r2)==0);
 assert(pm_ring_count()==2&&pm_ring_selected()==r3);          /* (the newest left: 05) */
 run(4);{float at,e;int gs;for(int k=0;k<PM_GRAINS;k++)if(pm_grain(k,&at,&e,&gs))assert(gs!=r2);}
 assert(!pm_ring_info(r2,&id,&parent,&source));
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
 assert(pm_capture_dropped()==0&&slot==r2&&pm_ring_info(slot,&id,&parent,&source)&&id==6&&parent==0);
 assert(pm_ring_info(r3,&id,&parent,&source)&&id==5&&parent==2);
 assert(sum_of(r1)==sum1&&sum_of(r3)==sum3);
 /* A panic in the middle of a capture: the capture is gone, the rings are not. */
 assert(pm_ring_delete(r2)==0);pm_note(0,48);run(30);
 assert(pm_capture(PM_SRC_MORPH)==0);run(50);pm_panic();run(10);
 assert(pm_capture_state()==PM_CAP_IDLE&&pm_service(&slot)==PM_EV_NONE&&pm_ring_count()==2);
 assert(sum_of(r1)==sum1&&sum_of(r3)==sum3);
 /* ...and so is a capture when every ring is cleared. */
 pm_note(0,48);run(30);assert(pm_capture(PM_SRC_MORPH)==0);run(50);pm_ring_clear();run(4);
 assert(pm_capture_state()==PM_CAP_IDLE&&pm_ring_count()==0&&pm_ring_selected()==-1);
 run(300);assert(pm_service(&slot)==PM_EV_NONE&&pm_ring_count()==0);

 /* Every control at both ends, with rings and without, capturing and not. */
 for(int pass=0;pass<2;pass++){
  pm_note(0,36);pm_grains(1);
  for(int k=0;k<PM_PARAMS;k++){
   pm_set(k,0);run(20);pm_set(k,1);run(20);pm_set(k,.5f);
   if(k==8)pm_capture(pass?PM_SRC_SILT:PM_SRC_MORPH);
   pm_service(&slot);
  }
 }
 pm_panic();assert(run(30)<.00001);

 /* What a capture does to its sound. NOISE alone: no note played, yet there is a ring of hiss, and it plays. */
 pm_init();pm_set(M_SUSTAIN,1);pm_set(R_NOISE,1);pm_set(G_MIX,1);pm_set(S_DENSITY,1);
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
 pm_grains(1);run(100);assert(run(200)>.003);
 /* GATE: with it up and nothing played, the hiss is kept out, so there is nothing to keep. */
 pm_init();pm_set(M_SUSTAIN,1);pm_set(R_NOISE,1);pm_set(R_GATE,1);
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_SILENT);
 /* ...and a note opens it. */
 pm_set(R_GATE,.5f);pm_set(R_NOISE,0);pm_note(0,48);run(100);
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
 /* DROP all the way up: stretches of the ring are left out, others are there. */
 pm_init();pm_set(M_SUSTAIN,1);pm_set(R_DROP,1);pm_note(0,48);run(100);
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
 {float lo[128],hi[128];int gaps=0,sound=0;pm_ring_wave(slot,lo,hi,128);
  for(int k=1;k<127;k++){if(hi[k]-lo[k]<.01f)gaps++;if(hi[k]-lo[k]>.2f)sound++;}
  assert(gaps>8&&sound>8);}
 /* CRUSH: the ring holds only a few levels. */
 pm_init();pm_set(M_SUSTAIN,1);pm_set(R_CRUSH,1);pm_note(0,48);run(100);
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
 {float lo[128],hi[128],seen[16];int n=0;pm_ring_wave(slot,lo,hi,128);
  for(int k=2;k<126;k++)for(int e=0;e<2;e++){float v=e?hi[k]:lo[k];int j;
   for(j=0;j<n;j++)if(fabsf(seen[j]-v)<.004f)break;
   if(j==n){assert(n<16);seen[n++]=v;}}
  assert(n>=2&&n<=9);}
 /* CARRY: the selected ring copied under the next capture. With nothing else to record, the new ring is the old
  * one again (and without CARRY there would have been nothing). */
 pm_init();pm_set(M_SUSTAIN,1);pm_note(0,48);run(100);assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
 pm_note(0,-1);run(600);
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_SILENT);
 pm_set(R_CARRY,1);run(40);
 {double a=sum_of(pm_ring_selected());
  assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
  assert(pm_ring_count()==2&&fabs(sum_of(slot)-a)<fabs(a)*.05);}
 /* a ring's two ends are eased to nothing, so the loop closes without a click */
 {float lo[4096],hi[4096];pm_ring_wave(slot,lo,hi,4096);
  assert(hi[0]-lo[0]<.02f&&hi[4095]-lo[4095]<.02f);}

 /* SILT's keys. A key played sounds its grains whether PLAY runs or not; CLUSTER starts that many together. */
 pm_init();pm_set(M_SUSTAIN,1);pm_note(0,48);run(100);assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);pm_note(0,-1);
 pm_set(G_MIX,1);pm_set(S_SIZE,1);pm_set(S_SPRAY,0);pm_set(S_REVERSE,0);run(200);
 assert(pm_active_grains()==0);
 pm_grain_pitch(0,0);pm_strike(0);run(1);assert(pm_active_grains()==2);assert(run(40)>.001);
 /* its pitch: an octave up reads the ring twice as fast */
 {float a0,a1,e,d0,d1;
  pm_panic();pm_grain_pitch(0,0);pm_strike(0);run(2);pm_grain(0,&a0,&e,0);run(1);pm_grain(0,&a1,&e,0);d0=a1-a0;if(d0<0)d0+=1;
  pm_panic();pm_grain_pitch(0,12);pm_strike(0);run(2);pm_grain(0,&a0,&e,0);run(1);pm_grain(0,&a1,&e,0);d1=a1-a0;if(d1<0)d1+=1;
  assert(d0>0&&fabsf(d1/d0-2.f)<.05f);
  /* two keys held and PLAY: the grains take them in turn, so both speeds are among them */
  pm_panic();run(60);pm_grain_pitch(0,0);pm_grain_pitch(1,12);pm_set(S_DENSITY,0);pm_grains(1);run(2);
  {float b0[PM_GRAINS],b1[PM_GRAINS];int slow=0,fast=0,n=pm_active_grains();
   for(int k=0;k<PM_GRAINS;k++)pm_grain(k,&b0[k],&e,0);
   run(1);
   for(int k=0;k<n;k++){float d;if(!pm_grain(k,&b1[k],&e,0))continue;d=b1[k]-b0[k];if(d<0)d+=1;
    if(fabsf(d/d0-1.f)<.05f)slow++;if(fabsf(d/d0-2.f)<.05f)fast++;}
   assert(n>=3&&slow>=1&&fast>=1);}
  pm_grains(0);}
 /* CLUSTER all the way up: four grains at a start (a key played starts twice) */
 pm_panic();pm_set(S_CLUSTER,1);run(60);pm_grain_pitch(0,0);pm_strike(0);run(1);assert(pm_active_grains()==8);
 /* SHAPE: struck, a grain is loud at once; a swell, it is still quiet a tenth of the way in */
 {float at,e;
  pm_panic();pm_set(S_CLUSTER,0);pm_set(S_SHAPE,0);run(60);pm_grain_pitch(0,0);pm_strike(0);run(8);
  assert(pm_grain(0,&at,&e,0)&&e>.8f);
  pm_panic();pm_set(S_SHAPE,1);run(60);pm_grain_pitch(0,0);pm_strike(0);run(8);
  assert(pm_grain(0,&at,&e,0)&&e<.1f);}
 /* never more grains than there is room for, however hard they are asked for, backward ones and the ring's seam
  * included */
 pm_panic();pm_set(S_CLUSTER,1);pm_set(S_DENSITY,1);pm_set(S_TIMING,1);pm_set(S_VARY,1);pm_set(S_SCATTER,1);
 pm_set(S_DETUNE,1);pm_set(S_REVERSE,.5f);pm_set(S_SPRAY,1);pm_set(S_POS,1);
 pm_grain_pitch(0,-12);pm_grain_pitch(1,7);pm_grain_pitch(2,19);pm_grains(1);
 for(int k=0;k<400;k++){run(1);assert(pm_active_grains()<=PM_GRAINS);}
 assert(pm_active_grains()>PM_GRAINS/2);
 /* DECAY. At its top a MORPH note is held; under it a struck note dies away by itself, and a note struck again
  * (the same key) sounds again. MORPH's own page hears MORPH whatever MIX says. */
 pm_init();pm_set(M_SUSTAIN,1);pm_note(0,57);run(400);assert(pm_voice_sounding(0)&&run(100)>.01);
 pm_init();pm_set(M_SUSTAIN,.3f);pm_note(0,57);run(20);
 {double early=run(40),late;assert(early>.01&&pm_voice_sounding(0));
  run(600);late=run(60);assert(late<early*.02&&!pm_voice_sounding(0));
  pm_note(0,57);run(20);assert(pm_voice_sounding(0)&&run(40)>early*.5);}
 pm_init();pm_set(M_SUSTAIN,1);pm_note(0,48);run(100);assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
 pm_set(S_DENSITY,1);pm_grains(1);run(200);
 {double both,morph_alone,mon;
  assert(fabsf(pm_get(G_MIX)-1.f)<1e-6f);                 /* (as it starts: while grains sound, the grains are heard) */
  both=run(200);pm_note(0,-1);run(600);                    /* MORPH let go: nothing of the sound changes much */
  assert(fabs(run(200)-both)<both*.35);
  pm_grains(0);run(400);pm_note(0,48);run(200);morph_alone=run(100);assert(morph_alone>.01);   /* no grains: MORPH */
  pm_grains(1);run(300);pm_monitor(1);run(300);mon=run(100);pm_monitor(0);
  assert(mon>.005);}
 /* Generations (DRIFT). The loop made again from itself, the ring it plays kept under the new one: forty of them
  * neither swell nor die away, nor sink into a hum; with most of the ring kept each is nearly the one before, with
  * little kept it is not; and the play point goes round the ring by itself (LOOP) or stays (LOOP 0). */
 {static int16_t before[PM_BUFFER];
  float keeps[2]={.88f,.15f};
  for(int k=0;k<2;k++){
   int slot=-1,ev,guard;double ab=0,aa=0,bb=0,e=0;int z=0;const int16_t *a;
   pm_init();pm_note(0,60);pm_note(1,67);
   assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);
   pm_grains(1);run(300);
   for(int g=0;g<40;g++){
    a=pm_ring_pcm(pm_ring_selected());assert(a);
    for(int i=0;i<(int)PM_BUFFER;i++)before[i]=a[i];
    assert(pm_capture_keep(PM_SRC_SILT,keeps[k])==0);
    ev=PM_EV_NONE;guard=0;
    while(!(ev=pm_service(&slot))){run(1);assert(++guard<400);}
    assert(ev==PM_EV_COMMITTED);
    run(200);
   }
   assert(pm_ring_count()==PM_RINGS);
   a=pm_ring_pcm(pm_ring_selected());
   for(int i=0;i<(int)PM_BUFFER;i++){
    ab+=(double)a[i]*before[i];aa+=(double)a[i]*a[i];bb+=(double)before[i]*before[i];
    e+=(double)a[i]*a[i];if(i)z+=(a[i-1]<0)!=(a[i]<0);
   }
   e=sqrt(e/PM_BUFFER)/32768.;
   assert(e>.12&&e<.3);                                    /* as loud as the first */
   assert(z>(int)PM_BUFFER/100&&z<(int)PM_BUFFER/8);        /* still notes: not a hum, not a hiss */
   if(k==0)assert(ab/sqrt(aa*bb)>.95);else assert(ab/sqrt(aa*bb)<.8);
  }
  {float p0,p1;
   pm_set(S_LOOP,.5f);run(4);p0=pm_position();run(40);p1=pm_position();assert(p0!=p1);
   pm_set(S_LOOP,0);run(40);p0=pm_position();run(40);p1=pm_position();assert(p0==p1);}}
 /* LOCK protects full rings from recycling, while the selected ring remains
  * a capture source regardless of lock state. */
 pm_init();pm_set(M_SUSTAIN,1);pm_note(0,48);run(30);
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);int lock1=slot;
 assert(pm_ring_lock(lock1,1)==0&&pm_ring_locked(lock1));
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);int lock2=slot;
 assert(pm_ring_lock(lock2,1)==0&&pm_ring_locked(lock2));
 assert(capture(PM_SRC_MORPH,&slot)==PM_EV_COMMITTED);int active_slot=slot;
 assert(pm_capture(PM_SRC_SILT)==PM_ERR_FULL&&pm_ring_count()==3);
 assert(pm_ring_lock(lock1,0)==0);
 pm_grains(1);run(100);
 assert(capture(PM_SRC_SILT,&slot)==PM_EV_COMMITTED);
 assert(slot==lock1&&pm_ring_locked(lock2)&&pm_ring_count()==3);
 assert(pm_ring_info(active_slot,&id,&parent,&source));
 puts("PALINDROME DSP smoke passed");
}
