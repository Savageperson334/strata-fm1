/* SPDX-License-Identifier: MPL-2.0
 * Tiny deterministic grammar compiler. A hand-curated lexicon with compact
 * approximated phoneme spellings, not the full WordNet / CMUdict dataset.
 * Pronunciation (the voice's table, dsp/babble_voice.c): i=ee I=ih e=eh A=a(t) a=ah O=aw U=uh V=oo(k) u=oo E=er
 * o=o(r); the diphthongs 1=ay 2=eye 3=ow 4=oy 5=oh; S=sh T=th(in) D=th(e) C=ch J=j N=ng; others as written. */
#include "babble_lang.h"
typedef struct {const char *text,*phones;} word;
/* a verb as said of something (third) and plain (base), and the worlds it suits (a bit a world, WORLD[]'s order;
 * 0: any): the kind of doing it is, by WordNet, against the kind of thing the worlds hold */
typedef struct {const char *third,*base,*phones,*base_phones;unsigned suits;} verb;
#define SZ(a) ((int)(sizeof(a)/sizeof((a)[0])))
/* the words: worlds of nouns (WORLD[]: the first four BabbleBeat's own, twelve each; the rest from WordNet, their
 * words in the order of its tree, so that neighbours are kin), adjectives (adj), adverbs (adv), prepositions
 * (prep), and verbs that take an object (trans) or none (intr). Generated: tools/gen_lexicon.py */
#include "babble_words.h"
/* The worlds as the rest of the file knows them: world 0 is ALL, every world's nouns one after another (so its
 * neighbours are kin too, and it runs from machines to deeds); worlds 1.. are WORLD[0..] themselves. */
#define BB_THEMES_N (BB_WORLDS_N+1)
static unsigned theme_size(int t){
 unsigned n=0;
 if(t)return WORLD[t-1].n;
 for(int w=0;w<BB_WORLDS_N;w++)n+=WORLD[w].n;
 return n;
}
/* (world: the word's own world, 1.., whichever theme it was asked of) */
static word theme_word(int t,unsigned i,int *world){
 if(t){*world=t;return WORLD[t-1].w[i%WORLD[t-1].n];}
 i%=theme_size(0);
 for(int w=0;;w++){if(i<WORLD[w].n){*world=w+1;return WORLD[w].w[i];}i-=WORLD[w].n;}
}
static int theme_of(int t){return ((t%BB_THEMES_N)+BB_THEMES_N)%BB_THEMES_N;}
static const bb_token THE={"the","DU",BB_OTHER},AND={"and","And",BB_OTHER},AGAIN={"again","Ugen",BB_OTHER},
 DOT={".","",BB_PUNCT},QUERY={"?","",BB_PUNCT},COMMA={",","",BB_PUNCT};
/* the kinds of phrase (BB_STATE..BB_WHEN) and GRAMMAR's five ways of building one, as the screen names them */
static const char *const BB_PATTERNS[BB_KINDS_N]={
 "STATEMENT","WHY?","NOT","WAS","IF","WILL","IS IT?","DID NOT","USED TO","BECAUSE","MAY","WHEN"
};
static const char *const BB_LEVELS[BB_LEVELS_N]={"BARE","SIMPLE","PLAIN","JOINED","NESTED"};
#define BB_PATTERNS_N BB_KINDS_N
/* the small words the forms are written with (the voice's spelling beside each) */
/* (unmarked: said lightly, as these little words are; ' before a vowel: the word's stress there) */
static const word FW[]={
 {"the","D@"},{"is","Iz"},{"not","n'at"},{"inside","Ins'2d"},{"and","And"},{"why","w'2"},{"does","dUz"},
 {"if","If"},{"when","wen"},{"because","bIk'Uz"},{"but","bUt"},{"that","DAt"},{"will","wIl"},{"did","dId"},
 {"was","wUz"},{"may","m1"},{"used","y'uzd"},{"to","tu"},{"no","n'5"},{"once","w'Uns"},{"never","n'evEr"},
 {"still","st'Il"},{"then","Den"},{"only","'5nli"},{"so","s5"},{"or","or"},{"soon","s'un"},{"maybe","m'1bi"},
 {"until","Unt'Il"},{"after","'AftEr"},{"more","m'or"},{"here","h'ir"},{"of","@v"},{"than","DAn"},{"like","l2k"},
 {"every","'evri"},{"this","DIs"},{"there","Der"},{"very","v'eri"},{"almost","'Olm5st"},{"as","Az"}
};
/* GRAMMAR. Every kind of phrase has a form at each of five levels of structure, bare to nested (alternatives
 * after a |). In a form: N the word's noun phrase ("the mirror"), aN the same with an adjective, n a bare noun,
 * O another noun phrase, V v a verb that takes an object (as said of it; plain), I i one that takes none,
 * A an adjective, R an adverb, P a preposition; anything else is itself. The first noun in a form is the key's word. */
static const char *const FORM[BB_KINDS_N][BB_LEVELS_N]={
 /* STATEMENT */{"N .|aN .|only N .|n , n .|here , N .|every n .|N of O .|almost N .|N P O .",
                 "N I .|N is A .|aN I .|N I R .|every n I .|N is very A .|this n I .|there is N .",
                 "aN V O .|N V O .|N I P O .|N is A and A .|N R V O .|N of O V O .|N is more A than O .|no n V O .|N V O like O .|N I P aN .|N is as A as O .",
                 "N I , but O V O .|N V O , and O I .|N is A , so O I .|N I and O I .|N I P O , and O I R .|every n V O , but this n I .",
                 "N that V O I .|N V O that V O .|N that I V O when O I .|N V O because O that I is A .|N that I P O V O R .|N of O that I V O like O ."},
 /* WHY? */     {"why N ?","why does N i ?|why is N A ?","why does N v O ?","why does N v O , and why does O i ?",
                 "why does N that I v O when O I ?"},
 /* NOT */      {"no n .|not N .","N does not i .|N is not A .|N does not i R .","N does not v O .|N does not i P O .","N does not v O , and O does not i .",
                 "N that I does not v O because O is not A ."},
 /* WAS */      {"once , aN .","N was A .|N was very A .","N was A P O .|N was more A than O .","N was A , but O was A .",
                 "N that was A used to v O when O was A ."},
 /* IF */       {"if N , O .","if N I , O I .","if N I , O V O .","if N I and O I , O V O .",
                 "if N that I V O , then O will i because O is A ."},
 /* WILL */     {"soon , N .","N will i .|N will i R .","N will v O .|N will i P O .","N will i , and then O will v O .",
                 "N that I will v O after O I ."},
 /* IS IT? */   {"N ?|A n ?|N of O ?","is N A ?|is N very A ?","is N A P O ?|does N v O ?|does N i P O ?","is N A , or is O A ?",
                 "is N that V O A when O I ?"},
 /* DID NOT */  {"never N .","N did not i .","N did not v O .","N did not v O , but O did not i .",
                 "N that I did not v O until O was A ."},
 /* USED TO */  {"no more n .","N used to i .","N used to v O .","N used to i , and O used to v O .",
                 "N that used to i still V O when O I ."},
 /* BECAUSE */  {"because N .","because N I .","N V O because O I .","N V O because O I , and O I because O is A .",
                 "N that I V O because O that I is A ."},
 /* MAY */      {"maybe N .","N may i .|N may i R .","N may v O .|N may i P O .","N may v O , or O may i .",
                 "N that I may v O if O that is A will i ."},
 /* WHEN */     {"when N , O .","when N I , O I .","when N I , O will v O .","when N I and O I , O will i .",
                 "when N that I V O , O that is A will i ."}
};
static unsigned next(bb_mind*m){
 unsigned x=m->rng;x^=x<<13;x^=x>>17;x^=x<<5;
 return m->rng=x?x:0x9E3779B9u;
}
static int roll(bb_mind*m,int n){return (int)(next(m)%(unsigned)n);}
static bb_token make(word w,unsigned role){bb_token x={w.text,w.phones,(uint8_t)role};return x;}
static void add(bb_mind*m,bb_token x){if(m->count<BB_MAX_TOKENS)m->tok[m->count++]=x;}
static void lit(bb_mind*m,const bb_token *t){add(m,*t);}
static int where(const char *text,int *theme,int *at);
/* a noun. The first one of a sentence is what the sentence is about: its world is kept (about), for the verbs */
static bb_token noun(bb_mind*m){
 int world=0,at;
 bb_token t;
 if(m->force_once){
  m->force_once=0;t=m->forced;
  if(!where(t.text,&world,&at))world=0;
 }else{
  int theme=theme_of((int)m->theme);
  if(roll(m,100)<(int)(m->wander*100.f)){
   /* Recalled nouns attract nearby words toward their original world. */
   theme=theme_of((int)m->drift_theme);
  }
  if(m->has_memory&&roll(m,100)<(int)(m->recall*100.f)){t=m->remembered;world=(int)m->memory_theme;}
  else t=make(theme_word(theme,(unsigned)roll(m,(int)theme_size(theme)),&world),BB_NOUN);
 }
 if(!m->about)m->about=(unsigned)world;
 return t;
}
static void N(bb_mind*m){add(m,noun(m));}
static void A(bb_mind*m){add(m,make(adj[roll(m,SZ(adj))],BB_ADJ));}
static void R(bb_mind*m){add(m,make(adv[roll(m,SZ(adv))],BB_OTHER));}
static void P(bb_mind*m){add(m,make(prep[roll(m,SZ(prep))],BB_OTHER));}
/* a verb for what the sentence is about: one that suits its world, looked for a few times; with WANDER up, more
 * and more often any verb at all */
static verb pick(bb_mind*m,const verb *table,int n){
 verb v=table[roll(m,n)];
 if(!m->about||roll(m,100)<(int)(m->wander*100.f))return v;
 for(int tries=0;tries<10&&v.suits&&!((v.suits>>(m->about-1))&1u);tries++)v=table[roll(m,n)];
 return v;
}
static void I(bb_mind*m,int base){
 verb v=pick(m,intr,SZ(intr));
 bb_token t={base?v.base:v.third,base?v.base_phones:v.phones,BB_VERB};
 add(m,t);
}
static void subj(bb_mind*m,int with_adj){
 lit(m,&THE);if(with_adj)A(m);N(m);
}
static void obj(bb_mind*m){lit(m,&THE);N(m);}
static void V(bb_mind*m,int base){
 verb v=pick(m,trans,SZ(trans));
 bb_token t={base?v.base:v.third,base?v.base_phones:v.phones,BB_VERB};add(m,t);
}
void bb_rebuild(bb_mind*m){
 int n=0;
 for(unsigned i=0;i<m->count;i++){
  const char *s=m->tok[i].text;
  if(!s)continue;
  if(i&&m->tok[i].role!=BB_PUNCT&&n<BB_TEXT_SIZE-1)m->text[n++]=' ';
  while(*s&&n<BB_TEXT_SIZE-1)m->text[n++]=*s++;
 }
 m->text[n]=0;
}
void bb_init(bb_mind*m,uint32_t seed){
 m->rng=seed?seed:1;m->count=0;m->theme=0;m->has_memory=0;
 m->pattern=m->drift_theme=m->memory_theme=m->force_once=m->change=0;
 m->remembered=(bb_token){0,0,BB_NOUN};
 m->forced=(bb_token){0,0,BB_NOUN};
 m->grammar=.5f;m->level=2;m->wander=.25f;m->recall=.35f;m->damage=.15f;
 m->length=.4f;m->style=.5f;m->text[0]=0;
}
static void extra_clause(bb_mind*m){
 if(m->count+5>=BB_MAX_TOKENS||m->tok[m->count-1].text!=DOT.text)return;
 m->count--;
 lit(m,&COMMA);lit(m,&AND);subj(m,0);I(m,0);lit(m,&DOT);
}
/* RECALL's top end: the sentence loses its way and starts again from its beginning ("the mirror was, the mirror
 * was pale."): it keeps its opening words and says the whole of itself after them */
static void loop_back(bb_mind*m){
 bb_token t[BB_MAX_TOKENS];
 int n=(int)m->count,body=n-1,cut,k=0;
 if(n<4||m->tok[n-1].role!=BB_PUNCT)return;
 cut=2+roll(m,body-2>0?body-2:1);       /* (the opening: two words at least, never the whole) */
 if(cut>body-1)cut=body-1;
 while(cut>2&&m->tok[cut-1].role==BB_PUNCT)cut--;   /* (not ending on a comma of its own) */
 if(n+cut+1>BB_MAX_TOKENS)return;
 for(int i=0;i<cut;i++)t[k++]=m->tok[i];
 t[k++]=COMMA;
 for(int i=0;i<n;i++)t[k++]=m->tok[i];
 for(int i=0;i<k;i++)m->tok[i]=t[i];
 m->count=(unsigned)k;
}
static int same(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
/* one of a form's alternatives (by chance), built word by word */
static void build(bb_mind*m,const char *t){
 int alts=1,pick;
 for(const char *c=t;*c;c++)if(*c=='|')alts++;
 for(pick=roll(m,alts);pick;t++)if(*t=='|')pick--;
 while(*t&&*t!='|'){
  char w[12];int n=0;
  while(*t==' ')t++;
  while(*t&&*t!=' '&&*t!='|'&&n<11)w[n++]=*t++;
  w[n]=0;
  if(!n)break;
  if(same(w,"N"))subj(m,0);
  else if(same(w,"aN"))subj(m,1);
  else if(same(w,"O"))obj(m);
  else if(same(w,"n"))N(m);
  else if(same(w,"V"))V(m,0);
  else if(same(w,"v"))V(m,1);
  else if(same(w,"I"))I(m,0);
  else if(same(w,"i"))I(m,1);
  else if(same(w,"A"))A(m);
  else if(same(w,"R"))R(m);
  else if(same(w,"P"))P(m);
  else if(same(w,"."))lit(m,&DOT);
  else if(same(w,"?"))lit(m,&QUERY);
  else if(same(w,","))lit(m,&COMMA);
  else for(int i=0;i<SZ(FW);i++)if(same(w,FW[i].text)){add(m,make(FW[i],BB_OTHER));break;}
 }
}
int bb_level(const bb_mind*m){int l=(int)(m->grammar*(float)BB_LEVELS_N);return l<0?0:l>=BB_LEVELS_N?BB_LEVELS_N-1:l;}
const char *bb_level_label(int level){return BB_LEVELS[level<0?0:level>=BB_LEVELS_N?BB_LEVELS_N-1:level];}
/* a phrase of a kind (BB_STATE..), in the form GRAMMAR's level gives it */
void bb_generate_pattern(bb_mind*m,int pattern){
 m->count=0;m->about=0;
 m->pattern=(unsigned)(pattern<0?0:pattern%BB_PATTERNS_N);
 m->level=(unsigned)bb_level(m);
 build(m,FORM[m->pattern][m->level]);
 if(m->length>.65f&&m->level>=2&&m->pattern!=BB_WHY&&m->pattern!=BB_ISIT&&
    roll(m,100)<(int)((m->length-.55f)*140.f)){
  extra_clause(m);
  if(m->length>.94f&&roll(m,100)<50)extra_clause(m);
 }
 m->force_once=0;
 /* RECALL past 70: it fixes on a word of its own if it was given none, and more and more often loops */
 if(m->recall>.7f){
  if(!m->has_memory)bb_remember(m);
  if(roll(m,100)<(int)((m->recall-.7f)*300.f)){
   loop_back(m);
   if(m->recall>.9f&&roll(m,100)<50)loop_back(m);
  }
 }
 bb_rebuild(m);
}
const char *bb_pattern_label(const bb_mind*m){
 return BB_PATTERNS[m->pattern%BB_PATTERNS_N];
}
void bb_generate(bb_mind*m){
 if(roll(m,100)<(int)(m->wander*65.f)){
  m->drift_theme=(m->has_memory&&roll(m,100)<(int)(m->recall*100.f))
   ?m->memory_theme:(unsigned)roll(m,BB_THEMES_N);
 }
 /* STYLE: statements only .. any kind (half of them plain statements) .. questions only */
 static const uint8_t SAYS[]={BB_STATE,BB_STATE,BB_STATE,BB_NOT,BB_WAS,BB_WILL,BB_DIDNOT,BB_USEDTO,BB_MAY};
 int pattern=roll(m,2)?BB_STATE:roll(m,BB_PATTERNS_N);
 if(m->style<.26f)pattern=SAYS[roll(m,SZ(SAYS))];
 else if(m->style>.74f)pattern=roll(m,2)?BB_WHY:BB_ISIT;
 bb_generate_pattern(m,pattern);
 m->change=0;
 /* DAMAGE: how often a new sentence is mutated as it is made; past 60 the mutation may break it, and twice */
 if(roll(m,100)<(int)(m->damage*100.f)){
  bb_mutate(m);
  if(m->damage>.6f&&roll(m,100)<40)bb_mutate(m);
 }
}
void bb_rewrite(bb_mind*m,int pattern){
 bb_token anchor={0,0,BB_NOUN};
 for(unsigned i=0;i<m->count;i++){
  if(m->tok[i].role==BB_NOUN){anchor=m->tok[i];break;}
 }
 if(anchor.text){m->forced=anchor;m->force_once=1;}
 bb_generate_pattern(m,pattern);
 m->change=3;
}
/* where a noun is: its world and its place there (by its text's address: the words are the tables' own) */
static int where(const char *text,int *theme,int *at){
 for(int t=0;t<BB_WORLDS_N;t++)for(unsigned i=0;i<WORLD[t].n;i++)if(WORLD[t].w[i].text==text){*theme=t+1;*at=(int)i;return 1;}
 return 0;
}
void bb_remember(bb_mind*m){
 int ids[BB_MAX_TOKENS],n=0;
 for(unsigned i=0;i<m->count;i++)if(m->tok[i].role==BB_NOUN)ids[n++]=(int)i;
 if(n){
  m->remembered=m->tok[ids[roll(m,n)]];
  m->has_memory=1;
  {int theme,at;if(where(m->remembered.text,&theme,&at))m->memory_theme=(unsigned)theme;}
 }
}
void bb_remember_word(bb_mind*m,const bb_token *w){
 if(!w||!w->text)return;
 m->remembered=*w;m->has_memory=1;
 {int theme,at;if(where(w->text,&theme,&at))m->memory_theme=(unsigned)theme;}
}
bb_token bb_noun_of(int theme,int i){
 int t=theme_of(theme),n=(int)theme_size(t);
 int world;
 return make(theme_word(t,(unsigned)(((i%n)+n)%n),&world),BB_NOUN);
}
int bb_worlds(void){return BB_THEMES_N;}
const char *bb_world_label(int theme){int t=theme_of(theme);return t?WORLD[t-1].name:"ALL";}
int bb_world_size(int theme){return (int)theme_size(theme_of(theme));}
bb_token bb_noun_near(bb_mind*m,const bb_token *w){
 int theme,at;
 if(w&&where(w->text,&theme,&at))return bb_noun_of(theme,at+(roll(m,2)?1:-1));
 return bb_noun_of((int)m->theme,roll(m,bb_world_size((int)m->theme)));
}
void bb_phrase(bb_mind*m,const bb_token *w,int pattern){
 if(w&&w->text){m->forced=*w;m->force_once=1;}
 bb_generate_pattern(m,pattern);
 m->change=0;
}
int bb_statement(bb_mind*m){(void)m;return BB_STATE;}
int bb_chance(bb_mind*m,float p){return roll(m,1000)<(int)(p*1000.f);}
static const char *const BB_OPS[BB_OPS_N]={
 "NEW NOUN","SWAP NOUNS","NEW ADJECTIVE","REBUILD","NEGATE","TENSE","ASK","ADD CLAUSE","RECALL","STUTTER","DROP","AGAIN"
};
const char *bb_op_label(int op){return op>=0&&op<BB_OPS_N?BB_OPS[op]:"";}
/* One named mutation of the sentence (BB_OP_*). Each does something every time: where the sentence has nothing for
 * it to work on (no adjective to change, a question that takes no clause), it rebuilds the sentence, around the
 * same first noun, into a shape that has. */
void bb_mutate_op(bb_mind*m,int op){
 int nouns[BB_MAX_TOKENS],nn=0,adjs[BB_MAX_TOKENS],na=0;
 for(unsigned i=0;i<m->count;i++){
  if(m->tok[i].role==BB_NOUN)nouns[nn++]=(int)i;
  if(m->tok[i].role==BB_ADJ)adjs[na++]=(int)i;
 }
 if(op<0||op>=BB_OPS_N)op=BB_OP_NOUN;
 m->change=(unsigned)op+1u;
 switch(op){
 case BB_OP_NOUN:if(nn)m->tok[nouns[roll(m,nn)]]=noun(m);break;
 case BB_OP_SWAP:
  if(nn>1){
   int ai=roll(m,nn),bi=(ai+1+roll(m,nn-1))%nn;
   int a=nouns[ai],b=nouns[bi];
   bb_token t=m->tok[a];m->tok[a]=m->tok[b];m->tok[b]=t;
  }else{bb_rewrite(m,BB_BECAUSE);m->change=(unsigned)op+1u;return;}       /* (one noun: a kind with more) */
  break;
 case BB_OP_ADJ:
  if(na)m->tok[adjs[roll(m,na)]]=make(adj[roll(m,SZ(adj))],BB_ADJ);
  else{bb_rewrite(m,BB_WAS);m->change=(unsigned)op+1u;return;}           /* (none: a kind that has one) */
  break;
 case BB_OP_REBUILD:{
  int to=roll(m,BB_PATTERNS_N);
  if(to==(int)m->pattern)to=(to+1)%BB_PATTERNS_N;
  bb_rewrite(m,to);m->change=(unsigned)op+1u;return;}
 case BB_OP_NEGATE:bb_rewrite(m,m->pattern==BB_NOT?BB_DIDNOT:m->pattern==BB_DIDNOT?BB_NOT:roll(m,2)?BB_NOT:BB_DIDNOT);m->change=(unsigned)op+1u;return;
 case BB_OP_TENSE:{
  static const int modes[]={BB_WILL,BB_WAS,BB_USEDTO};
  int k=roll(m,3);
  if(modes[k]==(int)m->pattern)k=(k+1)%3;
  bb_rewrite(m,modes[k]);m->change=(unsigned)op+1u;return;}
 case BB_OP_ASK:bb_rewrite(m,m->pattern==BB_WHY?BB_ISIT:m->pattern==BB_ISIT?BB_WHY:roll(m,2)?BB_WHY:BB_ISIT);m->change=(unsigned)op+1u;return;
 case BB_OP_CLAUSE:{
  unsigned before=m->count;
  extra_clause(m);
  if(m->count==before){bb_rewrite(m,BB_BECAUSE);m->change=(unsigned)op+1u;return;}       /* (no room, or a question) */
  break;}
 case BB_OP_RECALL:
  if(!m->has_memory)bb_remember(m);      /* (nothing remembered yet: this sentence's noun is) */
  if(nn&&m->has_memory)m->tok[nouns[roll(m,nn)]]=m->remembered;
  break;
 case BB_OP_STUTTER:
  if(m->count<BB_MAX_TOKENS-1&&m->count>2){
   int at=roll(m,(int)m->count-1);
   for(int i=(int)m->count;i>at;i--)m->tok[i]=m->tok[i-1];
   m->count++;
  }
  break;
 case BB_OP_DROP:
  if(m->count>3){
   int at=roll(m,(int)m->count-1);
   for(unsigned i=(unsigned)at;i<m->count-1;i++)m->tok[i]=m->tok[i+1];
   m->count--;
  }
  break;
 default:                                /* BB_OP_AGAIN */
  if(m->count<BB_MAX_TOKENS-1&&m->count>0){
   bb_token stop=m->tok[m->count-1];m->tok[m->count-1]=AGAIN;
   m->tok[m->count++]=stop;
  }
  break;
 }
 bb_rebuild(m);
}
/* A mutation by chance: one that keeps the phrase whole; with DAMAGE past 60, the ones that break it too. */
void bb_mutate(bb_mind*m){
 static const uint8_t KIND[]={BB_OP_NOUN,BB_OP_SWAP,BB_OP_ADJ,BB_OP_REBUILD,BB_OP_NEGATE,BB_OP_TENSE,
                              BB_OP_STUTTER,BB_OP_DROP,BB_OP_AGAIN,BB_OP_CLAUSE};
 bb_mutate_op(m,KIND[roll(m,m->damage>.6f?10:6)]);
}
int bb_words(const bb_mind*m){
 int n=0;
 for(unsigned i=0;i<m->count;i++)if(m->tok[i].phones&&m->tok[i].phones[0])n++;
 return n;
}
const bb_token *bb_word_at(const bb_mind*m,int index){
 int n=0;
 for(unsigned i=0;i<m->count;i++)if(m->tok[i].phones&&m->tok[i].phones[0])
  if(n++==index)return &m->tok[i];
 return 0;
}
