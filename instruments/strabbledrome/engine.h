/* SPDX-License-Identifier: MPL-2.0
 *
 * An instrument's side of shim.h: included first in the unit that then includes the instrument's own app.c, it
 * gives the launcher's functions the names the instrument's code calls the platform by. The instrument's source
 * is not changed. */
#pragma once
#include "app.h"                         /* the launcher's: plat.h, the block size all three share, STRATA's store */
#include "fm1b_config.h"
#include "shim.h"
#ifdef PLAT_HOST
#include <string.h>                      /* (the simulator's checks: strcmp, atoi) */
#include <stdlib.h>
#endif
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#define C_BLACK 0x0000u
#define C_WHITE 0xFFFFu
#define C_LINE sdr_color(0)
#define C_DIM sdr_color(1)
#define C_GRAY sdr_color(2)
#define C_AMB sdr_color(3)
#define C_HI sdr_color(4)
#define felucca_font_t sdr_font
#define FONT_XS SDR_FONT[0]
#define FONT_B SDR_FONT[1]
#define cv_begin sdr_cv_begin
#define cv_blit sdr_cv_blit
#define cv_rect sdr_cv_rect
#define cv_line sdr_cv_line
#define cv_text sdr_cv_text
#define text_w sdr_text_w
#define lcd_fill sdr_lcd_fill
#define ANN_NOW 1u
#define ann_on sdr_ann_on
#define ann_where sdr_ann_where
#define ann_say sdr_ann_say
#define ann_tick sdr_ann_tick
#define plat_buttons sdr_buttons
#define plat_keys sdr_keys
#define plat_enc sdr_enc
#undef APP_NAME
