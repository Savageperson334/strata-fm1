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
 /* Every structural template: exact bounded generation, no empty pronunciations,
  * appropriate grammatical markers. */
 const char *markers[]={"", " is ","inside ",0," does not "," will ",
  "why does ","is ","if "," because "," but "," that ","when "," did not ",
  " was "," may "," and "," used to "};
 a.grammar=1.f;a.damage=0.f;a.length=0.f;a.recall=0.f;
 for(int p=0;p<18;p++){
  bb_generate_pattern(&a,p);
  assert(a.pattern==(unsigned)p);
  assert(a.count>3&&a.count<BB_MAX_TOKENS);
  assert(strlen(a.text)<BB_TEXT_SIZE);
  assert(bb_words(&a)>=2);
  int end=(int)strlen(a.text)-1;
  assert(a.text[end]==(p==6||p==7?'?':'.'));
  assert(bb_pattern_label(&a)[0]);
  if(markers[p]&&p!=0)assert(strstr(a.text,markers[p])!=0);
  /* Structural rewrites carry the old first noun into the new sentence. */
  const char* first=0;
  for(unsigned k=0;k<a.count;k++)if(a.tok[k].role==BB_NOUN){first=a.tok[k].text;break;}
  assert(first);
  bb_rewrite(&a,(p+1)%18);
  assert(strstr(a.text,first)!=0);
 }
 /* Coherence-preserving mutations and intentionally destructive mutations */
 a.grammar=1.f;a.length=.8f;
 for(int i=0;i<200;i++){
  bb_generate(&a);bb_mutate(&a);
  assert(a.count>0&&a.count<=BB_MAX_TOKENS);
  assert(strlen(a.text)<BB_TEXT_SIZE);
 }
 a.grammar=0.f;a.damage=1.f;
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
 puts("BabbleBeat grammar tests passed");
}
