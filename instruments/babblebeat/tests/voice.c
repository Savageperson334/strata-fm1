/* SPDX-License-Identifier: MPL-2.0 */
/* BabbleBeat's voice: that it is loud, that a vowel's formants are where the table says, that a key's pitch is the
 * pitch, and that what to say can be changed at any moment without a click. */
#include "babble_lang.h"
#include "babble_voice.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define N 30000
static float buf[N];
static bb_voice v;
/* the power of buf[a..b) at f Hz (Goertzel) */
static double band(int a,int b,double f){
 double w=2*3.14159265358979*f/44100.,c=2*cos(w),s0,s1=0,s2=0;
 for(int i=a;i<b;i++){s0=buf[i]+c*s1-s2;s2=s1;s1=s0;}
 return (s1*s1+s2*s2-c*s1*s2)/((double)(b-a)*(b-a));
}
/* the power about f Hz: the strongest of the harmonics near it */
static double near(int a,int b,double f){
 double best=0;
 for(double g=f-150;g<=f+150;g+=25){double p=band(a,b,g);if(p>best)best=p;}
 return best;
}
static void render(int n){for(int i=0;i<n;i++){buf[i]=bb_voice_sample(&v);assert(isfinite(buf[i])&&fabsf(buf[i])<1.f);}}
static void say(const char *phones,int note){
 bb_token t={"x",phones,BB_NOUN};
 bb_voice_init(&v);bb_voice_word(&v,&t,note);
 assert(bb_voice_active(&v));            /* (at once: before the sound has taken it over) */
 render(N);
}
int main(void){
 bb_mind m;
 /* A sentence: it sounds, loudly, and it ends. */
 bb_init(&m,123);bb_generate(&m);bb_voice_init(&v);
 assert(!bb_voice_active(&v));bb_voice_say(&v,&m,-1);
 assert(bb_voice_active(&v));
 {double power=0;int loud=0,n=0;float peak=0,lev[BB_LEVELS],top=0;
  for(int i=0;i<44100*12;i++){
   float x=bb_voice_sample(&v);
   assert(isfinite(x)&&fabsf(x)<1.f);
   if(fabsf(x)>peak)peak=fabsf(x);
   if(bb_voice_active(&v)){power+=x*x;n++;if(fabsf(x)>.25f)loud++;}
   if(i==30000){bb_voice_levels(&v,lev,BB_LEVELS);for(int k=0;k<BB_LEVELS;k++)if(lev[k]>top)top=lev[k];}
  }
  assert(n>20000&&sqrt(power/n)>.15);    /* (the first voice's was .10 with half the peak) */
  assert(peak>.6f&&loud>2000);
  assert(top>.3f);                       /* the level the screen draws is the voice's own */
  assert(!bb_voice_active(&v));assert(bb_voice_progress(&v)>=.99f);}
 /* Vowels: ee has its second formant high (about 2250 Hz), oo has it low (about 900), ah in between (1120). */
 {double ee_hi,ee_lo,oo_hi,oo_lo,ah_mid,ee_mid;
  say("iiii",-1);ee_hi=near(8000,20000,2270);ee_lo=near(8000,20000,910);ee_mid=near(8000,20000,1130);
  say("uuuu",-1);oo_hi=near(8000,20000,2270);oo_lo=near(8000,20000,910);
  say("aaaa",-1);ah_mid=near(8000,20000,1130);
  if(getenv("BB_SHOW"))printf("ee hi/lo %.1f  oo lo/hi %.1f  ah/ee mid %.1f\n",ee_hi/ee_lo,oo_lo/oo_hi,ah_mid/ee_mid);
  assert(ee_hi>4*ee_lo);assert(oo_lo>20*oo_hi);assert(ah_mid>50*ee_mid);}
 /* A hiss is noise up high, not voice: s has far more above 5 kHz than at the voice's pitch. */
 {say("ssss",-1);if(getenv("BB_SHOW"))printf("s hi/lo %.1f\n",near(4000,14000,5800)/near(4000,14000,150));assert(near(4000,14000,5800)>30*near(4000,14000,150));}
 /* A word played from a key is at the key's pitch: A4, 440 Hz, and not a neighbour's. */
 {say("aaaa",69);assert(band(8000,20000,440)>30*band(8000,20000,415));assert(band(8000,20000,440)>30*band(8000,20000,466));}
 /* What to say next can come at any moment: the voice fades and takes it, with no jump in the sound; a stop is
  * silence within a few milliseconds. */
 {bb_token a={"a","aaaaaaaa",BB_NOUN},b={"b","iiiiiiii",BB_NOUN};float jump=0,prev=0;
  bb_voice_init(&v);bb_voice_word(&v,&a,-1);
  for(int i=0;i<60000;i++){
   float x=bb_voice_sample(&v);
   if(i%1777==0)bb_voice_word(&v,(i/1777)&1?&a:&b,(i/1777)%3?-1:60);
   if(fabsf(x-prev)>jump)jump=fabsf(x-prev);
   prev=x;
  }
  assert(jump<.45f);
  bb_voice_stop(&v);assert(!bb_voice_active(&v));
  for(int i=0;i<600;i++)prev=bb_voice_sample(&v);
  for(int i=0;i<2000;i++)assert(bb_voice_sample(&v)==0);}
 /* Every sound the lexicon spells is in the voice's table (a letter it lacked would be silently left out). */
 {static const char all[]="iIeAaOUVuEo12345wyrlmnNsSfTzvDhptkbdgCJ";
  bb_init(&m,5);m.grammar=1;m.damage=0;
  for(int th=0;th<4;th++){m.theme=(unsigned)th;
   for(int i=0;i<800;i++){bb_generate(&m);
    for(unsigned k=0;k<m.count;k++)for(const char*c=m.tok[k].phones;c&&*c;c++){
     int known=0;for(const char*q=all;*q;q++)if(*q==*c)known=1;
     if(!known){printf("no sound for '%c' in %s\n",*c,m.tok[k].text);return 1;}}}}}
 puts("BabbleBeat voice tests passed");
}
