/* SPDX-License-Identifier: MPL-2.0
 * STRATA as one of STRABBLEDROME's instruments: its own app.c, unchanged, as a unit of its own, its calls under its
 * own name and the platform reached through the launcher (../engine.h). */
#include "../engine.h"
#define APP_NAME "STRATA"
#define app_init strata_init
#define app_ui_init strata_ui_init
#define app_frame strata_frame
#define app_input_only strata_input_only
#define app_panic strata_panic
#define app_render strata_render
#define app_say strata_say
#define app_expect strata_expect
#define app_overload strata_overload
#include "../../../app/app.c"
