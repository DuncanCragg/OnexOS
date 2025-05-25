#ifndef USER_2D_H
#define USER_2D_H


#define USER_EVENT_NONE_AL (void*)0
#define USER_EVENT_INITIAL (void*)1
#define USER_EVENT_BUTTON  (void*)2
#define USER_EVENT_TOUCH   (void*)3
#define USER_EVENT_LOG     (void*)4

// ------------ events REVISIT: should be passed to user eval in arg as struct pointer

extern volatile bool         button_pending;
extern volatile bool         button_pressed;
extern volatile touch_info_t touch_info;
extern volatile bool         touch_down;

extern bool display_on;


#endif
