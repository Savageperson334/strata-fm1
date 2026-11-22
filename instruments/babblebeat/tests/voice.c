/* SPDX-License-Identifier: MPL-2.0 */
#include "babble_lang.h"
#include "babble_voice.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
int main(void){
 bb_mind m;bb_voice v;bb_init(&m,123);bb_generate(&m);bb_voice_init(&v);
 assert(!bb_voice_active(&v));bb_voice_say(&v,&m,-1);
 assert(bb_voice_active(&v));
 double power=0;int nonzero=0;
 for(int i=0;i<44100*12;i++){
  float x=bb_voice_sample(&v);
  assert(isfinite(x)&&fabsf(x)<1.f);power+=x*x;
  if(fabsf(x)>.0002f)nonzero++;
 }
 assert(nonzero>1000&&power>1e-3);
 assert(!bb_voice_active(&v));
 const bb_token*w=bb_word_at(&m,0);assert(w);
 bb_voice_word(&v,w,60);assert(bb_voice_active(&v));
 bb_voice_stop(&v);assert(!bb_voice_active(&v));
 puts("BabbleBeat voice tests passed");
}
