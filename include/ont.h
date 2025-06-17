#ifndef ONT_H
#define ONT_H

#include <onn.h>

/* Default, hard-coded behaviour evaluators */

bool evaluate_light_logic(object* o, void* d);

bool evaluate_clock_sync(object* o, void* d);

bool evaluate_clock(object* o, void* d);

#endif
