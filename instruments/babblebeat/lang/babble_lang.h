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
/* the named mutations (the black keys', in this order) */
enum {BB_OP_NOUN,BB_OP_SWAP,BB_OP_ADJ,BB_OP_REBUILD,BB_OP_NEGATE,BB_OP_TENSE,BB_OP_ASK,BB_OP_CLAUSE,BB_OP_RECALL,
      BB_OP_STUTTER,BB_OP_DROP,BB_OP_AGAIN,BB_OPS_N};
void bb_mutate_op(bb_mind*,int op);
const char *bb_op_label(int op);
void bb_remember(bb_mind*);
void bb_rebuild(bb_mind*);
int bb_words(const bb_mind*);
const bb_token *bb_word_at(const bb_mind*,int i);
