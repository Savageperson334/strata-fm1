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
 puts("PALINDROME DSP smoke passed");
}
