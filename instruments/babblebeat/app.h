/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#include "plat.h"
#define APP_NAME "BABBLEBEAT"
#define APP_SPLASH_RGB RGB(237,158,117)
#define APP_PALETTE 2u
#define APP_BLOCK 256u
/* Preserve the platform's panel calibration. No patch store in v0.1. */
enum {OBJ_PANEL,OBJ_NOBJ};
#define APP_STORE_MAP {0xDC000u}
void app_init(void);
void app_render(int32_t *lr,uint32_t n,uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a,const char *b);
#define APP_HOST_EXPECT
int app_expect(const char *what,int *got);
