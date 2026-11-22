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
 puts("PALINDROME DSP smoke passed");
}
