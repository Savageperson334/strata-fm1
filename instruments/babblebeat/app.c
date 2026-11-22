/* SPDX-License-Identifier: MPL-2.0
 * BabbleBeat / third FM-1 firmware. Single-press FX: voice; EDIT: words.
 * PLAY autonomous babbling; SEQ new phrase; ARP mutate; REC remember noun.
 * Note key: speak the next word at the key's pitch. */
#include "babble_lang.h"
#include "babble_voice.h"
static bb_mind mind;
static bb_voice mouth;
static uint8_t page,autoplay;
static int8_t octave;
static uint32_t old_keys,old_buttons,frames,last_spoken;
static float gain_now;
static int cursor;
static char message[36];
static uint32_t message_until;
static int touched=-1;
static uint32_t touched_at;
static const int ENC[7]={EN_K1,EN_K2,EN_K3,EN_K4,EN_SELECT,EN_ALGO,EN_PRESET};
static const float DEFAULTS[2][7]={
 {.32f,.52f,.50f,.18f,.78f,0.f,.30f}, /* Voice */
 {.85f,.25f,.35f,.15f,0.f,.40f,.50f}  /* Words */
};
static float params[2][7];
static const char *const NAMES[2][7]={
 {"PITCH","RATE","TONE","BREATH","LEVEL","STUTTER","INFLECT"},
 {"GRAMMAR","WANDER","RECALL","DAMAGE","WORLD","LENGTH","STYLE"}
};
static int32_t bb_yoff;
static void line(int32_t x,int32_t y,int32_t xx,int32_t yy,uint16_t c){
 if((y<bb_yoff&&yy<bb_yoff)||(y>=bb_yoff+120&&yy>=bb_yoff+120))return;
 cv_line(x,y-bb_yoff,xx,yy-bb_yoff,c);
}
static void rect(int x,int y,int w,int h,uint16_t c){cv_rect(x,y-bb_yoff,w,h,c);}
static int label(int x,int y,const char*s,uint16_t c){
 if(y<bb_yoff||y+14>bb_yoff+120)return x+text_w(&FONT_XS,s);
 return cv_text(x,y-bb_yoff,&FONT_XS,s,c);
}
static void apply(void){
 bb_voice_controls(&mouth,params[0]);
 mind.grammar=params[1][0];mind.wander=params[1][1];
 mind.recall=params[1][2];mind.damage=params[1][3];
 mind.theme=(unsigned)(params[1][4]*3.999f);
 mind.length=params[1][5];mind.style=params[1][6];
}
static void speak(void){bb_voice_say(&mouth,&mind,-1);last_spoken=plat_ms();}
static void fresh(void){bb_generate(&mind);cursor=0;speak();}
void app_init(void){
 bb_init(&mind,0xBABB1E25u);bb_voice_init(&mouth);
 for(int p=0;p<2;p++)for(int k=0;k<7;k++)params[p][k]=DEFAULTS[p][k];
 apply();bb_generate(&mind);
 page=1;autoplay=0;octave=0;old_keys=old_buttons=frames=last_spoken=0;
 gain_now=0;cursor=0;message_until=0;touched=-1;
}
void app_panic(void){bb_voice_stop(&mouth);autoplay=0;}
void app_render(int32_t *lr,uint32_t n,uint32_t master_q12){
 float target=(float)master_q12*(8300000.f/4096.f);
 for(uint32_t i=0;i<n;i++){
  gain_now+=(target-gain_now)*.004f;
  int32_t s=(int32_t)(bb_voice_sample(&mouth)*gain_now);
  *lr++=s;*lr++=s;
 }
}
void app_say(const char*a,const char*b){
 int n=0;
 while(a&&*a&&n<33)message[n++]=*a++;
 if(b&&*b&&n<32)message[n++]=' ';
 while(b&&*b&&n<33)message[n++]=*b++;
 message[n]=0;message_until=plat_ms()+1300u;
}
void app_input_only(void){
 uint32_t btn=plat_buttons(),keys=plat_keys();
 uint32_t nb=btn&~old_buttons,nk=keys&~old_keys,now=plat_ms();
 if(nb&(1u<<B_FX))page=0;
 if(nb&(1u<<B_EDIT))page=1;
 if(nb&(1u<<B_PLAY)){
  autoplay=!autoplay;
  if(autoplay)fresh();else bb_voice_stop(&mouth);
 }
 if(nb&(1u<<B_SEQ))fresh();
 if(nb&(1u<<B_ARP)){bb_mutate(&mind);cursor=0;speak();}
 if(nb&(1u<<B_REC)){
  bb_remember(&mind);
  app_say("REMEMBERED",mind.has_memory?mind.remembered.text:"NONE");
 }
 if(nb&(1u<<B_HOME)){autoplay=0;bb_voice_stop(&mouth);app_say("STOPPED","");}
 if(nb&(1u<<B_SAVE))app_say("PATCH SAVE","NOT YET");
 if((nb&(1u<<B_OCTDN))&&octave>-2)octave--;
 if((nb&(1u<<B_OCTUP))&&octave<2)octave++;
 for(int j=0;j<NKEYS;j++)if(nk&(1u<<j)){
  int n=bb_words(&mind);
  if(n){
   const bb_token*w=bb_word_at(&mind,cursor%n);
   cursor++;bb_voice_word(&mouth,w,48+j+12*octave);last_spoken=now;
  }
 }
 for(int k=0;k<7;k++){
  int delta=plat_enc(ENC[k]);
  if(!delta)continue;
  float v=params[page][k]+(float)delta/64.f;
  params[page][k]=v<0?0:v>1?1:v;
  touched=k;touched_at=now;apply();
 }
 old_buttons=btn;old_keys=keys;
 if(autoplay&&!bb_voice_active(&mouth)&&now-last_spoken>350u)fresh();
 uint32_t leds=(1u<<(page?B_EDIT:B_FX))|(autoplay?LED_PLAY_GREEN:0)|
  (mind.has_memory?(1u<<B_REC):0);
 plat_lights(leds,0,(1u<<B_FX)|(1u<<B_EDIT)|(1u<<B_PLAY)|
  (1u<<B_SEQ)|(1u<<B_ARP)|(1u<<B_REC),0,1);
}
static void knob(int col,int row,const char*name,float value,int lit){
 int x=col*60,y=200+row*20;
 uint16_t color=lit?C_WHITE:page?C_HI:C_AMB;
 label(x+3,y+1,name,color);
 rect(x+3,y+15,53,3,C_LINE);
 rect(x+3,y+15,(int)(value*53.f+.5f),3,color);
}
static void sentence(void){
 int x=9,y=35,lines=0,used=0;char word[26],ln[33];
 const char*p=mind.text;
 while(*p&&lines<6){
  int z=0;
  while(*p==' ')p++;
  while(*p&&*p!=' '&&z<24)word[z++]=*p++;
  word[z]=0;
  if(used&&used+1+z>29){
   ln[used]=0;label(x,y,ln,C_WHITE);y+=17;used=0;lines++;
  }
  if(used&&used<30)ln[used++]=' ';
  for(int i=0;i<z&&used<31;i++)ln[used++]=word[i];
 }
 if(used&&lines<7){ln[used]=0;label(x,y,ln,C_WHITE);}
}
static void scene(void){
 label(8,6,"B A B B L E B E A T",C_HI);
 label(234-text_w(&FONT_XS,page?"WORDS":"VOICE"),6,page?"WORDS":"VOICE",C_GRAY);
 line(8,25,232,25,C_LINE);
 sentence();
 label(9,130,bb_pattern_label(&mind),C_GRAY);
 int y=144;line(9,y+16,231,y+16,C_LINE);
 uint32_t s=mind.rng;
 for(int x=10;x<230;x+=5){
  s=s*1664525u+1013904223u;
  int h=(int)((s>>27)&15u);
  if(bb_voice_active(&mouth))h+=4;
  line(x,y+16-h/2,x,y+16+h/2,page?C_AMB:C_HI);
 }
 if(bb_voice_active(&mouth)){
  int x=10+(int)(220u*(unsigned)mouth.pos/(mouth.used?mouth.used:1u));
  rect(x,y-2,2,36,C_WHITE);
 }
 label(8,178,autoplay?"PLAY: SPEAKING":"PLAY: STOPPED",
  autoplay?C_WHITE:C_GRAY);
 label(235-text_w(&FONT_XS,"SEQ NEW / ARP MUTATE"),178,
  "SEQ NEW / ARP MUTATE",C_GRAY);
 const char*note=message_until&&(int32_t)(message_until-plat_ms())>0?message:0;
 if(note)label(8,116,note,C_AMB);
 else if(mind.has_memory){
  label(8,116,"MEMORY:",C_GRAY);
  label(64,116,mind.remembered.text,C_HI);
 }else label(8,116,"REC REMEMBERS A WORD",C_GRAY);
 static const uint8_t COL[7]={2,3,2,3,1,1,0},ROW[7]={0,0,1,1,0,1,1};
 knob(0,0,"VOLUME",(float)plat_master()/4096.f,0);
 for(int k=0;k<7;k++)knob(COL[k],ROW[k],NAMES[page][k],
  params[page][k],touched==k&&plat_ms()-touched_at<1000u);
 rect(119,201,2,37,C_DIM);
}
static void draw(void){
 for(int i=0;i<2;i++){bb_yoff=i*120;cv_begin(240,120,C_BLACK);scene();cv_blit(0,(uint32_t)bb_yoff);}
}
void app_ui_init(void){lcd_fill(0,0,240,240,C_BLACK);draw();}
void app_frame(void){app_input_only();if((++frames&3u)==0)draw();}
#ifdef PLAT_HOST
int app_expect(const char*name,int*got){
 if(!strcmp(name,"page"))*got=page;
 else if(!strcmp(name,"play"))*got=autoplay;
 else if(!strcmp(name,"words"))*got=bb_words(&mind);
 else if(!strcmp(name,"remember"))*got=mind.has_memory;
 else if(!strcmp(name,"grammar"))*got=(int)(params[1][0]*100.f+.5f);
 else if(!strcmp(name,"theme"))*got=(int)mind.theme;
 else if(!strcmp(name,"voice"))*got=bb_voice_active(&mouth);
 else return 0;
 return 1;
}
#endif
