/* SPDX-License-Identifier: MPL-2.0
 * PALINDROME as one of STRABBLEDROME's instruments: its own app.c, unchanged, as a unit of its own, its calls under its
 * own name and the platform reached through the launcher (../engine.h). */
#include "../engine.h"
#define APP_NAME "PALINDROME"
#define app_init palindrome_init
#define app_ui_init palindrome_ui_init
#define app_frame palindrome_frame
#define app_input_only palindrome_input_only
#define app_panic palindrome_panic
#define app_render palindrome_render
#define app_say palindrome_say
#define app_expect palindrome_expect
#define app_overload palindrome_overload
#include "../../palindrome/app.c"
