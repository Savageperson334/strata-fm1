/* SPDX-License-Identifier: MPL-2.0
 * Tiny deterministic grammar compiler. A hand-curated lexicon with compact
 * approximated phoneme spellings, not the full WordNet / CMUdict dataset.
 * Pronunciation: a=ah A=ae e=eh i=ee I=ih o=oh O=aw u=oo U=uh E=er.
 * S=sh T=unvoiced th D=voiced th C=ch J=j N=ng; others ~ as written. */
#include "babble_lang.h"
typedef struct {const char *text,*phones;} word;
typedef struct {const char *third,*base,*phones,*base_phones;} verb;
#define SZ(a) ((int)(sizeof(a)/sizeof((a)[0])))
static const word nouns[4][12]={
 {{"machine","mUSin"},{"signal","sIgnUl"},{"circuit","sErkIt"},{"engine","enJIn"},{"robot","robOt"},{"speaker","spikEr"},{"voltage","voltIJ"},{"static","stAtIk"},{"terminal","tErmInUl"},{"mirror","mIrEr"},{"antenna","AntenU"},{"filter","fIltEr"}},
 {{"ghost","gost"},{"shadow","SAdO"},{"moon","mun"},{"window","wIndO"},{"planet","plAnIt"},{"garden","gardUn"},{"ritual","rICUUl"},{"memory","memUri"},{"door","dor"},{"fog","fog"},{"language","lANgwIJ"},{"dream","drim"}},
 {{"mouth","mowT"},{"throat","Trot"},{"tongue","tuN"},{"bone","bon"},{"skin","skIn"},{"nerve","nErv"},{"lung","luN"},{"insect","Insekt"},{"vowel","vowUl"},{"breath","breT"},{"spine","spIn"},{"voice","vois"}},
 {{"castle","kAsUl"},{"coffin","kOfIn"},{"vampire","vAmpIr"},{"crypt","krIpt"},{"blood","blUd"},{"wolf","wUlf"},{"night","nIt"},{"diary","dIEri"},{"chapel","CApUl"},{"letter","letEr"},{"count","kownt"},{"grave","grav"}}
};
static const word adj[]={
 {"pale","pal"},{"electric","IlektrIk"},{"ancient","anSInt"},{"silent","sIlInt"},
 {"hidden","hIdUn"},{"crooked","krUkId"},{"synthetic","sInTetIk"},
 {"hollow","hOlO"},{"broken","brokUn"},{"restless","restlIs"},
 {"silver","sIlvEr"},{"strange","stranJ"}
};
static const word intr[]={
 {"waits","wats"},{"sleeps","slips"},{"whispers","wIspErz"},
 {"dreams","drimz"},{"shivers","SIvErz"},{"sings","sINz"},
 {"returns","rItErnz"},{"vanishes","vAnISIz"}
};
static const verb trans[]={
 {"remembers","remember","rImembErz","rImembEr"},
 {"follows","follow","fOlOz","fOlO"},{"haunts","haunt","hOnts","hOnt"},
 {"watches","watch","wOCIz","wOC"},{"forgets","forget","fOrgEts","fOrgEt"},
 {"opens","open","opUnz","opUn"},{"swallows","swallow","swOlOz","swOlO"},
 {"corrupts","corrupt","kOrUpts","kOrUpt"}
};
static const bb_token THE={"the","DU",BB_OTHER},IS={"is","Iz",BB_OTHER},
 NOT={"not","nOt",BB_OTHER},INSIDE={"inside","InsId",BB_OTHER},
 DO={"do","du",BB_OTHER},ONLY={"only","onli",BB_OTHER},
 NO={"no","no",BB_OTHER},AND={"and","And",BB_OTHER},
 WHY={"why","wI",BB_OTHER},DOES={"does","dUz",BB_OTHER},
 AGAIN={"again","Ugen",BB_OTHER},DOT={".","",BB_PUNCT},
 QUERY={"?","",BB_PUNCT};
static unsigned next(bb_mind*m){
 unsigned x=m->rng;x^=x<<13;x^=x>>17;x^=x<<5;
 return m->rng=x?x:0x9E3779B9u;
}
static int roll(bb_mind*m,int n){return (int)(next(m)%(unsigned)n);}
static bb_token make(word w,unsigned role){bb_token x={w.text,w.phones,(uint8_t)role};return x;}
static void add(bb_mind*m,bb_token x){if(m->count<BB_MAX_TOKENS)m->tok[m->count++]=x;}
static void lit(bb_mind*m,const bb_token *t){add(m,*t);}
static bb_token noun(bb_mind*m){
 int theme=(int)m->theme&3;
 if(roll(m,100)<(int)(m->wander*100))theme=roll(m,4);
 if(m->has_memory&&roll(m,100)<(int)(m->recall*100))return m->remembered;
 return make(nouns[theme][roll(m,12)],BB_NOUN);
}
static void N(bb_mind*m){add(m,noun(m));}
static void A(bb_mind*m){add(m,make(adj[roll(m,SZ(adj))],BB_ADJ));}
static void I(bb_mind*m){add(m,make(intr[roll(m,SZ(intr))],BB_VERB));}
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
 m->remembered=(bb_token){0,0,BB_NOUN};
 m->grammar=.85f;m->wander=.25f;m->recall=.35f;m->damage=.15f;
 m->length=.4f;m->style=.5f;m->text[0]=0;
}
static void second_clause(bb_mind*m){
 if(m->count+6>=BB_MAX_TOKENS)return;
 bb_token stop=m->tok[--m->count];
 lit(m,&AND);lit(m,&THE);N(m);I(m);add(m,stop);
}
void bb_generate(bb_mind*m){
 m->count=0;
 int pattern=roll(m,8);
 if(m->style<.33f)pattern=roll(m,4);
 else if(m->style>.69f)pattern=6+roll(m,2);
 switch(pattern){
 case 0:lit(m,&THE);A(m);N(m);V(m,0);lit(m,&THE);N(m);lit(m,&DOT);break;
 case 1:lit(m,&THE);N(m);lit(m,&IS);A(m);lit(m,&DOT);break;
 case 2:lit(m,&INSIDE);lit(m,&THE);N(m);lit(m,&THE);A(m);N(m);I(m);lit(m,&DOT);break;
 case 3:lit(m,&THE);N(m);I(m);lit(m,&DOT);break;
 case 4:lit(m,&DO);lit(m,&NOT);V(m,1);lit(m,&THE);N(m);lit(m,&DOT);break;
 case 5:lit(m,&NO);N(m);lit(m,&ONLY);lit(m,&THE);N(m);lit(m,&DOT);break;
 case 6:lit(m,&WHY);lit(m,&DOES);lit(m,&THE);N(m);V(m,1);lit(m,&THE);N(m);lit(m,&QUERY);break;
 default:lit(m,&IS);lit(m,&THE);N(m);A(m);lit(m,&QUERY);break;
 }
 if(pattern<4&&m->length>.67f&&roll(m,100)<(int)(m->length*100))
  second_clause(m);
 if(roll(m,100)<(int)((1.f-m->grammar)*m->damage*100.f)){
  bb_mutate(m);
  if(m->grammar<.3f&&roll(m,100)<40)bb_mutate(m);
 }
 bb_rebuild(m);
}
void bb_remember(bb_mind*m){
 int ids[BB_MAX_TOKENS],n=0;
 for(unsigned i=0;i<m->count;i++)if(m->tok[i].role==BB_NOUN)ids[n++]=(int)i;
 if(n){m->remembered=m->tok[ids[roll(m,n)]];m->has_memory=1;}
}
void bb_mutate(bb_mind*m){
 int nouns[BB_MAX_TOKENS],nn=0,adjs[BB_MAX_TOKENS],na=0;
 for(unsigned i=0;i<m->count;i++){
  if(m->tok[i].role==BB_NOUN)nouns[nn++]=(int)i;
  if(m->tok[i].role==BB_ADJ)adjs[na++]=(int)i;
 }
 int op=roll(m,m->grammar>.6f?3:6);
 if(op==0&&nn)m->tok[nouns[roll(m,nn)]]=noun(m);
 else if(op==1&&nn>1){
  int a=nouns[roll(m,nn)],b=nouns[roll(m,nn)];
  bb_token t=m->tok[a];m->tok[a]=m->tok[b];m->tok[b]=t;
 }else if(op==2&&na)m->tok[adjs[roll(m,na)]]=make(adj[roll(m,SZ(adj))],BB_ADJ);
 else if(op==3&&m->count<BB_MAX_TOKENS-1&&m->count>2){
  int at=roll(m,(int)m->count-1);
  for(int i=(int)m->count;i>at;i--)m->tok[i]=m->tok[i-1];
  m->count++;
 }else if(op==4&&m->count>3){
  int at=roll(m,(int)m->count-1);
  for(unsigned i=(unsigned)at;i<m->count-1;i++)m->tok[i]=m->tok[i+1];
  m->count--;
 }else if(op==5&&m->count<BB_MAX_TOKENS-1){
  bb_token stop=m->tok[m->count-1];m->tok[m->count-1]=AGAIN;
  m->tok[m->count++]=stop;
 }else if(nn&&m->has_memory)m->tok[nouns[roll(m,nn)]]=m->remembered;
 else if(nn)m->tok[nouns[roll(m,nn)]]=noun(m);
 bb_rebuild(m);
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
