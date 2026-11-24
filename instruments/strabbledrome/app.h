/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#include "plat.h"
#define APP_NAME "STRABBLEDROME"
#define APP_SPLASH_RGB RGB(200, 200, 210)
#define APP_PALETTE 2u                   /* as all three instruments: the screen in cyan */
#define APP_BLOCK 256u                   /* as all three: 5.8 ms a block */
#define APP_OVERLOAD                     /* the render ran late: STRATA lets a voice go (app_overload) */
void app_overload(void);
/* STRATA's store map and objects, so that its saved sound is where STRATA will look for it; the launcher itself
 * writes nothing but what the platform does (the panel's calibration) */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u, 0xDC000u}
void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
