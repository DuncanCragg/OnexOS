#ifndef USER_2D_H
#define USER_2D_H


#define USER_EVENT_INITIAL (void*)0
#define USER_EVENT_BUTTON  (void*)1
#define USER_EVENT_TOUCH   (void*)2
#define USER_EVENT_LOG     (void*)3

// ------------ events REVISIT: should be passed to user eval in arg as struct pointer

extern volatile bool         button_pending;
extern volatile bool         button_pressed;
extern volatile touch_info_t touch_info;
extern volatile bool         touch_down;
extern volatile uint32_t     touch_events;
extern volatile uint32_t     touch_events_seen;
extern volatile uint32_t     touch_events_spurious;

extern bool user_active; // REVISIT: rename to display_on, but also rethink it


#endif
