/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#include "babble_lang.h"
#define BB_QUEUE 256
typedef struct {
 char queue[BB_QUEUE];
 uint16_t used,pos;
 uint32_t remaining,total,noise;
 float phase,freq,source_gain,noise_gain,noise_lp;
 float state[3][2],coef[3],r2[3],input[3];
 float pitch,rate,tone,breath,volume,stutter,inflect;
 int note,word_number;
} bb_voice;
void bb_voice_init(bb_voice*);
void bb_voice_stop(bb_voice*);
void bb_voice_controls(bb_voice*,const float knobs[7]);
void bb_voice_say(bb_voice*,const bb_mind*,int note);
void bb_voice_word(bb_voice*,const bb_token*,int note);
int bb_voice_active(const bb_voice*);
float bb_voice_sample(bb_voice*);
