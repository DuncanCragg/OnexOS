
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <items.h>
#include <onex-kernel/mem.h>
#include <onex-kernel/lib.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include <onex-kernel/touch.h>

#include <onn.h>

#include <mathlib.h>
#include <g2d.h>

#define nBIG_LOG

// -------------------- User --------------------------

extern uint16_t word_index;
extern bool     in_word;
extern char     edit_word[];
extern uint8_t  cursor;

#define KBDSTART_X 0
#define KBDSTART_Y 120
extern uint8_t kbd_page;
extern int16_t kbd_x;
extern int16_t kbd_y;

extern int16_t text_scroll_offset;

extern char*    add_this_word;
extern uint16_t del_this_word;

extern void show_keyboard(uint8_t g2d_node);

extern char* event_log_buffer;

extern bool button_pending;
extern bool button_pressed;

extern touch_info_t touch_info;
extern bool         touch_down;

extern char* homeuid;
extern char* inventoryuid;

extern object* user;
extern object* responses;
extern char*   useruid;

static object* get_or_create_edit(char* uid) {

  object* edit;
  char* edit_uid=object_property(responses, uid);
  if(edit_uid){
    edit=onex_get_from_cache(edit_uid);
  }
  else{
    edit=object_new(0, 0, "edit rule", 5);

    object_property_set(edit, "edit-target", uid);
    object_property_set(edit, "edit-user",   useruid);
    object_property_add(edit, "Notifying",   uid);

    edit_uid=object_property(edit, "UID");
    object_property_set(responses, uid, edit_uid);
  }
  return edit;
}

static void set_edit_object(char* uid, char* key, uint16_t i, char* fmt, ...){

  #define MAX_UPDATE_PROPERTY_LEN 256
  char upd[MAX_UPDATE_PROPERTY_LEN];

  va_list args;
  va_start(args, fmt);
  size_t s=vsnprintf(upd, MAX_UPDATE_PROPERTY_LEN, fmt, args);
  if(s>=MAX_UPDATE_PROPERTY_LEN){ log_write("edit o/f"); return; }
  va_end(args);

  object* edit=get_or_create_edit(uid);

  char keypath[32];
  if(i) snprintf(keypath, 32, "%s\\:%d", key, i);
  else  snprintf(keypath, 32, "%s",      key);

  // XXX using 4 to index, and what about multiple props?
  char prevkey[64]; object_property_key_esc(edit, ":", 4, prevkey, 64);
  if(*prevkey) object_property_set(edit, prevkey, 0);

  object_property_set(edit, keypath, upd);
}

static object* create_new_object_like_others() {

  char* newtype="list editable";

  if(object_property_contains(user, "viewing:is", "text")){
    newtype="text editable";
  }
  else{
    uint16_t ll = object_property_length(user, "viewing:list");
    if(ll==0){
      newtype="text editable";
    }
    else{
      if( object_property_contains(user, "viewing:list:1:is", "text") &&
         !object_property_contains(user, "viewing:list:1:is", "list")    ){

        newtype="text editable";
      }
    }
  }

  object* r=object_new(0, 0, newtype, 4);

  if(!strcmp(newtype, "text editable")){
    object_set_evaluator(r, "notes");
    object_property_set(r, "text", "--");
  }
  else{
    object_set_evaluator(r, "editable");
  //object_property_set(r, "list", "--");
  }

  if(r) onex_run_evaluators(object_property(r, "UID"), 0);

  return r;
}

static void best_propname_for_link_drop(char* propname, uint16_t len, uint16_t index){
  char targetis[64]; snprintf(targetis, 64, "viewing:list:%d:is", index);
  if(object_property_contains(user, targetis, "list")){
    snprintf(propname, len, "list");
    return;
  }
  uint16_t ln=object_property_length(user, "inventory:list:1:is");
  uint16_t s=0;
  for(uint16_t j=1; j <= ln; j++){
    char* is=object_property_get_n(user, "inventory:list:1:is", j);
    if(!strcmp(is, "editable")) continue;
    s+=snprintf(propname+s, len-s, "%s", is);
    s+=snprintf(propname+s, len-s, "-");
  }
  *(propname+s-1)=0;
}

static void show_gfx_log(uint8_t root_g2d_node){

  uint64_t pre_render_time=time_ms();

  #define LOG_DECAY_TIME 5
  #define LOG_LINES_MAX 5
  #define LOG_LEN 35
  static char*    log_lines[LOG_LINES_MAX];
  static uint8_t  log_lines_index=0;
  static uint64_t log_last=0;
  if(event_log_buffer){

    char*   nextline=(char*)event_log_buffer;
    uint8_t remaininglen=strlen(nextline);
    do{
      uint8_t nextlinelen=min(remaininglen, LOG_LEN);

      if(log_lines[log_lines_index]) free(log_lines[log_lines_index]);
      log_lines[log_lines_index]=strndup(nextline, nextlinelen);
      log_lines_index=(log_lines_index+1)%LOG_LINES_MAX;

      nextline+=nextlinelen;
      remaininglen-=nextlinelen;

    } while(remaininglen);

    event_log_buffer=0;
    log_last=pre_render_time;
  }
  else
  if(log_last && pre_render_time > log_last+LOG_DECAY_TIME*1000){
    for(uint8_t i=0; i<LOG_LINES_MAX-1; i++){
      log_lines[i]=log_lines[i+1];
    }
    log_lines[LOG_LINES_MAX-1]=0;
    log_lines_index=max(log_lines_index-1, 0);
    log_last=pre_render_time-1;
  }
  for(uint8_t i=0; i<LOG_LINES_MAX; i++){
    if(log_lines[i]){
#if !defined(BIG_LOG)
      g2d_node_text(root_g2d_node, 20,240+i*8, G2D_RED, G2D_BLACK, 1, "%s", log_lines[i]);
#else
      g2d_node_text(root_g2d_node, 40,i*15, G2D_RED, G2D_BLACK, 2, "%s", log_lines[i]);
#endif
    }
  }
}

static void show_touch_point(uint8_t g2d_node){
  g2d_node_rectangle(g2d_node, 0,touch_info.y, 240,1, G2D_MAGENTA);
  g2d_node_rectangle(g2d_node, touch_info.x,0, 1,280, G2D_MAGENTA);
}

static uint16_t inv_grab_control=0;
static uint16_t inv_grab_index=0;
static bool     inv_delete=false;
static uint16_t inv_drop_control=0;
static uint16_t inv_drop_index=0;

static uint16_t list_selected_control=0;
static uint16_t list_selected_index=0;
static uint16_t prop_selected_index=0;

static bool     showing_title_editor=false;

#define NUM_LINK_TYPES 11
static char* testlinktypes[] = {
               "user",
               "battery",
               "touch",
               "button",
               "light",
               "clock",
               "watchface",
               "watch",
               "text",
               "about",
               "list",
               ""
};
static char*    linktypes[NUM_LINK_TYPES];
static uint8_t  numlinktypes=0;

static void get_linktypes(char* path){
  numlinktypes=0;
  char linktypeis[64];
  for(uint8_t i=0; i<NUM_LINK_TYPES; i++){
    snprintf(linktypeis, 64, "%s:is", testlinktypes[i]);
    if(object_pathpair_contains(user, path, linktypeis, testlinktypes[i])){
      linktypes[numlinktypes++]=testlinktypes[i];
    }
  }
}

static int16_t  scroll_bot_lim=0;
static bool     scroll_top=false;
static bool     scroll_bot=false;
static bool     scrolling=false;
static int16_t  scroll_offset=0;

static uint16_t swipe_control=0;
static uint16_t swipe_index=0;
static int16_t  swipe_offset=0;

#define VIEW_SLID_NOWHERE   0

#define VIEW_SLIDING_LEFT   1
#define VIEW_SLIDING_RIGHT  2
#define VIEW_SLIDING_UP     3
#define VIEW_SLIDING_DOWN   4

#define VIEW_SLID_IN_PLACE  5 // >= means one of these

#define VIEW_SLID_LEFT      5
#define VIEW_SLID_RIGHT     6
#define VIEW_SLID_UP        7
#define VIEW_SLID_DOWN      8

static int16_t view_offset_x=0;
static int16_t view_offset_y=0;
static uint8_t view_sliding_direction=VIEW_SLID_NOWHERE;

static void reset_viewing_state_variables(){

  numlinktypes=0;

  view_offset_x=0;
  view_offset_y=0;
  view_sliding_direction=VIEW_SLID_NOWHERE;

  scroll_bot_lim=0;
  scroll_top=false;
  scroll_bot=false;
  scrolling=false;
  scroll_offset=0;

  swipe_control=0;
  swipe_index=0;
  swipe_offset=0;

  word_index=1;
  in_word=false;
  edit_word[0]=0;
  cursor=0;

  showing_title_editor=false;

  kbd_page=1;
  kbd_x=KBDSTART_X;
  kbd_y=KBDSTART_Y;

  text_scroll_offset=0;
}

extern bool user_active;

static uint8_t fps = 111;

static void draw_by_type(char* path, uint8_t g2d_node);
static void draw_list(char* path, uint8_t g2d_node);
static void draw_watch(char* path, uint8_t g2d_node);
static void draw_notes(char* path, uint8_t g2d_node);
static void draw_about(char* path, uint8_t g2d_node);
static void draw_button(char* path, uint8_t g2d_node);
static void draw_raw_offset(char* path, uint8_t g2d_node, int16_t offx);
static void draw_raw(char* path, uint8_t g2d_node);

#define LIST_ADD_NEW_TOP 1
#define LIST_ADD_NEW_BOT 2
#define LIST_BACKGROUND  3

bool evaluate_user(object* usr, void* d) {

  if(!user_active) return true;

  static bool first_time=true;
  if(first_time){
    reset_viewing_state_variables();
    first_time=false;
  }

  bool is_a_touch_triggered_eval=!!d;

  if(touch_down && !is_a_touch_triggered_eval) return true;

  if(button_pending){
    if(button_pressed){
      uint16_t histlen=object_property_length(user, "history");
      if(histlen){
        char* viewing_uid = object_property_get_n(user, "history", histlen);
        object_property_set_n(user, "history", histlen, 0);
        object_property_set(user, "viewing", viewing_uid);
      }
      else{
        char* viewing_uid=object_property(user, "viewing");
        object_property_add(user, "history", viewing_uid);
        object_property_set(user, "viewing", homeuid);
      }
      reset_viewing_state_variables();
    }
    button_pending=false;
  }

  if(list_selected_control){
    char* viewing_uid=object_property(user, "viewing");
    char* upd_fmt=(list_selected_control==LIST_ADD_NEW_TOP? "=> %s @.": "=> @. %s");
    object* o = create_new_object_like_others();
    if(o) set_edit_object(viewing_uid, "list", 0, upd_fmt, object_property(o, "UID"));
    list_selected_control=0;
  }
  else
  if(prop_selected_index){
    char propnameesc[64];
    object_property_key_esc(user, "viewing:", prop_selected_index, propnameesc, 64);
    char* sel_uid=object_pathpair_get_n(user, "viewing", propnameesc, list_selected_index);
    if(sel_uid && is_uid(sel_uid)){
      char* viewing_uid=object_property(user, "viewing");
      object_property_add(user, "history", viewing_uid);
      object_property_set(user, "viewing", sel_uid);
      reset_viewing_state_variables();
    }
    prop_selected_index=0;
    list_selected_index=0;
  }
  else
  if(list_selected_index){
    char* viewing_uid=object_property(user, "viewing");
    char* sel_uid = numlinktypes?
                 object_pathpair(      user, "viewing", linktypes[list_selected_index-1]):
                 object_property_get_n(user, "viewing:list", list_selected_index);
    object_property_add(user, "history", viewing_uid);
    object_property_set(user, "viewing", sel_uid);
    reset_viewing_state_variables();
    list_selected_index=0;
  }

  bool reset_swipe=false;

  if(add_this_word && showing_title_editor){
    char* viewing_uid=object_property(user, "viewing");
    set_edit_object(viewing_uid, "title", 0, "=> %s", add_this_word);
    add_this_word=0;
    showing_title_editor=false;
  }
  else
  if(add_this_word){
    char* viewing_uid=object_property(user, "viewing");
    set_edit_object(viewing_uid, "text", 0, "=> @. %s", add_this_word);
    add_this_word=0;
  }
  else
  if(del_this_word){
    char* viewing_uid=object_property(user, "viewing");
    set_edit_object(viewing_uid, "text", del_this_word, "=>");
    del_this_word=0;
  }
  else
  if(inv_grab_control){
    char* viewing_uid=object_property(user, "viewing");
    set_edit_object(inventoryuid, "list", 0, "=> %s @.", viewing_uid);
    inv_grab_control=0;
    reset_swipe=true;
  }
  else
  if(inv_grab_index){
    char* viewing_uid=object_property(user, "viewing");
    char* grab_uid = numlinktypes?
                       object_pathpair(      user, "viewing", linktypes[inv_grab_index-1]):
                       object_property_get_n(user, "viewing:list", inv_grab_index);
    set_edit_object(inventoryuid, "list", 0, "=> %s @.", grab_uid);
    if(inv_delete){
      if(numlinktypes) set_edit_object(viewing_uid, linktypes[inv_grab_index-1], 0, "=>");
      else             set_edit_object(viewing_uid, "list", inv_grab_index, "=>");
    }
    inv_grab_index=0;
    inv_delete=false;
    reset_swipe=true;
  }
  else
  if(inv_drop_control){
    char* drop_uid = object_property_get_n(user, "inventory:list", 1);
    if(drop_uid){
      char* viewing_uid=object_property(user, "viewing");
      char* upd_fmt=(inv_drop_control==LIST_ADD_NEW_TOP? "=> %s @.": "=> @. %s");
      set_edit_object(viewing_uid, "list", 0, upd_fmt, drop_uid);
      set_edit_object(inventoryuid, "list", 1, "=>");
    }
    inv_drop_control=0;
    reset_swipe=true;
  }
  else
  if(inv_drop_index){
    char* drop_uid = object_property_get_n(user, "inventory:list", 1);
    if(drop_uid){
      char propname[128];
      best_propname_for_link_drop(propname, 128, inv_drop_index);
      char* into_uid = numlinktypes?
                       object_pathpair(      user, "viewing", linktypes[inv_drop_index-1]):
                       object_property_get_n(user, "viewing:list", inv_drop_index);
      set_edit_object(into_uid, propname, 0, "=> %s @.", drop_uid);
      set_edit_object(inventoryuid, "list", 1, "=>");
    }
    inv_drop_index=0;
    reset_swipe=true;
  }

  // -------------------------------------------------

  uint8_t root_g2d_node = g2d_node_create(0, 0,0, SCREEN_WIDTH,SCREEN_HEIGHT, 0,0,0);

  g2d_clear_screen(0x00);

  draw_by_type("viewing", root_g2d_node);

#if defined(LOG_TO_GFX)
  show_gfx_log(root_g2d_node);
#endif

  g2d_render();

  uint64_t post_render_time=time_ms();

  static uint8_t renders = 0;
  renders++;

  static uint64_t last_render_time = 0;

  uint64_t time_since_last = post_render_time - last_render_time;
  if(time_since_last > 1000){
     fps = renders * 1000 / time_since_last;
     renders = 0;
     last_render_time = post_render_time;
  }

  // -------------------------------------------------

  if(reset_swipe){
    swipe_control=0;
    swipe_index=0;
    swipe_offset=0;
  }

  return true;
}

static void draw_by_type(char* path, uint8_t g2d_node) {

  if(object_pathpair_contains(user, path, "is", "list"))   draw_list(path, g2d_node);
  else
  if(object_pathpair_contains(user, path, "is", "watch"))  draw_watch(path, g2d_node);
  else
  if(object_pathpair_contains(user, path, "is", "text" ))  draw_notes(path, g2d_node);
  else
  if(object_pathpair_contains(user, path, "is", "about"))  draw_about(path, g2d_node);
  else
  if(object_pathpair_contains(user, path, "is", "button")) draw_button(path, g2d_node);
  else
                                                           draw_raw(path, g2d_node);
}

#define DELETE_SWIPE_DISTANCE -140
#define GRAB_SWIPE_DISTANCE    -70
#define DROP_SWIPE_DISTANCE     90

#define SLIDE_DWELL 20

static void view_cb(bool down, int16_t dx, int16_t dy, uint16_t control, uint16_t index){

  if(down){

    if(view_sliding_direction==VIEW_SLID_NOWHERE &&
        (abs(view_offset_x) > SLIDE_DWELL ||
         abs(view_offset_y) > SLIDE_DWELL   )           ){

      bool sliding_horizontally = abs(view_offset_x) > abs(view_offset_y);

      if(sliding_horizontally){
        if(view_offset_x < -SLIDE_DWELL) view_sliding_direction=VIEW_SLIDING_LEFT;
        else
        if(view_offset_x >  SLIDE_DWELL) view_sliding_direction=VIEW_SLIDING_RIGHT;
      }
      else {
        if(view_offset_y < -SLIDE_DWELL) view_sliding_direction=VIEW_SLIDING_UP;
        else
        if(view_offset_y >  SLIDE_DWELL) view_sliding_direction=VIEW_SLIDING_DOWN;
      }
    }

    // down; view_sliding_direction set if outside dwell

    if(view_sliding_direction==VIEW_SLIDING_LEFT ||
       view_sliding_direction==VIEW_SLIDING_RIGHT  ){

      view_offset_x+=dx;
      view_offset_y=0;

      if(abs(view_offset_x) < SLIDE_DWELL) view_sliding_direction=VIEW_SLID_NOWHERE;

      return;
    }
    if(view_sliding_direction==VIEW_SLIDING_UP   ||
       view_sliding_direction==VIEW_SLIDING_DOWN   ){

      view_offset_x=0;
      view_offset_y+=dy;

      if(abs(view_offset_y) < SLIDE_DWELL) view_sliding_direction=VIEW_SLID_NOWHERE;

      return;
    }

    // down; view_sliding_direction not set or set to slid-in-place

    if(view_sliding_direction < VIEW_SLID_IN_PLACE){

      view_offset_x+=dx;
      view_offset_y+=dy;

      return;
    }

    // down; slid-in-place so touching something on that view now

    if(!swipe_index){

      bool vertical = abs(dy) > abs(dx);

      if(view_sliding_direction==VIEW_SLID_UP && dy && vertical){
        view_offset_y+=dy;
        view_sliding_direction=VIEW_SLIDING_UP;
        return;
      }
      if(view_sliding_direction==VIEW_SLID_LEFT && dx && !vertical){
        view_offset_x+=dx;
        view_sliding_direction=VIEW_SLIDING_LEFT;
        return;
      }
    }

    // down; slid-in-place; not back to sliding again

    if(index && dx){
      swipe_index=index;
      swipe_offset+=dx;
    }

    return;
  }

  // up; either let go after sliding or after in-page action

  if(view_sliding_direction < VIEW_SLID_IN_PLACE){

    if(abs(view_offset_x) <= 120 &&
       abs(view_offset_y) <= 140    ){

      view_offset_x=0;
      view_offset_y=0;
      view_sliding_direction=VIEW_SLID_NOWHERE;
      return;
    }
    if(view_offset_x > 120){
      view_offset_x= 240;
      view_sliding_direction=VIEW_SLID_RIGHT;
      return;
    }
    if(view_offset_x < -120){
      view_offset_x= -240;
      view_sliding_direction=VIEW_SLID_LEFT;
      return;
    }
    if(view_offset_y > 140){
      view_offset_y= 280;
      view_sliding_direction=VIEW_SLID_DOWN;
      return;
    }
    if(view_offset_y < -140){
      view_offset_y= -280;
      view_sliding_direction=VIEW_SLID_UP;
      return;
    }
  }

  // up; slid-in-place; either horiz swipe or touch

  if(swipe_index){
    if(swipe_offset < GRAB_SWIPE_DISTANCE && swipe_index){
      inv_grab_index=swipe_index;
      if(swipe_offset < DELETE_SWIPE_DISTANCE){
        inv_delete=true;
      }
    }
    else
    if(swipe_offset > DROP_SWIPE_DISTANCE && swipe_index){
      inv_drop_index=swipe_index;
    }
    else {
      swipe_index=0;
      swipe_offset=0;
    }
    return;
  }

  // up; touched

  list_selected_index=index;
}

#define CHILD_HEIGHT 70

static void draw_swipe_feedback(uint8_t scroll_g2d_node, uint16_t y,
                                uint16_t del_col,  uint16_t grab_col,  uint16_t drop_col,
                                char*    del_text, char*    grab_text, char*    drop_text){

  uint16_t c=(swipe_offset < DELETE_SWIPE_DISTANCE)? del_col:
             (swipe_offset < GRAB_SWIPE_DISTANCE)?   grab_col:
             (swipe_offset > DROP_SWIPE_DISTANCE)?   drop_col: G2D_GREY_7;

  char* s=(swipe_offset < DELETE_SWIPE_DISTANCE)? del_text:
          (swipe_offset < GRAB_SWIPE_DISTANCE)?   grab_text:
          (swipe_offset > DROP_SWIPE_DISTANCE)?   drop_text: "";

  uint16_t x=(swipe_offset < DELETE_SWIPE_DISTANCE)? 130:
             (swipe_offset < GRAB_SWIPE_DISTANCE)?   130:
             (swipe_offset > DROP_SWIPE_DISTANCE)?    30: 0;

  g2d_node_rectangle(scroll_g2d_node, 20,y, 200,CHILD_HEIGHT-10, c);
  g2d_node_text(scroll_g2d_node, x,y+15, G2D_WHITE, c, 4, "%s", s);
}

static void draw_links(char* path, uint8_t container_g2d_node){

  uint16_t y=280;

  for(uint8_t i=1; i<=numlinktypes; i++){

    if(i==swipe_index){
      uint16_t droppables=object_property_length(user, "inventory:list");
      uint16_t drop_col  = droppables? G2D_BLUE: G2D_GREY_7;
      char*    drop_text = droppables? "O->": "---";
      draw_swipe_feedback(container_g2d_node, y, G2D_RED, G2D_GREEN_18, drop_col,
                                                 "<-X",   "<-O",        drop_text);
    }
    uint8_t child_g2d_node = g2d_node_create(container_g2d_node,
                                             (i==swipe_index? swipe_offset: 0)+20,y,
                                             200,CHILD_HEIGHT-10,
                                             view_cb,0,i);
    if(child_g2d_node){
      static char pathbufrec[64];
      snprintf(pathbufrec, 64, "%s:%s", path, linktypes[i-1]);
      draw_by_type(pathbufrec, child_g2d_node);
    }
    y+=CHILD_HEIGHT;
  }
}

static void list_cb(bool down, int16_t dx, int16_t dy, uint16_t control, uint16_t index){

  if(down){
    bool vertical = abs(dy) > abs(dx);
    if(!swipe_control && !swipe_index && dy && vertical){
      scrolling=true;
      bool stretching = (scroll_top && dy>0) ||
                        (scroll_bot && dy<0);
      scroll_offset+= stretching? dy/3: dy;
    }
    else
    if(!scrolling && dx && !vertical && control!=LIST_BACKGROUND){
      swipe_control=control;
      swipe_index=index;
      swipe_offset+= dx;
    }
    return;
  }

  if(scrolling){
    if(scroll_top) scroll_offset=0;
    if(scroll_bot) scroll_offset=scroll_bot_lim;
    scrolling=false;
    return;
  }

  if(swipe_control || swipe_index){
    if(swipe_offset < GRAB_SWIPE_DISTANCE && swipe_control){
      inv_grab_control=swipe_control;
    }
    else
    if(swipe_offset < GRAB_SWIPE_DISTANCE && swipe_index){
      inv_grab_index=swipe_index;
      if(swipe_offset < DELETE_SWIPE_DISTANCE){
        inv_delete=true;
      }
    }
    else
    if(swipe_offset > DROP_SWIPE_DISTANCE && swipe_control){
      inv_drop_control=swipe_control;
    }
    else
    if(swipe_offset > DROP_SWIPE_DISTANCE && swipe_index){
      inv_drop_index=swipe_index;
    }
    else {
      swipe_control=0;
      swipe_index=0;
      swipe_offset=0;
    }
    return;
  }

  if(control!=LIST_BACKGROUND){
    list_selected_control=control;
    list_selected_index=index;
  }
}

static void title_cb(bool down, int16_t dx, int16_t dy, uint16_t control, uint16_t index){
  if(down) showing_title_editor=true;
}

static uint8_t make_in_scroll_button(uint8_t scroll_g2d_node,
                                     uint16_t y, uint16_t control, char* text){

  if(control==swipe_control){
    uint16_t droppables=object_property_length(user, "inventory:list");
    uint16_t drop_col  = droppables? G2D_BLUE: G2D_GREY_7;
    char*    drop_text = droppables? "O->": "---";
    draw_swipe_feedback(scroll_g2d_node, y, G2D_GREEN_18, G2D_GREEN_18, drop_col,
                                            "<-O",        "<-O",        drop_text);
  }

  uint8_t n=g2d_node_create(scroll_g2d_node,
                            (control==swipe_control? swipe_offset: 0)+20,y,
                            200,CHILD_HEIGHT-10,
                            list_cb,control,0);

  g2d_node_rectangle(n, 0,0, g2d_node_width(n),g2d_node_height(n), G2D_GREY_F);
  g2d_node_text(n, 10,15, G2D_WHITE, G2D_GREY_F, 4, "%s", text);

  return n;
}

#define BOTTOM_MARGIN 20
#define TITLE_HEIGHT 45

static void draw_list(char* path, uint8_t g2d_node) {

  char* title = object_pathpair(user, path, "title:1");
  if(!title) title="List";

  // -------------------------------------------------------

  if(g2d_node_height(g2d_node) < SCREEN_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_GREY_1D/13);
    uint8_t s=g2d_node_height(g2d_node) < 60? 2: 3;
    uint8_t m=(g2d_node_height(g2d_node)-8*s)/2;
    g2d_node_text(g2d_node, m,m, G2D_WHITE, G2D_GREY_1D/13, s, "%s", title);
    return;
  }

  // -------------------------------------------------------

  uint8_t title_g2d_node = g2d_node_create(g2d_node,
                                           0,0,
                                           g2d_node_width(g2d_node),
                                           TITLE_HEIGHT,
                                           title_cb,0,0);
  if(!title_g2d_node) return;

  if(showing_title_editor && cursor==0){
    snprintf(edit_word, 64, "%s", title);
    cursor=strlen(edit_word);
    in_word=true;
  }
  g2d_node_rectangle(title_g2d_node, 0,0,
                     g2d_node_width(title_g2d_node),g2d_node_height(title_g2d_node),
                     showing_title_editor? G2D_GREY_1D/7: G2D_GREY_1D/13);

  g2d_node_text(title_g2d_node, 30,15, G2D_WHITE,
                showing_title_editor? G2D_GREY_1D/7: G2D_GREY_1D/13, 2,
                "%s",
                showing_title_editor? edit_word: title);

  // -------------------------------------------------------

  uint8_t list_container_g2d_node = g2d_node_create(g2d_node,
                                                    0,TITLE_HEIGHT,
                                                    g2d_node_width(g2d_node),
                                                    g2d_node_height(g2d_node)-TITLE_HEIGHT,
                                                    0,0,0);
  if(!list_container_g2d_node) return;

  // -------------------------------------------------------

  uint16_t ll = object_pathpair_length(user, path, "list");

  uint16_t scroll_height=max(10+(ll+2)*CHILD_HEIGHT+10, SCREEN_HEIGHT*4/3);

  uint8_t scroll_g2d_node = g2d_node_create(list_container_g2d_node,
                                            0, scroll_offset,
                                            g2d_node_width(list_container_g2d_node),
                                            scroll_height,
                                            list_cb,LIST_BACKGROUND,0);
  if(!scroll_g2d_node) return;

  scroll_bot_lim = -scroll_height + SCREEN_HEIGHT - BOTTOM_MARGIN;
  scroll_top = (scroll_offset > 0);
  scroll_bot = (scroll_offset < scroll_bot_lim);

  uint16_t stretch_height=g2d_node_height(list_container_g2d_node)/3;
  if(scroll_top) g2d_node_rectangle(list_container_g2d_node,
                                    20,0,
                                    200,stretch_height, G2D_GREY_7);
  if(scroll_bot) g2d_node_rectangle(list_container_g2d_node,
                                    20,2*stretch_height,
                                    200,stretch_height, G2D_GREY_7);

  uint16_t y=CHILD_HEIGHT-10+20;

  make_in_scroll_button(scroll_g2d_node, 10, LIST_ADD_NEW_TOP, "+");

  for(uint8_t i=1; i<=ll; i++){

    if(i==swipe_index){
      uint16_t droppables=object_property_length(user, "inventory:list");
      uint16_t drop_col  = droppables? G2D_BLUE: G2D_GREY_7;
      char*    drop_text = droppables? "O->": "---";
      draw_swipe_feedback(scroll_g2d_node, y, G2D_RED, G2D_GREEN_18, drop_col,
                                              "<-X",   "<-O",        drop_text);
    }
    uint8_t child_g2d_node = g2d_node_create(scroll_g2d_node,
                                             (i==swipe_index? swipe_offset: 0)+20,y,
                                             200,CHILD_HEIGHT-10,
                                             list_cb,0,i);
    if(child_g2d_node){
      static char pathbufrec[64];
      snprintf(pathbufrec, 64, "%s:list:%d", path, i);
      draw_by_type(pathbufrec, child_g2d_node);
    }
    y+=CHILD_HEIGHT;
  }
  make_in_scroll_button(scroll_g2d_node, y, LIST_ADD_NEW_BOT, "+");

  if(showing_title_editor){
    show_keyboard(g2d_node);
  }
}

#define BATTERY_LOW      G2D_RED
#define BATTERY_MED      G2D_YELLOW
#define BATTERY_HIGH     G2D_GREEN
#define BATTERY_CHARGING G2D_BLUE

static void draw_watch(char* path, uint8_t g2d_node) {

  char* pc=object_pathpair(   user, path, "battery:percent:1");
  bool  ch=object_pathpair_is(user, path, "battery:status", "charging");
  char* ts=object_pathpair(   user, path, "watchface:clock:ts");
  char* tz=object_pathpair(   user, path, "watchface:clock:tz:2");
  bool h24=object_pathpair_is(user, path, "watchface:ampm-24hr", "24hr");

  if(!ts) return;

  char* e;

  uint64_t tsnum=strtoull(ts,&e,10);
  if(*e) return;

  uint32_t tznum=strtoul(tz?tz:"0",&e,10);
  if(*e) tznum=0;

  time_t est=(time_t)(tsnum+tznum);
  struct tm tms={0};
  localtime_r(&est, &tms);

  char g2dbuf[64];

  if(g2d_node_height(g2d_node) < SCREEN_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_YELLOW/6);
    strftime(g2dbuf, 64, h24? "%H:%M": "%l:%M", &tms);
    uint8_t s=g2d_node_height(g2d_node) < 60? 2: 3;
    uint8_t m=(g2d_node_height(g2d_node)-8*s)/2;
    g2d_node_text(g2d_node, m,m, G2D_WHITE, G2D_YELLOW/6, s, "%s", g2dbuf);
    return;
  }

  get_linktypes(path);

  uint16_t container_height = g2d_node_height(g2d_node);

  container_height += CHILD_HEIGHT * numlinktypes;
  container_height += CHILD_HEIGHT;

  int16_t offx = abs(view_offset_x) < SLIDE_DWELL? 0: view_offset_x;
  int16_t offy = abs(view_offset_y) < SLIDE_DWELL? 0: view_offset_y;

  uint8_t container_g2d_node = g2d_node_create(g2d_node, offx, offy,
                                               g2d_node_width(g2d_node),
                                               container_height,
                                               view_cb,0,0);
  if(container_g2d_node){

  strftime(g2dbuf, 64, h24? "%H:%M": "%l:%M", &tms);
  g2d_node_text(container_g2d_node, 10, 100, G2D_WHITE, G2D_BLACK, 7, "%s", g2dbuf);

  strftime(g2dbuf, 64, "%S", &tms);
  g2d_node_text(container_g2d_node, 105, 140, G2D_GREY_1A, G2D_BLACK, 1, "%s", g2dbuf);

  if(!h24){
    strftime(g2dbuf, 64, "%p", &tms);
    g2d_node_text(container_g2d_node, 180, 160, G2D_BLUE, G2D_BLACK, 3, "%s", g2dbuf);
  }

  strftime(g2dbuf, 64, "%a %d %h", &tms);
  g2d_node_text(container_g2d_node, 30, 220, G2D_BLUE, G2D_BLACK, 3, "%s", g2dbuf);

  int8_t pcnum=pc? (int8_t)strtol(pc,&e,10): 0;
  if(pcnum<0) pcnum=0;
  if(pcnum>100) pcnum=100;

  uint16_t batt_col;
  if(ch)       batt_col=BATTERY_CHARGING; else
  if(pcnum>30) batt_col=BATTERY_HIGH;     else
  if(pcnum>17) batt_col=BATTERY_MED;
  else         batt_col=BATTERY_LOW;

  g2d_node_text(container_g2d_node, 180, 15, batt_col, G2D_BLACK, 2, "%d%%", pcnum);

  if(offy < 0) draw_links(path, container_g2d_node);

  } // if(container_g2d_node)

  if(offx < 0) draw_raw_offset(path, g2d_node, offx);
}

static void word_cb(bool down, int16_t dx, int16_t dy, uint16_t c, uint16_t wi){
  static bool scrolled=false;
  if(!down){
    if(!scrolled && wi) word_index=wi;
    scrolled=false;
  }
  else
  if(dx+dy!=0){
    text_scroll_offset+=dy;
    scrolled=true;
  }
}

static void draw_notes(char* path, uint8_t g2d_node) {

  if(g2d_node_height(g2d_node) < SCREEN_HEIGHT){
    char* word1=object_pathpair(user, path, "text:1");
    char* word2=object_pathpair(user, path, "text:2");
    char* word3=object_pathpair(user, path, "text:3");
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_GREEN/6);
    uint8_t s=g2d_node_height(g2d_node) < 60? 2: 3;
    uint8_t m=(g2d_node_height(g2d_node)-8*s)/2;
    uint8_t n=m+g2d_text_width(word1,s)+10;
    uint8_t o=n+g2d_text_width(word2,s)+10;
    g2d_node_text(g2d_node, m,m, G2D_WHITE, G2D_GREEN/6, s, "%s", word1? word1: "");
    g2d_node_text(g2d_node, n,m, G2D_WHITE, G2D_GREEN/6, s, "%s", word2? word2: "");
    g2d_node_text(g2d_node, o,m, G2D_WHITE, G2D_GREEN/6, s, "%s", word3? word3: "");
    return;
  }

  #define SIDE_MARGIN  20
  #define TOP_MARGIN   30
  #define LINE_HEIGHT  20
  #define WORD_SPACING  7
  #define CURSOR_WIDTH  3

  get_linktypes(path);

  uint16_t container_height = g2d_node_height(g2d_node);

  container_height += CHILD_HEIGHT * numlinktypes;
  container_height += CHILD_HEIGHT;

  int16_t offx = abs(view_offset_x) < SLIDE_DWELL? 0: view_offset_x;
  int16_t offy = abs(view_offset_y) < SLIDE_DWELL? 0: view_offset_y;

  uint8_t container_g2d_node = g2d_node_create(g2d_node, offx, offy,
                                               g2d_node_width(g2d_node),
                                               container_height,
                                               view_cb,0,0);
  if(container_g2d_node){

  g2d_node_text(container_g2d_node, 10,5, G2D_BLUE, G2D_BLACK, 2,
                "fps: %02d (%d,%d)", fps, touch_info.x, touch_info.y);

  int16_t wd=g2d_node_width(g2d_node)-2*SIDE_MARGIN;
  int16_t ht=g2d_node_height(g2d_node)-2*TOP_MARGIN;
  if(wd<0 || ht<0) return;

  uint8_t text_container_g2d_node = g2d_node_create(container_g2d_node,
                                                    SIDE_MARGIN,TOP_MARGIN, wd,ht,
                                                    word_cb,0,0);
  if(text_container_g2d_node){

  uint16_t words=object_pathpair_length(user, path, "text");

  uint16_t lines=(words/2)+1;
  uint16_t scroll_height=lines*LINE_HEIGHT;

  if(in_word){
    word_index=words+1;
  }

  uint8_t text_scroll_g2d_node = g2d_node_create(text_container_g2d_node,
                                                 0, text_scroll_offset,
                                                 wd, scroll_height,
                                                 word_cb,0,0);
  if(text_scroll_g2d_node){

  uint16_t k=0;
  uint16_t j=0;
  for(uint16_t w=1; w<=words+1; w++){

    int16_t available_width = wd-k;

    char*    word = 0;
    uint16_t word_width=available_width;
    if(w<=words){
      word = object_pathpair_get_n(user, path, "text", w);
      word_width=WORD_SPACING+g2d_text_width(word, 2);
    }
    if(k && word_width > available_width){
      // not fab way to "extend" the prev word out to full width for cursor
      g2d_node_create(text_scroll_g2d_node,
                      k, j*LINE_HEIGHT,
                      available_width, LINE_HEIGHT,
                      word_cb,0,(w-1));
      k=0;
      j++;
    }
    if(w==word_index){
      g2d_node_rectangle(text_scroll_g2d_node,
                         k, j*LINE_HEIGHT,
                         CURSOR_WIDTH, LINE_HEIGHT,
                         G2D_MAGENTA);
    }

    uint8_t word_g2d_node = g2d_node_create(text_scroll_g2d_node,
                                            k, j*LINE_HEIGHT,
                                            word_width, LINE_HEIGHT,
                                            word_cb,0,w);

    char* word_to_show = word? word: (in_word? edit_word: "");
    g2d_node_text(word_g2d_node, 6,2, G2D_WHITE, G2D_BLACK, 2, "%s", word_to_show);
    k+=word_width;
  }

  } // if(text_scroll_g2d_node)

  show_keyboard(container_g2d_node);

  show_touch_point(container_g2d_node);

  } // if(text_container_g2d_node)

  if(offy < 0) draw_links(path, container_g2d_node);

  } // if(container_g2d_node)

  if(offx < 0) draw_raw_offset(path, g2d_node, offx);
}

extern uint32_t loop_time;
extern uint32_t touch_events;
extern uint32_t touch_events_seen;
extern uint32_t touch_events_spurious;

static void draw_about(char* path, uint8_t g2d_node) {

  if(g2d_node_height(g2d_node) < SCREEN_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_CYAN/6);
//  uint32_t touch_events_percent = (100*touch_events_seen)/(1+touch_events);
    uint8_t s=g2d_node_height(g2d_node) < 60? 2: 3;
    uint8_t m=(g2d_node_height(g2d_node)-8*s)/2;
    g2d_node_text(g2d_node, m,m, G2D_WHITE, G2D_CYAN/6, s,
                  "%dfps %ldms", fps, loop_time);
//              "%ld%% %ld %ldms", touch_events_percent, touch_events_spurious, loop_time);
    return;
  }

  get_linktypes(path);

  uint16_t container_height = g2d_node_height(g2d_node);

  container_height += CHILD_HEIGHT * numlinktypes;
  container_height += CHILD_HEIGHT;

  int16_t offx = abs(view_offset_x) < SLIDE_DWELL? 0: view_offset_x;
  int16_t offy = abs(view_offset_y) < SLIDE_DWELL? 0: view_offset_y;

  uint8_t container_g2d_node = g2d_node_create(g2d_node, offx, offy,
                                               g2d_node_width(g2d_node),
                                               container_height,
                                               view_cb,0,0);
  if(container_g2d_node){

  g2d_node_text(container_g2d_node, 20, 40, G2D_BLUE, G2D_BLACK, 2,
                "fps: %d (%d,%d)", fps, touch_info.x, touch_info.y);

  g2d_node_text(container_g2d_node, 10, 110, G2D_BLUE, G2D_BLACK, 3,
                "cpu: %s", object_pathpair(user, path, "cpu"));

  g2d_node_text(container_g2d_node, 10, 190, G2D_BLUE, G2D_BLACK, 1,
                "build: %s %s", object_pathpair_get_n(user, path, "build-info", 1),
                                object_pathpair_get_n(user, path, "build-info", 2));

  if(offy < 0) draw_links(path, container_g2d_node);

  } // if(container_g2d_node)

  if(offx < 0) draw_raw_offset(path, g2d_node, offx);
}

static void draw_button(char* path, uint8_t g2d_node) {

  if(g2d_node_height(g2d_node) < SCREEN_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_CYAN/6);
    uint8_t s=g2d_node_height(g2d_node) < 60? 2: 3;
    uint8_t m=(g2d_node_height(g2d_node)-8*s)/2;
    g2d_node_text(g2d_node, m,m, G2D_WHITE, G2D_CYAN/6, s,
                      "%s", object_pathpair_is(user, path, "state", "down")? "down": "up");
    return;
  }
  draw_raw(path, g2d_node);
}

static void raw_cb(bool down, int16_t dx, int16_t dy, uint16_t p_index, uint16_t l_index){

  if(down){
    if(dy){
      scrolling=true;
      scroll_offset+= dy;
    }
    return;
  }

  if(scrolling){
    scrolling=false;
    return;
  }

  prop_selected_index=p_index;
  list_selected_index=l_index;
}

#define PROP_HEIGHT 38
#define PROP_MARGIN 5
static void draw_raw(char* path, uint8_t g2d_node) {

  if(g2d_node_height(g2d_node) < SCREEN_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_RGB256(90,90,130));
    uint8_t s=g2d_node_height(g2d_node) < 60? 2: 3;
    uint8_t m=(g2d_node_height(g2d_node)-8*s)/2;
    uint16_t ll=object_pathpair_length(user, path, "is");
    uint16_t vx=m;
    for(uint8_t i=1; i<=ll; i++){
      char* is=object_pathpair_get_n(user, path, "is", i);
      uint16_t word_width=WORD_SPACING+g2d_text_width(is, s);
      g2d_node_text(g2d_node, vx,m, G2D_WHITE, G2D_RGB256(90,90,130), s, "%s", is);
      vx+=word_width+WORD_SPACING;
    }
    return;
  }

  uint8_t title_g2d_node = g2d_node_create(g2d_node,
                                           0,0,
                                           g2d_node_width(g2d_node),
                                           TITLE_HEIGHT,
                                           0,0,0);
  if(!title_g2d_node) return;

  g2d_node_rectangle(title_g2d_node, 0,0,
                     g2d_node_width(title_g2d_node),g2d_node_height(title_g2d_node),
                     G2D_GREY_1D/13);

  g2d_node_text(title_g2d_node, 30,15, G2D_WHITE, G2D_GREY_1D/13, 1,
                "%s", object_pathpair(user, path, "UID"));

  uint8_t list_container_g2d_node = g2d_node_create(g2d_node,
                                                    0,TITLE_HEIGHT,
                                                    g2d_node_width(g2d_node),
                                                    g2d_node_height(g2d_node)-TITLE_HEIGHT,
                                                    0,0,0);
  if(!list_container_g2d_node) return;

  char viewpath[64]; snprintf(viewpath, 64, "%s:", path);
  int16_t ol = object_property_size(user, viewpath);
  if(ol<0) return;

  uint16_t scroll_height=max(10+ol*PROP_HEIGHT+10, SCREEN_HEIGHT*4/3);

  uint8_t scroll_g2d_node = g2d_node_create(list_container_g2d_node,
                                            0,scroll_offset,
                                            g2d_node_width(list_container_g2d_node),
                                            scroll_height,
                                            raw_cb,0,0);
  if(!scroll_g2d_node) return;

  uint16_t vy=PROP_MARGIN;
  for(uint16_t p=1; p<=ol; p++){

    uint8_t propname_g2d_node = g2d_node_create(scroll_g2d_node,
                                                PROP_MARGIN,vy,
                                                100,PROP_HEIGHT-PROP_MARGIN, 0,0,0);

    char* propname =       object_property_key(    user, viewpath, p);
    char  propnameesc[64]; object_property_key_esc(user, viewpath, p, propnameesc, 64);

    g2d_node_rectangle(propname_g2d_node, 0,0,
                       g2d_node_width(propname_g2d_node),g2d_node_height(propname_g2d_node),
                       G2D_CYAN/6);
    g2d_node_text(propname_g2d_node, 7,7, G2D_WHITE, G2D_CYAN/6, 2, "%s", propname);


    uint16_t ll=object_pathpair_length(user, path, propnameesc);
    uint16_t vx=PROP_MARGIN+100+PROP_MARGIN;
    for(uint8_t i=1; i<=ll; i++){

      char* propvalue=object_pathpair_get_n(user, path, propnameesc, i);
      bool isuid=is_uid(propvalue);

      uint16_t valwid=(i==ll? 140: isuid? 50: g2d_text_width(propvalue, 2)+WORD_SPACING+5);
      uint8_t propvalue_g2d_node = g2d_node_create(scroll_g2d_node,
                                                   vx,vy,
                                                   valwid, PROP_HEIGHT-PROP_MARGIN,
                                                   raw_cb,p,i);
      if(!propvalue_g2d_node) break;

      if(isuid){
        static char pathbufrec[128];
        snprintf(pathbufrec, 128, "%s:%s:%d", path, propnameesc, i);
        draw_by_type(pathbufrec, propvalue_g2d_node);
      }
      else{
        g2d_node_rectangle(propvalue_g2d_node, 0,0,
                           g2d_node_width(propvalue_g2d_node),
                           g2d_node_height(propvalue_g2d_node),
                           G2D_GREY_1D/13);
        g2d_node_text(propvalue_g2d_node,
                      7,7, G2D_WHITE, G2D_GREY_1D/13, 2, "%s", propvalue);
      }
      vx+=valwid+5;
    }
    vy+=PROP_HEIGHT;
  }
}

static void draw_raw_offset(char* path, uint8_t g2d_node, int16_t offx){

  uint8_t raw_container_g2d_node = g2d_node_create(g2d_node,
                                                   240+offx,0,
                                                   -offx,
                                                   g2d_node_height(g2d_node),
                                                   view_cb,0,0);
  if(!raw_container_g2d_node) return;

  uint8_t raw_g2d_node = g2d_node_create(raw_container_g2d_node,
                                         -(240+offx),0,
                                         240, g2d_node_height(g2d_node),
                                         view_cb,0,0);
  if(!raw_g2d_node) return;

  draw_raw(path, raw_g2d_node);
}
