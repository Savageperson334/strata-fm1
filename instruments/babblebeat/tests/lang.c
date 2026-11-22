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
 puts("BabbleBeat grammar tests passed");
}
