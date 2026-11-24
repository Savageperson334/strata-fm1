/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#define BB_MAX_TOKENS 24
#define BB_TEXT_SIZE 192
enum {BB_OTHER,BB_NOUN,BB_ADJ,BB_VERB,BB_PUNCT};
/* the kinds of phrase, and how many ways GRAMMAR has of building each (bare .. nested) */
enum {BB_STATE,BB_WHY,BB_NOT,BB_WAS,BB_IF,BB_WILL,BB_ISIT,BB_DIDNOT,BB_USEDTO,BB_BECAUSE,BB_MAY,BB_WHEN,BB_KINDS_N};
#define BB_LEVELS_N 5
typedef struct {const char *text,*phones;uint8_t role;} bb_token;
typedef struct {
 uint32_t rng;bb_token tok[BB_MAX_TOKENS],remembered;
 unsigned count,theme,has_memory,pattern,level,drift_theme,memory_theme,force_once,change;
 bb_token forced;
 float grammar,wander,recall,damage,length,style;
 char text[BB_TEXT_SIZE];
} bb_mind;
void bb_init(bb_mind*,uint32_t seed);
void bb_generate(bb_mind*);
int bb_level(const bb_mind*);            /* GRAMMAR's level now, 0 bare .. 4 nested (a phrase's own: its level field) */
const char *bb_level_label(int level);
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
/* a key's word: the nouns of a world in order; the word next to one in its world (how a key's word moves: a
 * step, not a jump); a sentence of a pattern whose first noun is the word; that word made the memory */
bb_token bb_noun_of(int theme,int i);
int bb_worlds(void);                     /* how many worlds of nouns there are (world 0: ALL, every world's nouns one
                                          * after another); a world's name; how many nouns in it */
const char *bb_world_label(int theme);
int bb_world_size(int theme);
bb_token bb_noun_near(bb_mind*,const bb_token *word);
void bb_phrase(bb_mind*,const bb_token *word,int pattern);
void bb_remember_word(bb_mind*,const bb_token *word);
int bb_statement(bb_mind*);              /* a plain statement's pattern, by chance (the white keys' kind) */
int bb_chance(bb_mind*,float p);         /* 1 with probability p */
void bb_rebuild(bb_mind*);
int bb_words(const bb_mind*);
const bb_token *bb_word_at(const bb_mind*,int i);
