
#ifdef UI_APP_CODE
#include "apps/clock.c"
#include "apps/bouncy.c"
#include "apps/todo.c"
#include "apps/spirals.c"
#include "apps/wifi.c"
#if CTX_FLOW3R
#include "apps/captouch.c"
#endif
#ifdef APP_SSH
#include "apps/term.c"
#endif
#endif

#if CTX_FLOW3R
UI_APP("captouch", NULL, view_captouch, "apps")
#endif


UI_APP("clock",    NULL, view_clock, "apps")
UI_APP("bouncy",   NULL, view_bouncy, "apps")
UI_APP("todo",     NULL, view_todo, "apps")
UI_APP("spirals",  NULL, view_spirals, "spirals")
UI_APP("wifi",     NULL, view_wifi, "os")

#ifdef APP_SSH
UI_APP("term",     NULL, view_term, "apps")
#endif


