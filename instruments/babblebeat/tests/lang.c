/* SPDX-License-Identifier: MPL-2.0 */
#include "babble_lang.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 bb_mind a,b;bb_init(&a,42);bb_init(&b,42);
 a.grammar=b.grammar=1.f;a.damage=b.damage=0.f;
 for(int i=0;i<200;i++){
  bb_generate(&a);bb_generate(&b);
  assert(!strcmp(a.text,b.text));
  assert(a.count>2&&a.count<=BB_MAX_TOKENS);
  assert(strlen(a.text)<BB_TEXT_SIZE);
  assert(bb_words(&a)>=2);
  int n=(int)strlen(a.text);
  assert(a.text[n-1]=='.'||a.text[n-1]=='?');
 }
 bb_remember(&a);assert(a.has_memory&&a.remembered.role==BB_NOUN);
 const char *keep=a.remembered.text;
 a.recall=1.f;a.wander=1.f;
 for(int i=0;i<100;i++){bb_generate(&a);assert(strstr(a.text,keep)!=0);}
 a.grammar=0;a.damage=1;
 for(int i=0;i<100;i++){
  bb_generate(&a);assert(a.count>=2&&a.count<=BB_MAX_TOKENS);
  bb_mutate(&a);assert(a.text[0]);
 }
 /* GRAMMAR: every kind of phrase at every one of its five levels. Bounded, said (no empty pronunciation), ended
  * as its kind ends, marked as its kind from SIMPLE up; and the levels differ in what they are: longer upward,
  * a clause joined from JOINED, one nested ("that") at NESTED and nowhere below. */
 {const char *markers[BB_KINDS_N]={0,"why "," not "," was ","if "," will ",0," did not "," used to ","because "," may ","when "};
  long words[BB_LEVELS_N]={0};
  a.damage=0.f;a.length=0.f;a.recall=0.f;
  for(int l=0;l<BB_LEVELS_N;l++){
   a.grammar=(l+.5f)/BB_LEVELS_N;
   assert(bb_level(&a)==l&&bb_level_label(l)[0]);
   for(int p=0;p<BB_KINDS_N;p++)for(int i=0;i<60;i++){
    int end,that=0,commas=0;
    bb_generate_pattern(&a,p);
    assert(a.pattern==(unsigned)p&&a.level==(unsigned)l);
    assert(a.count>=2&&a.count<BB_MAX_TOKENS);
    assert(strlen(a.text)<BB_TEXT_SIZE);
    assert(bb_words(&a)>=(l?3:2));
    for(unsigned k=0;k<a.count;k++){
     assert(a.tok[k].text&&a.tok[k].phones);
     if(a.tok[k].role!=BB_PUNCT)assert(a.tok[k].phones[0]);
     that+=!strcmp(a.tok[k].text,"that");commas+=!strcmp(a.tok[k].text,",");
    }
    end=(int)strlen(a.text)-1;
    assert(a.text[end]==(p==BB_WHY||p==BB_ISIT?'?':'.'));
    assert(bb_pattern_label(&a)[0]);
    if(l&&markers[p])assert(strstr(a.text,markers[p])!=0);
    if(l<4)assert(!that);else assert(that);
    if(l==1&&p!=BB_IF&&p!=BB_WHEN)assert(!commas);
    words[l]+=bb_words(&a);
    /* A rewrite into another kind carries the first noun with it. */
    {const char *first=0;
     for(unsigned k=0;k<a.count;k++)if(a.tok[k].role==BB_NOUN){first=a.tok[k].text;break;}
     assert(first);
     bb_rewrite(&a,(p+1)%BB_KINDS_N);
     assert(strstr(a.text,first)!=0);}
   }
   if(l)assert(words[l]>words[l-1]);
  }
  a.grammar=1.f;}
 /* Coherence-preserving mutations and intentionally destructive mutations */
 a.grammar=1.f;a.length=.8f;
 for(int i=0;i<200;i++){
  bb_generate(&a);bb_mutate(&a);
  assert(a.count>0&&a.count<=BB_MAX_TOKENS);
  assert(strlen(a.text)<BB_TEXT_SIZE);
 }
 a.grammar=0.f;a.damage=1.f;                /* (bare forms, and the mutations that break them) */
 for(int i=0;i<200;i++){
  bb_generate(&a);bb_mutate(&a);
  assert(a.count>0&&a.count<=BB_MAX_TOKENS);
  assert(strlen(a.text)<BB_TEXT_SIZE);
 }
 /* The named mutations (the black keys'): each one changes the sentence, every time, within bounds; the ones
  * with a meaning mean it. */
 a.grammar=1.f;a.damage=0.f;a.length=.3f;a.recall=0.f;a.wander=.3f;
 for(int op=0;op<BB_OPS_N;op++){
  int changed=0;
  assert(bb_op_label(op)[0]);
  for(int i=0;i<300;i++){
   char before[BB_TEXT_SIZE];
   bb_generate(&a);strcpy(before,a.text);
   bb_mutate_op(&a,op);
   assert(a.count>0&&a.count<=BB_MAX_TOKENS&&strlen(a.text)<BB_TEXT_SIZE&&a.text[0]);
   if(strcmp(before,a.text))changed++;
   if(op==BB_OP_NEGATE)assert(strstr(a.text," not ")!=0);
   if(op==BB_OP_ASK)assert(a.text[strlen(a.text)-1]=='?');
   if(op==BB_OP_AGAIN)assert(strstr(a.text," again")!=0);
   if(op==BB_OP_RECALL)assert(a.has_memory&&strstr(a.text,a.remembered.text)!=0);
   if(op==BB_OP_ADJ){int n=0;for(unsigned k=0;k<a.count;k++)n+=a.tok[k].role==BB_ADJ;assert(n>0);}
  }
  /* (a swap of two nouns that are the same word, or a noun drawn again, can leave the words as they were) */
  assert(changed>(op==BB_OP_RECALL?60:200));
 }
 /* A key's word. Every kind of phrase made about a word has the word in it; the word next to it is another word,
  * and a step back from there can be the word again (a walk along its world, not a jump). */
 {bb_mind k;bb_init(&k,99);k.recall=0;
  assert(bb_worlds()>=4&&bb_world_size(0)==12&&bb_world_label(0)[0]);
  for(int w=0;w<bb_worlds()*40;w++){
   bb_token word=bb_noun_of(w/40,w*7),nearby;
   assert(word.text&&word.role==BB_NOUN);
   for(int pattern=0;pattern<BB_KINDS_N;pattern++){bb_phrase(&k,&word,pattern);assert(strstr(k.text,word.text)!=0);}
   bb_phrase(&k,&word,bb_statement(&k));assert(k.text[strlen(k.text)-1]=='.');
   nearby=bb_noun_near(&k,&word);assert(nearby.text&&nearby.text!=word.text);
  }
  bb_token word=bb_noun_of(1,3);bb_remember_word(&k,&word);assert(k.has_memory&&k.remembered.text==word.text);}
 /* RECALL's top end: the sentences loop back on themselves (longer, the opening said again), still bounded and
  * still ended; under 70 they do not. */
 {bb_mind k;long plain=0,looped=0;int again=0;
  bb_init(&k,7);k.damage=0;k.length=0;k.recall=.5f;
  for(int i=0;i<300;i++){bb_generate(&k);plain+=(long)k.count;}
  k.recall=1.f;
  for(int i=0;i<300;i++){
   bb_generate(&k);looped+=(long)k.count;
   assert(k.count<=BB_MAX_TOKENS);
   {size_t n=strlen(k.text);assert(k.text[n-1]=='.'||k.text[n-1]=='?');}
   assert(k.has_memory);
   {const char *c=strstr(k.text,", ");if(c&&!strncmp(c+2,k.text,4))again++;}
  }
  assert(looped>plain+600);assert(again>150);}
 /* The whole lexicon: every world's every noun has a text the screen can show and a spelling the voice can say
  * (the voice's own letters and no others), and makes a sentence that fits; so do sentences at large, which
  * draw on every adjective and verb. */
 {bb_mind k;const char *voice="iIeAaOUVuEo12345wyrlmnNsSfTzvDhptkbdgCJ";long nouns=0;
  bb_init(&k,2026);k.recall=0;k.damage=0;k.wander=1.f;
  for(int t=0;t<bb_worlds();t++)for(int i=0;i<bb_world_size(t);i++){
   bb_token word=bb_noun_of(t,i);
   assert(word.text&&word.text[0]&&strlen(word.text)<=12&&word.phones[0]);
   for(const char *c=word.phones;*c;c++)assert(strchr(voice,*c));
   for(const char *c=word.text;*c;c++)assert(*c>='a'&&*c<='z');
   nouns++;
  }
  assert(nouns>1500);
  for(int i=0;i<40000;i++){
   k.grammar=(float)(i%5)/5.f+.1f;k.theme=(unsigned)(i%bb_worlds());k.length=(i&1)?1.f:0.f;
   bb_generate(&k);
   assert(k.count>=2&&k.count<=BB_MAX_TOKENS&&strlen(k.text)<BB_TEXT_SIZE);
   for(unsigned j=0;j<k.count;j++)for(const char *c=k.tok[j].phones;*c;c++)assert(strchr(voice,*c));
  }}
 puts("BabbleBeat grammar tests passed");
}
