/* SPDX-License-Identifier: MPL-2.0
 * BabbleBeat / third FM-1 firmware. One press each; nothing but GLO is held.
 *
 *   white keys   sixteen phrases, one a key. A key says its phrase (the first time, makes it), lower keys in a
 *                lower voice. The key last played is the phrase everything else works on.
 *   black keys   eleven mutators, each one named mutation of that phrase, said at once: NEW NOUN, SWAP NOUNS,
 *                NEW ADJECTIVE, REBUILD, NEGATE, TENSE, ASK, ADD CLAUSE, RECALL, STUTTER, DROP (left to right)
 *   FX, EDIT     the VOICE knobs, the WORDS knobs
 *   SEQ          a new phrase on the key; ARP a mutation by chance; REC remembers a noun of it
 *   PLAY         babbles: new phrases one after another; HOME stops
 *   OCT- OCT+    the whole voice down or up an octave
 *   GLO, held    the guide: what every button does, on the screen, laid out as the buttons are
 *   SAVE, SEL, ENV, LFO   nothing: they say so
 *
 * The screen, drawn as two strips of 240 x 120 (no line of text may cross y 120): the phrase (y 28..102), a line
 * for what was just done or what is remembered (y 104), the phrase's pattern and the sixteen keys' phrases as a
 * row of marks (y 122), the voice's own level over the last second with a mark where it is in the phrase
 * (y 138..172), PLAY's state, the knobs. */
#include "babble_lang.h"
#include "babble_voice.h"
#define NPHRASE 16                       /* the white keys */
#define NMUTE 11                         /* the black keys */
/* the keys by colour, left to right (the FM-1's 27 keys start on F) */
static const uint8_t WHITE_K[NPHRASE]={0,2,4,6,7,9,11,12,14,16,18,19,21,23,24,26};
static const uint8_t BLACK_K[NMUTE]={1,3,5,8,10,13,15,17,20,22,25};
typedef struct {bb_token tok[BB_MAX_TOKENS];uint8_t count,pattern,used;} phrase;
static phrase phrases[NPHRASE];          /* each white key's phrase, as it was left */
static int cur;                          /* the key whose phrase is the one on the screen (mind's) */
static bb_mind mind;
static bb_voice mouth;
static uint8_t page,autoplay;
static int8_t octave,last_op=-1;
static uint32_t old_keys,old_buttons,frames,last_spoken;
static uint8_t stirred;                  /* the first press has put its moment into the sentences' chance */
static float gain_now;
static uint8_t guide;                    /* GLO is held: the guide is on the screen */
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
/* the phrase on the screen kept on its key; a key's phrase brought to the screen */
static void keep(void){
 phrase*p=&phrases[cur];
 for(unsigned i=0;i<mind.count&&i<BB_MAX_TOKENS;i++)p->tok[i]=mind.tok[i];
 p->count=(uint8_t)mind.count;p->pattern=(uint8_t)mind.pattern;p->used=1;
}
static void bring(int k){
 const phrase*p=&phrases[k];
 for(unsigned i=0;i<p->count;i++)mind.tok[i]=p->tok[i];
 mind.count=p->count;mind.pattern=p->pattern;
 bb_rebuild(&mind);
}
/* said in the register of its key (the middle keys: the PITCH knob's own), an octave either way with OCT */
static void speak(void){
 bb_voice_transpose(&mouth,(int)WHITE_K[cur]-12+12*octave);
 bb_voice_say(&mouth,&mind,-1);last_spoken=plat_ms();
}
static void fresh(void){bb_generate(&mind);keep();speak();}
void app_init(void){
 bb_init(&mind,0xBABB1E25u);bb_voice_init(&mouth);
 for(int p=0;p<2;p++)for(int k=0;k<7;k++)params[p][k]=DEFAULTS[p][k];
 apply();bb_generate(&mind);
 page=1;autoplay=0;octave=0;old_keys=old_buttons=frames=last_spoken=0;stirred=0;guide=0;
 for(int k=0;k<NPHRASE;k++)phrases[k].used=phrases[k].count=phrases[k].pattern=0;
 cur=6;keep();                           /* (the first phrase: on a middle key) */
 gain_now=0;message_until=0;touched=-1;
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
 /* the same sentences after every power-on would be a recital: the moment of the first press makes them this
  * session's own */
 if((nb||nk)&&!stirred){stirred=1;mind.rng^=now*2654435761u;if(!mind.rng)mind.rng=0xBABB1E25u;}
 if(nb&(1u<<B_FX))page=0;
 if(nb&(1u<<B_EDIT))page=1;
 if(nb&(1u<<B_PLAY)){
  autoplay=!autoplay;
  if(autoplay)fresh();else bb_voice_stop(&mouth);
 }
 if(nb&(1u<<B_SEQ))fresh();
 if(nb&(1u<<B_ARP)){bb_mutate(&mind);keep();speak();app_say("MUTATE:",bb_op_label((int)mind.change-1));}
 if(nb&(1u<<B_REC)){
  bb_remember(&mind);
  app_say("REMEMBERED",mind.has_memory?mind.remembered.text:"NONE");
 }
 guide=(uint8_t)((btn>>B_GLO)&1u);
 if(nb&(1u<<B_HOME)){autoplay=0;bb_voice_stop(&mouth);app_say("STOPPED","");}
 if(nb&(1u<<B_SAVE))app_say("SAVE: NOT AVAILABLE","");
 if(nb&(1u<<B_SEL))app_say("SEL: NOT USED","");
 if(nb&(1u<<B_ENV))app_say("ENV: NOT USED","");
 if(nb&(1u<<B_LFO))app_say("LFO: NOT USED","");
 if((nb&(1u<<B_OCTDN))&&octave>-2)octave--;
 if((nb&(1u<<B_OCTUP))&&octave<2)octave++;
 /* a white key: its phrase (made the first time), said; a black key: its mutation of the phrase, said */
 for(int j=0;j<NPHRASE;j++)if(nk&(1u<<WHITE_K[j])){
  cur=j;
  if(phrases[j].used){bring(j);speak();}
  else fresh();
 }
 for(int j=0;j<NMUTE;j++)if(nk&(1u<<BLACK_K[j])){
  bb_mutate_op(&mind,j);keep();speak();
  app_say("MUTATE:",bb_op_label(j));last_op=(int8_t)j;
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
 {
  /* the keys: the phrase's own lit, the others that hold a phrase and the mutators dim */
  uint32_t kl=1u<<WHITE_K[cur],kd=0;
  for(int j=0;j<NPHRASE;j++)if(phrases[j].used)kd|=1u<<WHITE_K[j];
  for(int j=0;j<NMUTE;j++)kd|=1u<<BLACK_K[j];
  if(guide)leds|=1u<<B_GLO;
  plat_lights(leds,kl,(1u<<B_FX)|(1u<<B_EDIT)|(1u<<B_PLAY)|
   (1u<<B_SEQ)|(1u<<B_ARP)|(1u<<B_REC)|(1u<<B_HOME)|(1u<<B_GLO),kd&~kl,1);
 }
}
static void knob(int col,int row,const char*name,float value,int lit){
 int x=col*60,y=200+row*20;
 uint16_t color=lit?C_WHITE:page?C_HI:C_AMB;
 label(x+3,y+1,name,color);
 rect(x+3,y+15,53,3,C_LINE);
 rect(x+3,y+15,(int)(value*53.f+.5f),3,color);
}
static void sentence(void){
 int x=9,y=28,lines=0,used=0;char word[26],ln[33];
 const char*p=mind.text;
 while(*p&&lines<5){
  int z=0;
  while(*p==' ')p++;
  while(*p&&*p!=' '&&z<24)word[z++]=*p++;
  word[z]=0;
  if(used&&used+1+z>29){
   ln[used]=0;label(x,y,ln,C_WHITE);y+=15;used=0;lines++;
  }
  if(used&&used<30)ln[used++]=' ';
  for(int i=0;i<z&&used<31;i++)ln[used++]=word[i];
 }
 if(used&&lines<5){ln[used]=0;label(x,y,ln,C_WHITE);}
}
static void scene(void){
 static float lev[BB_LEVELS];
 static const uint8_t COL[7]={2,3,2,3,1,1,0},ROW[7]={0,0,1,1,0,1,1};
 if(guide){
  /* GLO held: what every button does, where the buttons are (two rows of six), then the keys */
  static const char *const G[12][3]={
   {"FX","VOICE","KNOBS"},{"SEL","-",""},{"ENV","-",""},{"LFO","-",""},{"EDIT","WORDS","KNOBS"},{"GLO","THIS","GUIDE"},
   {"HOME","STOP",""},{"SAVE","-",""},{"ARP","MUT-","ATE"},{"SEQ","NEW","WORDS"},{"PLAY","TALK","ON OFF"},{"REC","KEEP","NOUN"}};
  label(8,2,"BABBLEBEAT GUIDE",C_HI);
  label(234-text_w(&FONT_XS,"LET GO OF GLO"),2,"LET GO OF GLO",C_GRAY);
  for(int i=0;i<12;i++){
   int x=(i%6)*40,y=17+(i/6)*42,used=G[i][1][0]!='-';
   rect(x+1,y,38,1,C_LINE);rect(x+1,y+40,38,1,C_LINE);rect(x+1,y,1,41,C_LINE);rect(x+38,y,1,41,C_LINE);
   label(x+3,y+1,G[i][0],used?C_GRAY:C_DIM);
   label(x+3,y+13,G[i][1],used?C_WHITE:C_DIM);
   label(x+3,y+25,G[i][2],C_HI);
  }
  label(4,104,"WHITE KEYS: A PHRASE EACH",C_WHITE);
  label(4,122,"BLACK KEYS: MUTATE IT, LEFT TO RIGHT:",C_WHITE);
  label(4,136,"NOUN SWAP ADJ REBUILD NEGATE TENSE",C_AMB);
  label(4,150,"ASK CLAUSE RECALL STUTTER DROP",C_AMB);
  label(4,166,"OCT- OCT+: THE VOICE DOWN / UP",C_GRAY);
  label(4,180,"KNOBS: NAMED BELOW (MASTER = VOLUME)",C_GRAY);
  knob(0,0,"VOLUME",(float)plat_master()/4096.f,0);
  for(int k=0;k<7;k++)knob(COL[k],ROW[k],NAMES[page][k],params[page][k],0);
  rect(119,201,2,37,C_DIM);
  return;
 }
 const char*note=message_until&&(int32_t)(message_until-plat_ms())>0?message:0;
 int y=140,talking=bb_voice_active(&mouth);
 label(8,4,"B A B B L E B E A T",C_HI);
 label(234-text_w(&FONT_XS,page?"WORDS":"VOICE"),4,page?"WORDS":"VOICE",C_GRAY);
 line(8,22,232,22,C_LINE);
 sentence();
 /* what was just done; else what is remembered (y 104: whole in the upper strip) */
 if(note)label(8,104,note,C_AMB);
 else if(mind.has_memory){
  label(8,104,"MEMORY:",C_GRAY);
  label(64,104,mind.remembered.text,C_HI);
 }else label(8,104,"REC REMEMBERS A WORD",C_GRAY);
 label(9,122,bb_pattern_label(&mind),C_GRAY);
 for(int j=0;j<NPHRASE;j++)             /* the sixteen keys' phrases: this one, those there are, those not yet */
  rect(134+j*6,j==cur?123:126,4,j==cur?10:5,j==cur?C_WHITE:phrases[j].used?C_HI:C_LINE);
 /* the voice's own level, the last second of it (not a decoration: silence is a flat line), and where it is in
  * the sentence */
 line(9,y+16,231,y+16,C_LINE);
 bb_voice_levels(&mouth,lev,BB_LEVELS);
 for(int i=0;i<BB_LEVELS;i++){
  int x=12+i*5,h=(int)(lev[i]*30.f+.5f);
  if(h>0)line(x,y+16-h/2,x,y+16+h/2,page?C_AMB:C_HI);
 }
 if(talking){
  int x=10+(int)(220.f*bb_voice_progress(&mouth));
  rect(x,y-2,2,36,C_WHITE);
 }
 label(8,178,autoplay?"PLAY: SPEAKING":"PLAY: STOPPED",
  autoplay?C_WHITE:C_GRAY);
 label(235-text_w(&FONT_XS,"SEQ NEW / ARP MUTATE"),178,
  "SEQ NEW / ARP MUTATE",C_GRAY);
 knob(0,0,"VOLUME",(float)plat_master()/4096.f,0);
 for(int k=0;k<7;k++)knob(COL[k],ROW[k],NAMES[page][k],
  params[page][k],touched==k&&plat_ms()-touched_at<1000u);
 rect(119,201,2,37,C_DIM);
}
static void draw(void){
 for(int i=0;i<2;i++){bb_yoff=i*120;cv_begin(240,120,C_BLACK);scene();cv_blit(0,(uint32_t)bb_yoff);}
}
void app_ui_init(void){
 lcd_fill(0,0,240,240,C_BLACK);
 app_say(APP_NAME,APP_VERSION);message_until=plat_ms()+2500u;      /* (its name and version, for a moment) */
 draw();
}
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
 else if(!strcmp(name,"key"))*got=cur;
 else if(!strcmp(name,"phrases")){int n=0;for(int k=0;k<NPHRASE;k++)n+=phrases[k].used;*got=n;}
 else if(!strcmp(name,"op"))*got=last_op;
 else if(!strcmp(name,"guide"))*got=guide;
 else if(!strcmp(name,"pattern"))*got=(int)mind.pattern;
 else if(!strcmp(name,"note"))*got=message_until&&(int32_t)(message_until-plat_ms())>0;   /* a message is showing */
 else return 0;
 return 1;
}
#endif
