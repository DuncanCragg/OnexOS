#ifndef EVALUATORS_H
#define EVALUATORS_H

#include <onn.h>

void evaluators_init();

bool evaluate_battery_in(object* bat, void* d);
bool evaluate_compass_in(object* com, void* d);
bool evaluate_bcs_in(object* bcs, void* d);
bool evaluate_touch_in(object* tch, void* d);
bool evaluate_button_in(object* btn, void* d);
bool evaluate_about_in(object* abt, void* d);
bool evaluate_backlight_out(object* blt, void* d);
bool evaluate_motion_in(object* mtn, void* d);

#endif
