/* SPDX-License-Identifier: MPL-2.0 */
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
int main(void){
 pm_init();assert(run(20)<.00001);
 pm_note(0,48);pm_note(1,55);
 assert(run(350)>.005);assert(pm_fill()==1);
 pm_record(PM_FREEZE);pm_note(0,-1);pm_note(1,-1);
 pm_set(G_MIX,1);pm_set(S_DENSITY,1);pm_grains(1);
 run(300);assert(pm_active_grains()>0);assert(run(200)>.001);
 for(int mode=0;mode<4;mode++){
  pm_record(mode);pm_note(0,36);
  for(int k=0;k<PM_PARAMS;k++){
   pm_set(k,0);run(20);pm_set(k,1);run(20);pm_set(k,.5f);
  }
 }
 pm_panic();assert(run(30)<.00001);
 pm_init();assert(pm_fill()==0);assert(run(20)<.00001);
 /* The recording's noise. NOISE alone fills the memory with hiss: no note played, yet the grains sound. */
 pm_set(R_NOISE,1);pm_set(G_MIX,1);pm_set(S_DENSITY,1);
 run(400);assert(pm_fill()==1);
 pm_record(PM_FREEZE);pm_grains(1);run(100);
 assert(run(200)>.003);
 /* GATE: with it up and nothing played, the head lifts and the hiss is not recorded. */
 pm_init();pm_set(R_NOISE,1);pm_set(R_GATE,1);pm_set(G_MIX,1);pm_set(S_DENSITY,1);
 run(400);assert(pm_writing()<.01f);
 pm_record(PM_FREEZE);pm_grains(1);run(100);
 assert(run(200)<.0005);
 /* ...and a note opens it. */
 pm_init();pm_set(R_GATE,.5f);pm_note(0,48);run(200);assert(pm_writing()>.9f);
 /* DROP all the way up: the head skips some stretches and writes others. */
 pm_set(R_GATE,0);pm_set(R_DROP,1);
 {int up=0,down=0;for(int k=0;k<400;k++){run(1);if(pm_writing()>.9f)up++;if(pm_writing()<.1f)down++;}
  assert(up>20&&down>20);}
 /* CRUSH: the memory holds only a few levels. */
 pm_init();pm_set(R_CRUSH,1);pm_note(0,48);run(400);
 {float lo[64],hi[64];int levels[9]={0},n=0;pm_ring(lo,hi,64);
  for(int k=0;k<64;k++){int a=(int)floorf(hi[k]*4.f+.5f)+4,b=(int)floorf(lo[k]*4.f+.5f)+4;
   assert(fabsf(hi[k]*4.f-floorf(hi[k]*4.f+.5f))<.01f);levels[a]=levels[b]=1;}
  for(int k=0;k<9;k++)n+=levels[k];assert(n>=2&&n<=9);}
 /* SILT's keys. A key played sounds its grains whether PLAY runs or not; CLUSTER starts that many together. */
 pm_init();pm_note(0,48);run(350);pm_record(PM_FREEZE);pm_note(0,-1);
 pm_set(G_MIX,1);pm_set(S_SIZE,1);pm_set(S_SPRAY,0);pm_set(S_REVERSE,0);run(200);
 assert(pm_active_grains()==0);
 pm_grain_pitch(0,0);pm_strike(0);run(1);assert(pm_active_grains()==2);assert(run(40)>.001);
 /* its pitch: an octave up reads the memory twice as fast */
 {float a0,a1,e,d0,d1;
  pm_panic();pm_grain_pitch(0,0);pm_strike(0);run(2);pm_grain(0,&a0,&e);run(1);pm_grain(0,&a1,&e);d0=a1-a0;if(d0<0)d0+=1;
  pm_panic();pm_grain_pitch(0,12);pm_strike(0);run(2);pm_grain(0,&a0,&e);run(1);pm_grain(0,&a1,&e);d1=a1-a0;if(d1<0)d1+=1;
  assert(d0>0&&fabsf(d1/d0-2.f)<.05f);
  /* two keys held and PLAY: the grains take them in turn, so both speeds are among them */
  pm_panic();run(60);pm_grain_pitch(0,0);pm_grain_pitch(1,12);pm_set(S_DENSITY,0);pm_grains(1);run(2);
  {float b0[PM_GRAINS],b1[PM_GRAINS];int slow=0,fast=0,n=pm_active_grains();
   for(int k=0;k<PM_GRAINS;k++)pm_grain(k,&b0[k],&e);
   run(1);
   for(int k=0;k<n;k++){float d;if(!pm_grain(k,&b1[k],&e))continue;d=b1[k]-b0[k];if(d<0)d+=1;
    if(fabsf(d/d0-1.f)<.05f)slow++;if(fabsf(d/d0-2.f)<.05f)fast++;}
   assert(n>=3&&slow>=1&&fast>=1);}
  pm_grains(0);}
 /* CLUSTER all the way up: four grains at a start (a key played starts twice) */
 pm_panic();pm_set(S_CLUSTER,1);run(60);pm_grain_pitch(0,0);pm_strike(0);run(1);assert(pm_active_grains()==8);
 /* SHAPE: struck, a grain is loud at once; a swell, it is still quiet a tenth of the way in */
 {float at,e;
  pm_panic();pm_set(S_CLUSTER,0);pm_set(S_SHAPE,0);run(60);pm_grain_pitch(0,0);pm_strike(0);run(8);
  assert(pm_grain(0,&at,&e)&&e>.8f);
  pm_panic();pm_set(S_SHAPE,1);run(60);pm_grain_pitch(0,0);pm_strike(0);run(8);
  assert(pm_grain(0,&at,&e)&&e<.1f);}
 /* never more grains than there is room for, however hard they are asked for */
 pm_panic();pm_set(S_CLUSTER,1);pm_set(S_DENSITY,1);pm_set(S_TIMING,1);pm_set(S_VARY,1);pm_set(S_SCATTER,1);
 pm_set(S_DETUNE,1);pm_grain_pitch(0,-12);pm_grain_pitch(1,7);pm_grain_pitch(2,19);pm_grains(1);
 for(int k=0;k<400;k++){run(1);assert(pm_active_grains()<=PM_GRAINS);}
 assert(pm_active_grains()>PM_GRAINS/2);
 puts("PALINDROME DSP smoke passed");
}
