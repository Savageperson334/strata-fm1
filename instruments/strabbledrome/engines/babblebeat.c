/* SPDX-License-Identifier: MPL-2.0
 * BABBLEBEAT as one of STRABBLEDROME's instruments: its own app.c, unchanged, as a unit of its own, its calls under its
 * own name and the platform reached through the launcher (../engine.h). */
#include "../engine.h"
#define APP_NAME "BABBLEBEAT"
#define app_init babblebeat_init
#define app_ui_init babblebeat_ui_init
#define app_frame babblebeat_frame
#define app_input_only babblebeat_input_only
#define app_panic babblebeat_panic
#define app_render babblebeat_render
#define app_say babblebeat_say
#define app_expect babblebeat_expect
#define app_overload babblebeat_overload
#include "../../babblebeat/app.c"
