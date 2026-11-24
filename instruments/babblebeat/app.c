/* SPDX-License-Identifier: MPL-2.0
 * BabbleBeat / third FM-1 firmware. One press each; nothing but GLO is held.
 *
 *   white keys   sixteen words, one a key (the nouns of the WORLD, in order). A key says a phrase about its word:
 *                a plain statement, kept as it was left. Now and then the word moves to the next word of its
 *                world (WANDER: how often), and now and then the phrase is made again (DAMAGE)
 *   black keys   the word of the white key to their left, in another kind of phrase, a kind a key: WHY?, NOT, WAS,
 *                IF, WILL, IS IT?, DID NOT, USED TO, BECAUSE, MAY, WHEN (left to right)
 *   FX, EDIT     the VOICE knobs, the WORDS knobs; SEL the next set of knobs (VOICE, MOUTH, WORDS)
 *   SEQ          a new word on the key; ARP its phrase made again; REC remembers the key's word (RECALL brings it
 *                back into other phrases, and at its top end the phrases loop)
 *   PLAY         babbles: one phrase after another about the key's word; HOME stops
 *   OCT- OCT+    the whole voice down or up an octave (each key has its own register besides)
 *   GLO, held    the guide: what every button does, on the screen, laid out as the buttons are
 *   SAVE, ENV, LFO   nothing: they say so
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
#define NSLOT 27
typedef struct {bb_token tok[BB_MAX_TOKENS];uint8_t count,pattern,level,used;const char *word;} phrase;
static phrase phrases[NSLOT];            /* each key's phrase, as it was left, and the word it was made about */
static bb_token words[NPHRASE];          /* each white key's word (text 0: not given yet) */
static int cur;                          /* the key (0..26) whose phrase is the one on the screen (mind's) */
static uint16_t word_at;                 /* where in the world's list this session's words start */
static int world_now;                    /* WORLD as last seen (a turn into another world: every key's word again) */
/* the black keys' kinds of phrase, left to right (the language's patterns), and what the screen calls them */
static const uint8_t SHARP[NMUTE]={BB_WHY,BB_NOT,BB_WAS,BB_IF,BB_WILL,BB_ISIT,BB_DIDNOT,BB_USEDTO,BB_BECAUSE,BB_MAY,BB_WHEN};
static int grammar_level;                /* GRAMMAR's level as last seen (a turn across a level: every phrase again) */
static bb_mind mind;
static bb_voice mouth;
static uint8_t page,autoplay;
static int8_t octave;
static uint32_t old_keys,old_buttons,frames,last_spoken;
static uint8_t stirred;                  /* the first press has put its moment into the sentences' chance */
static float gain_now;
static uint8_t guide;                    /* GLO is held: the guide is on the screen */
static char message[36];
static uint32_t message_until;
static int touched=-1;
static uint32_t touched_at;
static const int ENC[7]={EN_K1,EN_K2,EN_K3,EN_K4,EN_SELECT,EN_ALGO,EN_PRESET};
#define NPAGE 3                          /* the knobs' sets: 0 VOICE (FX), 1 WORDS (EDIT), 2 MOUTH; SEL: the next */
static const float DEFAULTS[NPAGE][7]={
 {.32f,.52f,.50f,.18f,.78f,0.f,.30f}, /* Voice */
 {.50f,.25f,.35f,.15f,0.f,.40f,.50f}, /* Words */
 {.50f,.50f,.50f,0.f,0.f,0.f,0.f}     /* Mouth */
};
static float params[NPAGE][7];
static const char *const NAMES[NPAGE][7]={
 {"PITCH","RATE","TONE","BREATH","LEVEL","STUTTER","INFLECT"},
 {"GRAMMAR","WANDER","RECALL","DAMAGE","WORLD","LENGTH","STYLE"},
 {"SIZE","SLUR","GAP","WAVER","","",""}
};
static const char *const PAGE_NAME[NPAGE]={"VOICE","WORDS","MOUTH"};
static const uint8_t PAGE_LED[NPAGE]={B_FX,B_EDIT,B_SEL};
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
 bb_voice_controls(&mouth,params[0]);bb_voice_mouth(&mouth,params[2]);
 mind.grammar=params[1][0];mind.wander=params[1][1];
 mind.recall=params[1][2];mind.damage=params[1][3];
 mind.theme=(unsigned)(params[1][4]*((float)bb_worlds()-.001f));
 mind.length=params[1][5];mind.style=params[1][6];
}
static int black_of(int k){for(int j=0;j<NMUTE;j++)if(BLACK_K[j]==k)return j;return -1;}
/* the white key a key's word belongs to: its own, or for a black key the one to its left */
static int white_of(int k){
 int w=black_of(k)>=0?k-1:k;
 for(int j=0;j<NPHRASE;j++)if(WHITE_K[j]==w)return j;
 return 0;
}
/* a key's word, given the first time it is wanted: the WORLD's nouns in order along the keys (twelve to a world;
 * (they are in an order: neighbouring keys are neighbouring words), from where this session starts in it */
static bb_token *word_of(int k){
 int w=white_of(k);
 if(!words[w].text)words[w]=bb_noun_of((int)mind.theme,w+(int)word_at);
 return &words[w];
}
/* the phrase on the screen kept on its key; a key's phrase brought to the screen */
static void keep(void){
 phrase*p=&phrases[cur];
 for(unsigned i=0;i<mind.count&&i<BB_MAX_TOKENS;i++)p->tok[i]=mind.tok[i];
 p->count=(uint8_t)mind.count;p->pattern=(uint8_t)mind.pattern;p->level=(uint8_t)mind.level;p->used=1;p->word=word_of(cur)->text;
}
static void bring(int k){
 const phrase*p=&phrases[k];
 for(unsigned i=0;i<p->count;i++)mind.tok[i]=p->tok[i];
 mind.count=p->count;mind.pattern=p->pattern;mind.level=p->level;
 bb_rebuild(&mind);
}
/* said in the register of its key (the middle keys: the PITCH knob's own), an octave either way with OCT */
static void speak(void){
 bb_voice_transpose(&mouth,cur-12+12*octave);
 bb_voice_say(&mouth,&mind,-1);last_spoken=plat_ms();
}
/* a new phrase of the key's kind about its word: a plain statement on a white key, the black key's own kind */
static void compose(int k){
 int b=black_of(k);
 bb_phrase(&mind,word_of(k),b>=0?SHARP[b]:bb_statement(&mind));
}
/* the phrase made again: with DAMAGE past 60, as often broken instead (a word doubled, dropped, an "again") */
static void change(int k){
 if(mind.damage>.6f&&bb_chance(&mind,.5f))bb_mutate_op(&mind,BB_OP_STUTTER+(bb_chance(&mind,.34f)?0:bb_chance(&mind,.5f)?1:2));
 else compose(k);
}
/* a key: its phrase, said. The word moves now and then to the next word of its world (a white key's press;
 * WANDER: how often, and at 0 never); the phrase is made again when its word has moved, and now and then (DAMAGE) */
static void play_key(int k){
 phrase*p=&phrases[k];
 bb_token*w=word_of(k);
 int b=black_of(k),moved=0;
 if(p->used&&b<0&&bb_chance(&mind,mind.wander*.25f)){*w=bb_noun_near(&mind,w);moved=1;}
 cur=k;
 if(!p->used||p->word!=w->text||p->level!=bb_level(&mind))compose(k);   /* (none yet; its word moved; GRAMMAR turned) */
 else{
  bring(k);
  if(bb_chance(&mind,mind.damage))change(k);
 }
 keep();speak();
 if(moved)app_say("ITS WORD IS NOW",w->text);
 else if(b>=0)app_say(bb_pattern_label(&mind),w->text);
}
/* PLAY: one phrase after another about the key's word, of any kind; the word moving as it does under the key */
static void babble(void){
 bb_token*w=word_of(cur);
 if(bb_chance(&mind,mind.wander*.25f))*w=bb_noun_near(&mind,w);
 bb_phrase(&mind,w,bb_chance(&mind,.6f)?bb_statement(&mind):SHARP[(mind.rng>>9)%NMUTE]);
 speak();
}
void app_init(void){
 bb_init(&mind,0xBABB1E25u);bb_voice_init(&mouth);
 for(int p=0;p<NPAGE;p++)for(int k=0;k<7;k++)params[p][k]=DEFAULTS[p][k];
 apply();
 page=1;autoplay=0;octave=0;old_keys=old_buttons=frames=last_spoken=0;stirred=0;guide=0;word_at=0;
 for(int k=0;k<NSLOT;k++){phrases[k].used=phrases[k].count=phrases[k].pattern=0;phrases[k].word=0;}
 for(int k=0;k<NPHRASE;k++)words[k]=(bb_token){0,0,BB_NOUN};
 grammar_level=bb_level(&mind);world_now=(int)mind.theme;
 cur=11;compose(cur);keep();             /* (the first phrase: on a middle key, the C) */
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
 if((nb||nk)&&!stirred){stirred=1;mind.rng^=now*2654435761u;if(!mind.rng)mind.rng=0xBABB1E25u;word_at=(uint16_t)(mind.rng%4096u);}
 if(nb&(1u<<B_FX))page=0;
 if(nb&(1u<<B_EDIT))page=1;
 if(nb&(1u<<B_PLAY)){
  autoplay=!autoplay;
  if(autoplay)babble();else bb_voice_stop(&mouth);
 }
 if(nb&(1u<<B_SEQ)){                     /* a new word on the key (the next one that is not its own), and its phrase */
  bb_token*w=word_of(cur),was=*w;
  *w=bb_noun_of((int)mind.theme,(int)(mind.rng>>5));
  if(w->text==was.text)*w=bb_noun_near(&mind,w);
  compose(cur);keep();speak();app_say("NEW WORD:",w->text);
 }
 if(nb&(1u<<B_ARP)){bring(cur);change(cur);keep();speak();app_say("PHRASE CHANGED","");}
 if(nb&(1u<<B_REC)){
  bb_remember_word(&mind,word_of(cur));
  app_say("REMEMBERED",mind.remembered.text);
 }
 guide=(uint8_t)((btn>>B_GLO)&1u);
 if(nb&(1u<<B_HOME)){autoplay=0;bb_voice_stop(&mouth);app_say("STOPPED","");}
 if(nb&(1u<<B_SAVE))app_say("SAVE: NOT AVAILABLE","");
 if(nb&(1u<<B_SEL)){page=page==0?2:page==2?1:0;app_say("KNOBS:",PAGE_NAME[page]);}
 if(nb&(1u<<B_ENV))app_say("ENV: NOT USED","");
 if(nb&(1u<<B_LFO))app_say("LFO: NOT USED","");
 if((nb&(1u<<B_OCTDN))&&octave>-2)octave--;
 if((nb&(1u<<B_OCTUP))&&octave<2)octave++;
 /* a key: its word's phrase, said (a white key: a statement; a black key: its own kind, of the word to its left) */
 for(int k=0;k<NSLOT;k++)if(nk&(1u<<k))play_key(k);
 int turned=0;
 for(int k=0;k<7;k++){
  int delta=plat_enc(ENC[k]);
  if(!delta||!NAMES[page][k][0])continue;
  turned=1;
  float v=params[page][k]+(float)delta/64.f;
  params[page][k]=v<0?0:v>1?1:v;
  touched=k;touched_at=now;apply();
 }
 /* GRAMMAR turned across a level: the phrase on the screen is built again that way at once, and said; every
  * other key's follows when it is next played */
 if(turned&&bb_level(&mind)!=grammar_level){
  grammar_level=bb_level(&mind);
  compose(cur);keep();speak();
  app_say("GRAMMAR:",bb_level_label(grammar_level));
 }
 /* WORLD turned into another world: every key gives up its word and takes one from there when it is next
  * played; the key on the screen at once, and said */
 if(turned&&(int)mind.theme!=world_now){
  world_now=(int)mind.theme;
  for(int k=0;k<NPHRASE;k++)words[k]=(bb_token){0,0,BB_NOUN};
  compose(cur);keep();speak();
  app_say("WORLD:",bb_world_label(world_now));
 }
 old_buttons=btn;old_keys=keys;
 if(autoplay&&!bb_voice_active(&mouth)&&now-last_spoken>350u)babble();
 uint32_t leds=(1u<<PAGE_LED[page])|(autoplay?LED_PLAY_GREEN:0)|
  (mind.has_memory?(1u<<B_REC):0);
 {
  /* the keys: the phrase's own lit, the others that hold a phrase and the mutators dim */
  uint32_t kl=1u<<cur,kd=0;
  for(int j=0;j<NPHRASE;j++)if(words[j].text)kd|=1u<<WHITE_K[j];
  for(int j=0;j<NMUTE;j++)kd|=1u<<BLACK_K[j];
  if(guide)leds|=1u<<B_GLO;
  plat_lights(leds,kl,(1u<<B_FX)|(1u<<B_EDIT)|(1u<<B_SEL)|(1u<<B_PLAY)|
   (1u<<B_SEQ)|(1u<<B_ARP)|(1u<<B_REC)|(1u<<B_HOME)|(1u<<B_GLO),kd&~kl,1);
 }
}
static void knob(int col,int row,const char*name,float value,int lit){
 int x=col*60,y=200+row*20;
 uint16_t color=lit?C_WHITE:page?C_HI:C_AMB;
 if(!name[0])return;                     /* (a knob with nothing on this set) */
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
   {"FX","VOICE","KNOBS"},{"SEL","NEXT","KNOBS"},{"ENV","-",""},{"LFO","-",""},{"EDIT","WORDS","KNOBS"},{"GLO","THIS","GUIDE"},
   {"HOME","STOP",""},{"SAVE","-",""},{"ARP","MUT-","ATE"},{"SEQ","NEW","WORD"},{"PLAY","TALK","ON OFF"},{"REC","KEEP","WORD"}};
  label(8,2,"BABBLEBEAT GUIDE",C_HI);
  label(234-text_w(&FONT_XS,"LET GO OF GLO"),2,"LET GO OF GLO",C_GRAY);
  for(int i=0;i<12;i++){
   int x=(i%6)*40,y=17+(i/6)*42,used=G[i][1][0]!='-';
   rect(x+1,y,38,1,C_LINE);rect(x+1,y+40,38,1,C_LINE);rect(x+1,y,1,41,C_LINE);rect(x+38,y,1,41,C_LINE);
   label(x+3,y+1,G[i][0],used?C_GRAY:C_DIM);
   label(x+3,y+13,G[i][1],used?C_WHITE:C_DIM);
   label(x+3,y+25,G[i][2],C_HI);
  }
  label(4,104,"WHITE KEYS: A WORD EACH, IN A PHRASE",C_WHITE);
  label(4,122,"BLACK KEYS: THE WORD ON THEIR LEFT, AS",C_WHITE);
  label(4,136,"WHY? NOT WAS IF WILL  IS IT? DID NOT",C_AMB);
  label(4,150,"USED TO  BECAUSE  MAY  WHEN",C_AMB);
  label(4,166,"GRAMMAR (EDIT): BARE TO NESTED",C_GRAY);
  label(4,180,"KNOBS: NAMED BELOW (MASTER = VOLUME)",C_GRAY);
  knob(0,0,"VOLUME",(float)plat_master()/4096.f,0);
  for(int k=0;k<7;k++)knob(COL[k],ROW[k],NAMES[page][k],params[page][k],0);
  rect(119,201,2,37,C_DIM);
  return;
 }
 const char*note=message_until&&(int32_t)(message_until-plat_ms())>0?message:0;
 int y=140,talking=bb_voice_active(&mouth);
 label(8,4,"B A B B L E B E A T",C_HI);
 label(234-text_w(&FONT_XS,PAGE_NAME[page]),4,PAGE_NAME[page],C_GRAY);
 line(8,22,232,22,C_LINE);
 sentence();
 /* what was just done; else the kind of phrase, and what is remembered (y 104: whole in the upper strip) */
 if(note)label(8,104,note,C_AMB);
 else{
  label(label(8,104,bb_pattern_label(&mind),C_GRAY)+8,104,bb_level_label((int)mind.level),C_AMB);
  if(mind.has_memory)label(234-text_w(&FONT_XS,mind.remembered.text),104,mind.remembered.text,C_HI);
 }
 /* the key's word; the sixteen keys' words as a row of marks: this one's, those given, those not yet */
 label(label(9,122,"WORD: ",C_GRAY),122,words[white_of(cur)].text,C_HI);
 for(int j=0;j<NPHRASE;j++){
  int here=j==white_of(cur);
  rect(134+j*6,here?123:126,4,here?10:5,here?C_WHITE:words[j].text?C_HI:C_LINE);
 }
 if(black_of(cur)>=0)rect(137+white_of(cur)*6,119,4,3,C_AMB);         /* (a black key: its mark, up and to the right) */
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
 label(235-text_w(&FONT_XS,"SEL: MORE KNOBS"),178,"SEL: MORE KNOBS",C_GRAY);
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
 else if(!strcmp(name,"spoken"))*got=bb_words(&mind);
 else if(!strcmp(name,"remember"))*got=mind.has_memory;
 else if(!strcmp(name,"grammar"))*got=(int)(params[1][0]*100.f+.5f);
 else if(!strcmp(name,"theme"))*got=(int)mind.theme;
 else if(!strcmp(name,"worlds"))*got=bb_worlds();
 else if(!strcmp(name,"voice"))*got=bb_voice_active(&mouth);
 else if(!strcmp(name,"key"))*got=cur;
 else if(!strcmp(name,"white"))*got=white_of(cur);
 else if(!strcmp(name,"sharp"))*got=black_of(cur);
 else if(!strcmp(name,"phrases")){int n=0;for(int k=0;k<NSLOT;k++)n+=phrases[k].used;*got=n;}
 else if(!strcmp(name,"words")){int n=0;for(int k=0;k<NPHRASE;k++)n+=words[k].text!=0;*got=n;}
 else if(!strcmp(name,"has")){*got=0;for(unsigned i=0;i<mind.count;i++)if(mind.tok[i].text==words[white_of(cur)].text)*got=1;}
 else if(!strcmp(name,"wander"))*got=(int)(params[1][1]*100.f+.5f);
 else if(!strcmp(name,"size"))*got=(int)(params[2][0]*100.f+.5f);
 else if(!strcmp(name,"guide"))*got=guide;
 else if(!strcmp(name,"pattern"))*got=(int)mind.pattern;
 else if(!strcmp(name,"level"))*got=(int)mind.level;
 else if(!strcmp(name,"tokens"))*got=(int)mind.count;
 else if(!strcmp(name,"note"))*got=message_until&&(int32_t)(message_until-plat_ms())>0;   /* a message is showing */
 else return 0;
 return 1;
}
#endif
