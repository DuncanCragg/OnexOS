#ifndef USER_2D_H
#define USER_2D_H


#define USER_EVENT_NONE_AL  0 // 0b00000000
#define USER_EVENT_NONE     1 // 0b00000001 // USER_EVENT_NONE_AL in pending events bitmask
#define USER_EVENT_INITIAL  2 // 0b00000010
#define USER_EVENT_BUTTON   4 // 0b00000100
#define USER_EVENT_TOUCH    8 // 0b00001000
#define USER_EVENT_LOG     16 // 0b00010000

// ------------ events REVISIT: should be passed to user eval in arg as struct pointer

extern volatile bool         button_pressed;
extern volatile touch_info_t touch_info;
extern volatile bool         touch_down;
extern volatile uint8_t      pending_user_event;
extern volatile uint32_t     pending_user_event_time;

extern bool display_on;

extern bool evaluate_user_2d(object* o, void* d);


#endif
