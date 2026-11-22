/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#define BB_MAX_TOKENS 24
#define BB_TEXT_SIZE 192
enum {BB_OTHER,BB_NOUN,BB_ADJ,BB_VERB,BB_PUNCT};
typedef struct {const char *text,*phones;uint8_t role;} bb_token;
typedef struct {
 uint32_t rng;bb_token tok[BB_MAX_TOKENS],remembered;
 unsigned count,theme,has_memory,pattern,drift_theme,memory_theme,force_once,change;
 bb_token forced;
 float grammar,wander,recall,damage,length,style;
 char text[BB_TEXT_SIZE];
} bb_mind;
void bb_init(bb_mind*,uint32_t seed);
void bb_generate(bb_mind*);
void bb_generate_pattern(bb_mind*,int pattern);
void bb_rewrite(bb_mind*,int pattern);
const char *bb_pattern_label(const bb_mind*);
void bb_mutate(bb_mind*);
void bb_remember(bb_mind*);
void bb_rebuild(bb_mind*);
int bb_words(const bb_mind*);
const bb_token *bb_word_at(const bb_mind*,int i);
