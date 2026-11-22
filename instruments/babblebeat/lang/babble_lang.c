/* SPDX-License-Identifier: MPL-2.0
 * Tiny deterministic grammar compiler. A hand-curated lexicon with compact
 * approximated phoneme spellings, not the full WordNet / CMUdict dataset.
 * Pronunciation (the voice's table, dsp/babble_voice.c): i=ee I=ih e=eh A=a(t) a=ah O=aw U=uh V=oo(k) u=oo E=er
 * o=o(r); the diphthongs 1=ay 2=eye 3=ow 4=oy 5=oh; S=sh T=th(in) D=th(e) C=ch J=j N=ng; others as written. */
#include "babble_lang.h"
typedef struct {const char *text,*phones;} word;
typedef struct {const char *third,*base,*phones,*base_phones;} verb;
#define SZ(a) ((int)(sizeof(a)/sizeof((a)[0])))
static const word nouns[4][12]={
 {{"machine","mUSin"},{"signal","sIgnUl"},{"circuit","sErkIt"},{"engine","enJIn"},{"robot","r5bat"},{"speaker","spikEr"},{"voltage","v5ltIJ"},{"static","stAtIk"},{"terminal","tErmInUl"},{"mirror","mIrEr"},{"antenna","AntenU"},{"filter","fIltEr"}},
 {{"ghost","g5st"},{"shadow","SAd5"},{"moon","mun"},{"window","wInd5"},{"planet","plAnIt"},{"garden","gardUn"},{"ritual","rICuUl"},{"memory","memEri"},{"door","dor"},{"fog","fag"},{"language","lANgwIJ"},{"dream","drim"}},
 {{"mouth","m3T"},{"throat","Tr5t"},{"tongue","tUN"},{"bone","b5n"},{"skin","skIn"},{"nerve","nErv"},{"lung","lUN"},{"insect","Insekt"},{"vowel","v3Ul"},{"breath","breT"},{"spine","sp2n"},{"voice","v4s"}},
 {{"castle","kAsUl"},{"coffin","kOfIn"},{"vampire","vAmp2r"},{"crypt","krIpt"},{"blood","blUd"},{"wolf","wVlf"},{"night","n2t"},{"diary","d2Uri"},{"chapel","CApUl"},{"letter","letEr"},{"count","k3nt"},{"grave","gr1v"}}
};
static const word adj[]={
 {"pale","p1l"},{"electric","IlektrIk"},{"ancient","1nSUnt"},{"silent","s2lUnt"},
 {"hidden","hIdUn"},{"crooked","krVkId"},{"synthetic","sInTetIk"},
 {"hollow","hal5"},{"broken","br5kUn"},{"restless","restlIs"},
 {"silver","sIlvEr"},{"strange","str1nJ"}
};
static const verb intr[]={
 {"waits","wait","w1ts","w1t"},{"sleeps","sleep","slips","slip"},
 {"whispers","whisper","wIspErz","wIspEr"},{"dreams","dream","drimz","drim"},
 {"shivers","shiver","SIvErz","SIvEr"},{"sings","sing","sINz","sIN"},
 {"returns","return","rItErnz","rItErn"},{"vanishes","vanish","vAnISIz","vAnIS"}
};
static const verb trans[]={
 {"remembers","remember","rImembErz","rImembEr"},
 {"follows","follow","fal5z","fal5"},{"haunts","haunt","hOnts","hOnt"},
 {"watches","watch","waCIz","waC"},{"forgets","forget","fErgets","fErget"},
 {"opens","open","5pUnz","5pUn"},{"swallows","swallow","swal5z","swal5"},
 {"corrupts","corrupt","kUrUpts","kUrUpt"}
};
static const bb_token THE={"the","DU",BB_OTHER},IS={"is","Iz",BB_OTHER},
 NOT={"not","nat",BB_OTHER},INSIDE={"inside","Ins2d",BB_OTHER},
 DO={"do","du",BB_OTHER},AND={"and","And",BB_OTHER},
 WHY={"why","w2",BB_OTHER},DOES={"does","dUz",BB_OTHER},
 AGAIN={"again","Ugen",BB_OTHER},DOT={".","",BB_PUNCT},
 QUERY={"?","",BB_PUNCT},COMMA={",","",BB_PUNCT},
 IF={"if","If",BB_OTHER},WHEN={"when","wen",BB_OTHER},
 BECAUSE={"because","bIkUz",BB_OTHER},BUT={"but","bUt",BB_OTHER},
 THAT={"that","DAt",BB_OTHER},WILL={"will","wIl",BB_OTHER},
 DID={"did","dId",BB_OTHER},WAS={"was","wUz",BB_OTHER},
 MAY={"may","m1",BB_OTHER},USED={"used","yuzd",BB_OTHER},
 TO={"to","tu",BB_OTHER};
static const char *const BB_PATTERNS[]={
 "TRANSITIVE", "IS / ADJECTIVE", "INSIDE / CLAUSE", "INTRANSITIVE",
 "NEGATION", "FUTURE", "WHY / QUESTION", "IS / QUESTION",
 "IF / THEN", "BECAUSE", "BUT / CONTRAST", "RELATIVE / THAT",
 "WHEN / FUTURE", "PAST NEGATION", "WAS / PAST", "MAY / MODAL",
 "TWO CLAUSES", "USED TO / PAST"
};
#define BB_PATTERNS_N ((int)(sizeof(BB_PATTERNS)/sizeof(BB_PATTERNS[0])))
static unsigned next(bb_mind*m){
 unsigned x=m->rng;x^=x<<13;x^=x>>17;x^=x<<5;
 return m->rng=x?x:0x9E3779B9u;
}
static int roll(bb_mind*m,int n){return (int)(next(m)%(unsigned)n);}
static bb_token make(word w,unsigned role){bb_token x={w.text,w.phones,(uint8_t)role};return x;}
static void add(bb_mind*m,bb_token x){if(m->count<BB_MAX_TOKENS)m->tok[m->count++]=x;}
static void lit(bb_mind*m,const bb_token *t){add(m,*t);}
static bb_token noun(bb_mind*m){
 if(m->force_once){m->force_once=0;return m->forced;}
 int theme=(int)m->theme&3;
 if(roll(m,100)<(int)(m->wander*100.f)){
  /* Recalled nouns attract nearby words toward their original world. */
  theme=(int)m->drift_theme&3;
 }
 if(m->has_memory&&roll(m,100)<(int)(m->recall*100.f))return m->remembered;
 return make(nouns[theme][roll(m,12)],BB_NOUN);
}
static void N(bb_mind*m){add(m,noun(m));}
static void A(bb_mind*m){add(m,make(adj[roll(m,SZ(adj))],BB_ADJ));}
static void I(bb_mind*m,int base){
 verb v=intr[roll(m,SZ(intr))];
 bb_token t={base?v.base:v.third,base?v.base_phones:v.phones,BB_VERB};
 add(m,t);
}
static void subj(bb_mind*m,int with_adj){
 lit(m,&THE);if(with_adj)A(m);N(m);
}
static void obj(bb_mind*m){lit(m,&THE);N(m);}
static void V(bb_mind*m,int base){
 verb v=trans[roll(m,SZ(trans))];
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
 m->grammar=.85f;m->wander=.25f;m->recall=.35f;m->damage=.15f;
 m->length=.4f;m->style=.5f;m->text[0]=0;
}
static void extra_clause(bb_mind*m){
 if(m->count+5>=BB_MAX_TOKENS||m->tok[m->count-1].text!=DOT.text)return;
 m->count--;
 lit(m,&COMMA);lit(m,&AND);subj(m,0);I(m,0);lit(m,&DOT);
}
void bb_generate_pattern(bb_mind*m,int pattern){
 m->count=0;
 m->pattern=(unsigned)(pattern<0?0:pattern%BB_PATTERNS_N);
 switch(m->pattern){
 case 0:subj(m,1);V(m,0);obj(m);lit(m,&DOT);break;
 case 1:subj(m,0);lit(m,&IS);A(m);lit(m,&DOT);break;
 case 2:lit(m,&INSIDE);obj(m);lit(m,&COMMA);subj(m,1);I(m,0);lit(m,&DOT);break;
 case 3:subj(m,0);I(m,0);lit(m,&DOT);break;
 case 4:subj(m,0);lit(m,&DOES);lit(m,&NOT);V(m,1);obj(m);lit(m,&DOT);break;
 case 5:subj(m,0);lit(m,&WILL);I(m,1);lit(m,&DOT);break;
 case 6:lit(m,&WHY);lit(m,&DOES);subj(m,0);V(m,1);obj(m);lit(m,&QUERY);break;
 case 7:lit(m,&IS);subj(m,0);A(m);lit(m,&QUERY);break;
 case 8:lit(m,&IF);subj(m,0);I(m,0);lit(m,&COMMA);subj(m,0);V(m,0);obj(m);lit(m,&DOT);break;
 case 9:subj(m,0);V(m,0);obj(m);lit(m,&BECAUSE);subj(m,0);I(m,0);lit(m,&DOT);break;
 case 10:subj(m,0);I(m,0);lit(m,&COMMA);lit(m,&BUT);subj(m,0);V(m,0);obj(m);lit(m,&DOT);break;
 case 11:subj(m,0);lit(m,&THAT);V(m,0);obj(m);I(m,0);lit(m,&DOT);break;
 case 12:lit(m,&WHEN);subj(m,0);I(m,0);lit(m,&COMMA);subj(m,0);lit(m,&WILL);I(m,1);lit(m,&DOT);break;
 case 13:subj(m,0);lit(m,&DID);lit(m,&NOT);V(m,1);obj(m);lit(m,&DOT);break;
 case 14:subj(m,0);lit(m,&WAS);A(m);lit(m,&DOT);break;
 case 15:subj(m,0);lit(m,&MAY);V(m,1);obj(m);lit(m,&DOT);break;
 case 16:subj(m,0);I(m,0);lit(m,&AND);subj(m,0);I(m,0);lit(m,&DOT);break;
 default:subj(m,0);lit(m,&USED);lit(m,&TO);I(m,1);lit(m,&DOT);break;
 }
 if(m->length>.65f&&m->pattern!=6&&m->pattern!=7&&
    roll(m,100)<(int)((m->length-.55f)*140.f)){
  extra_clause(m);
  if(m->length>.94f&&roll(m,100)<50)extra_clause(m);
 }
 m->force_once=0;
 bb_rebuild(m);
}
const char *bb_pattern_label(const bb_mind*m){
 return BB_PATTERNS[m->pattern%BB_PATTERNS_N];
}
void bb_generate(bb_mind*m){
 if(roll(m,100)<(int)(m->wander*65.f)){
  m->drift_theme=(m->has_memory&&roll(m,100)<(int)(m->recall*100.f))
   ?m->memory_theme:(unsigned)roll(m,4);
 }
 int pattern=roll(m,BB_PATTERNS_N);
 if(m->style<.26f)pattern=roll(m,6);
 else if(m->style>.74f)pattern=6+roll(m,2);
 bb_generate_pattern(m,pattern);
 m->change=0;
 /* DAMAGE: how often a new sentence is mutated as it is made. GRAMMAR (bb_mutate) decides whether a mutation
  * keeps the grammar or breaks it; low GRAMMAR may also mutate twice. */
 if(roll(m,100)<(int)(m->damage*100.f)){
  bb_mutate(m);
  if(m->grammar<.3f&&roll(m,100)<40)bb_mutate(m);
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
void bb_remember(bb_mind*m){
 int ids[BB_MAX_TOKENS],n=0;
 for(unsigned i=0;i<m->count;i++)if(m->tok[i].role==BB_NOUN)ids[n++]=(int)i;
 if(n){
  m->remembered=m->tok[ids[roll(m,n)]];
  m->has_memory=1;
  for(int theme=0;theme<4;theme++)
   for(int w=0;w<12;w++)
    if(m->remembered.text==nouns[theme][w].text)m->memory_theme=(unsigned)theme;
 }
}
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
  }else{bb_rewrite(m,roll(m,2)?0:9);m->change=(unsigned)op+1u;return;}   /* (one noun: a shape with two) */
  break;
 case BB_OP_ADJ:
  if(na)m->tok[adjs[roll(m,na)]]=make(adj[roll(m,SZ(adj))],BB_ADJ);
  else{bb_rewrite(m,roll(m,2)?1:0);m->change=(unsigned)op+1u;return;}    /* (none: a shape with one) */
  break;
 case BB_OP_REBUILD:{
  int to=roll(m,BB_PATTERNS_N);
  if(to==(int)m->pattern)to=(to+1)%BB_PATTERNS_N;
  bb_rewrite(m,to);m->change=(unsigned)op+1u;return;}
 case BB_OP_NEGATE:bb_rewrite(m,m->pattern==4?13:m->pattern==13?4:roll(m,2)?4:13);m->change=(unsigned)op+1u;return;
 case BB_OP_TENSE:{
  static const int modes[]={5,14,17};
  int k=roll(m,3);
  if(modes[k]==(int)m->pattern)k=(k+1)%3;
  bb_rewrite(m,modes[k]);m->change=(unsigned)op+1u;return;}
 case BB_OP_ASK:bb_rewrite(m,m->pattern==6?7:m->pattern==7?6:6+roll(m,2));m->change=(unsigned)op+1u;return;
 case BB_OP_CLAUSE:{
  unsigned before=m->count;
  extra_clause(m);
  if(m->count==before){bb_rewrite(m,roll(m,2)?16:10);m->change=(unsigned)op+1u;return;}   /* (no room, or a question) */
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
/* A mutation by chance: with GRAMMAR up, one that keeps the grammar; down, the ones that break it too. */
void bb_mutate(bb_mind*m){
 static const uint8_t KIND[]={BB_OP_NOUN,BB_OP_SWAP,BB_OP_ADJ,BB_OP_REBUILD,BB_OP_NEGATE,BB_OP_TENSE,
                              BB_OP_STUTTER,BB_OP_DROP,BB_OP_AGAIN,BB_OP_CLAUSE};
 bb_mutate_op(m,KIND[roll(m,m->grammar>.6f?6:10)]);
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
