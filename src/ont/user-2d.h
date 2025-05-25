#ifndef USER_2D_H
#define USER_2D_H


#define USER_EVENT_NONE_AL 0
#define USER_EVENT_INITIAL 1
#define USER_EVENT_BUTTON  2
#define USER_EVENT_TOUCH   4
#define USER_EVENT_LOG     8
#define USER_EVENT_NONE    16 // for USER_EVENT_NONE_AL in bitmask for pending events

// ------------ events REVISIT: should be passed to user eval in arg as struct pointer

extern volatile bool         button_pending;
extern volatile bool         button_pressed;
extern volatile touch_info_t touch_info;
extern volatile bool         touch_down;
extern volatile uint8_t      pending_user_event;
extern volatile uint32_t     pending_user_event_time;

extern bool display_on;


#endif
