
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <boards.h>

#include <items.h>
#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include <onex-kernel/touch.h>

#include <onn.h>

#include "mathlib.h"
#include "g2d.h"

// -------------------- User --------------------------

extern uint16_t word_index;
extern bool     in_word;
extern char     edit_word[];
extern uint8_t  cursor;

#define KBDSTART_X 0
#define KBDSTART_Y 250
extern uint8_t kbd_page;
extern int16_t kbd_x;
extern int16_t kbd_y;

extern int16_t text_scroll_offset;

extern uint16_t del_this_word;
extern char*    add_this_word;

extern void show_keyboard(uint8_t g2d_node);

extern char* event_log_buffer;

#define BUTTON_ACTION_NONE  0
#define BUTTON_ACTION_WAIT  1
#define BUTTON_ACTION_SHORT 2
#define BUTTON_ACTION_LONG  3
extern uint8_t button_action;

extern touch_info_t touch_info;
extern bool         touch_down;

extern char* homeuid;

object* user;

// XXX separated out to remind me to extend the respective APIs to allow sprintf-style
// XXX varargs for path segments, value construction and g2d_node_text()
static char pathbuf[64];
static char g2dbuf[64];

static void eval_update_list(char* uid, char* key, uint16_t i, char* val) {
  properties* update = properties_new(1);
  list* li=0;
  if(i){
    li=list_new(i+1);
    if(!val || !*val){
      for(uint16_t x=1; x<i; x++) list_add(li, value_new("something"));
      list_add(li, value_new("(=>)"));
    }
  }
  else{
    if(val && *val){
      li=list_new(3);
      list_add(li, value_new((char*)"=>"));
      list_add(li, value_new((char*)"@."));
      list_add(li, value_new(val));
    }
  }
  properties_set(update, key, li);
  onex_run_evaluators(uid, update);
}

static object* create_new_object_like_others() {

  char* newtype="list editable";

  snprintf(pathbuf, 64, "viewing:is");
  if(object_property_contains(user, pathbuf, "text")){
    newtype="text editable";
  }
  else{
    int16_t ll = object_property_length(user, "viewing:list");
    if(ll==0){
      newtype="text editable";
    }
    else{
      snprintf(pathbuf, 64, "viewing:list:1:is");
      if( object_property_contains(user, pathbuf, "text") &&
         !object_property_contains(user, pathbuf, "list")    ){

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

static void show_gfx_log(uint8_t root_g2d_node){

  uint64_t pre_render_time=time_ms();

  #define LOG_DECAY_TIME 5
  #define LOG_LINES_MAX 5
  #define LOG_LEN 40
  static char*    log_lines[LOG_LINES_MAX];
  static uint8_t  log_lines_index=0;
  static uint64_t log_last=0;
  if(event_log_buffer){
    if(log_lines[log_lines_index]) free(log_lines[log_lines_index]);
    log_lines[log_lines_index]=strndup((char*)event_log_buffer, LOG_LEN);
    log_lines_index=(log_lines_index+1)%LOG_LINES_MAX;
    uint8_t linelen=strlen((char*)event_log_buffer);
    if(linelen>LOG_LEN){
      char* lastbit=(char*)event_log_buffer+linelen-LOG_LEN;
      log_lines[log_lines_index]=strndup(lastbit, LOG_LEN);
      log_lines_index=(log_lines_index+1)%LOG_LINES_MAX;
    }
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
      g2d_node_text(root_g2d_node, 20,8+i*8, log_lines[i], G2D_RED, G2D_BLACK, 1);
    }
  }
}

static void show_touch_point(uint8_t g2d_node){
  uint8_t touch_g2d_node=g2d_node_create(g2d_node, touch_info.x, touch_info.y, 5,5, 0,0);
  g2d_node_rectangle(touch_g2d_node, 0,0, 5,5, G2D_MAGENTA);
}

static char* list_selected_uid=0;

static int16_t  scroll_bot_lim=0;
static bool     scroll_top=false;
static bool     scroll_bot=false;
static bool     scrolling=false;
static int16_t  scroll_offset=0;

static void reset_viewing_state_variables(){

  scroll_bot_lim=0;
  scroll_top=false;
  scroll_bot=false;
  scrolling=false;
  scroll_offset=0;

  word_index=1;
  in_word=false;
  edit_word[0]=0;
  cursor=0;

  kbd_page=1;
  kbd_x=KBDSTART_X;
  kbd_y=KBDSTART_Y;

  text_scroll_offset=0;
}

bool user_active=true;

static uint8_t fps = 111;

static void draw_by_type(char* p, uint8_t g2d_node);
static void draw_list(char* p, uint8_t g2d_node);
static void draw_watch(char* p, uint8_t g2d_node);
static void draw_notes(char* p, uint8_t g2d_node);
static void draw_about(char* p, uint8_t g2d_node);
static void draw_default(char* p, uint8_t g2d_node);

static bool first_time=true;

bool evaluate_user(object* usr, void* d) {

  if(first_time){
    reset_viewing_state_variables();
    first_time=false;
  }

  bool is_a_touch_triggered_eval=!!d;

  if(touch_down && !is_a_touch_triggered_eval) return true;

  if(!user_active) return true;

  if(button_action==BUTTON_ACTION_SHORT){
    uint16_t histlen=object_property_length(user, "history");
    if(histlen){
      snprintf(pathbuf, 64, "history:%d", histlen);
      char* viewing = object_property(user, pathbuf);
      object_property_set(user, pathbuf, 0);
      object_property_set(user, "viewing", viewing);
      reset_viewing_state_variables();
    }
    button_action=BUTTON_ACTION_NONE;
  }
  else
  if(button_action==BUTTON_ACTION_LONG){
    char* viewing_uid=object_property(user, "viewing");
    object_property_add(user, "history", viewing_uid);
    object_property_set(user, "viewing", homeuid);
    reset_viewing_state_variables();
    button_action=BUTTON_ACTION_NONE;
  }

  if(list_selected_uid){
    char* viewing_uid=object_property(user, "viewing");
    if(!strcmp(list_selected_uid, "new-at-top")){
      object* o = create_new_object_like_others();
      if(o) eval_update_list(viewing_uid, "list", 0, object_property(o, "UID"));
    }
    else{
      object_property_add(user, "history", viewing_uid);
      object_property_set(user, "viewing", list_selected_uid);
      reset_viewing_state_variables();
    }
    list_selected_uid=0;
  }

  if(del_this_word || add_this_word){
    // hack alert - setting epoch ts from kbd
    if(add_this_word && object_property_contains(user, "viewing:is", "watch")){
        char* e; uint64_t tsnum=strtoull(add_this_word,&e,10);
        if(!(*e)) time_es_set(tsnum);
    }
    // hack alert - setting epoch ts from kbd
    else{
      char* viewing_uid=object_property(user, "viewing");
      eval_update_list(viewing_uid, "text", del_this_word, add_this_word);
    }
    del_this_word=0; add_this_word=0;
  }

  uint8_t root_g2d_node = g2d_node_create(0, 0,0, ST7789_WIDTH,ST7789_HEIGHT, 0,0);

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

  return true;
}

void draw_by_type(char* p, uint8_t g2d_node)
{
  snprintf(pathbuf, 64, "%s:is", p);

  if(object_property_contains(user, pathbuf, "list"))  draw_list(p, g2d_node);    else
  if(object_property_contains(user, pathbuf, "watch")) draw_watch(p, g2d_node);   else
  if(object_property_contains(user, pathbuf, "text" )) draw_notes(p, g2d_node);   else
  if(object_property_contains(user, pathbuf, "about")) draw_about(p, g2d_node);   else
                                                       draw_default(p, g2d_node);
}

static void list_cb(bool down, int16_t dx, int16_t dy, void* uid){

  if(down){
    if(dx+dy){
      scrolling=true;
      bool stretching = (scroll_top && dy>0) ||
                        (scroll_bot && dy<0);
      scroll_offset+= stretching? dy/3: dy;
    }
    return;
  }
  if(scrolling){
    scrolling=false;
    if(scroll_top) scroll_offset=0;
    if(scroll_bot) scroll_offset=scroll_bot_lim;
    return;
  }
  list_selected_uid=uid;
}

static void draw_list(char* p, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:list", p);

  uint8_t ll=object_property_length(user, pathbuf);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_GREY_1D/13);
    g2d_node_text(g2d_node, 10,20, "list", G2D_WHITE, G2D_GREY_1D/13, 3);
    return;
  }
  #define CHILD_HEIGHT 70
  #define BOTTOM_MARGIN 20

  uint16_t scroll_height=max(10+ll*CHILD_HEIGHT+10, ST7789_HEIGHT*4/3);

  uint8_t scroll_g2d_node = g2d_node_create(g2d_node,
                                            0, scroll_offset,
                                            g2d_node_width(g2d_node),
                                            scroll_height,
                                            list_cb, 0);
  if(!scroll_g2d_node) return;

  scroll_bot_lim = -scroll_height + ST7789_HEIGHT - BOTTOM_MARGIN;
  scroll_top = (scroll_offset > 0);
  scroll_bot = (scroll_offset < scroll_bot_lim);

  uint16_t stretch_height=g2d_node_height(g2d_node)/3;
  if(scroll_top) g2d_node_rectangle(g2d_node,
                                    20,0,
                                    200,stretch_height, G2D_GREY_7);
  if(scroll_bot) g2d_node_rectangle(g2d_node,
                                    20,2*stretch_height,
                                    200,stretch_height, G2D_GREY_7);

  uint8_t new_top_g2d_node = g2d_node_create(scroll_g2d_node,
                                             20,10,
                                             200,CHILD_HEIGHT-30,
                                             list_cb, "new-at-top");
  g2d_node_rectangle(new_top_g2d_node, 0,0,
                     g2d_node_width(new_top_g2d_node),g2d_node_height(new_top_g2d_node),
                     G2D_GREY_F);
  g2d_node_text(new_top_g2d_node, 10,10, "add new", G2D_WHITE, G2D_GREY_F, 2);

  uint16_t y=g2d_node_height(new_top_g2d_node)+20;

  for(uint8_t i=1; i<=ll; i++){

    static char pathbufrec[64];
    snprintf(pathbufrec, 64, "%s:list:%d", p, i);
    char* uid=object_property(user, pathbufrec);

    uint8_t child_g2d_node = g2d_node_create(scroll_g2d_node,
                                             20,y,
                                             200,CHILD_HEIGHT-10,
                                             list_cb, uid);

    if(child_g2d_node) draw_by_type(pathbufrec, child_g2d_node);

    y+=CHILD_HEIGHT;
  }
}

#define BATTERY_LOW      G2D_RED
#define BATTERY_MED      G2D_YELLOW
#define BATTERY_HIGH     G2D_GREEN
#define BATTERY_CHARGING G2D_BLUE

static void draw_watch(char* path, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:battery:percent", path);
  char* pc=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:battery:status", path);
  bool  ch=object_property_is(user, pathbuf, "charging");
  snprintf(pathbuf, 64, "%s:watchface:clock:ts", path);
  char* ts=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:watchface:clock:tz:2", path);
  char* tz=object_property(   user, pathbuf);
  snprintf(pathbuf, 64, "%s:watchface:ampm-24hr", path);
  bool h24=object_property_is(user, pathbuf, "24hr");

  if(!ts) return;

  char* e;

  uint64_t tsnum=strtoull(ts,&e,10);
  if(*e) return;

  uint32_t tznum=strtoul(tz?tz:"0",&e,10);
  if(*e) tznum=0;

  time_t est=(time_t)(tsnum+tznum);
  struct tm tms={0};
  localtime_r(&est, &tms);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_YELLOW/6);
    strftime(g2dbuf, 64, h24? "%H:%M": "%l:%M", &tms);
    g2d_node_text(g2d_node, 10,20, g2dbuf, G2D_WHITE, G2D_YELLOW/6, 3);
    return;
  }

  strftime(g2dbuf, 64, h24? "%H:%M": "%l:%M", &tms);
  g2d_node_text(g2d_node, 10, 90, g2dbuf, G2D_WHITE, G2D_BLACK, 7);

  strftime(g2dbuf, 64, "%S", &tms);
  g2d_node_text(g2d_node, 105, 130, g2dbuf, G2D_GREY_1A, G2D_BLACK, 1);

  if(!h24){
    strftime(g2dbuf, 64, "%p", &tms);
    g2d_node_text(g2d_node, 180, 150, g2dbuf, G2D_BLUE, G2D_BLACK, 3);
  }

  strftime(g2dbuf, 64, "%a %d %h", &tms);
  g2d_node_text(g2d_node, 30, 210, g2dbuf, G2D_BLUE, G2D_BLACK, 3);

  int8_t pcnum=pc? (int8_t)strtol(pc,&e,10): 0;
  if(pcnum<0) pcnum=0;
  if(pcnum>100) pcnum=100;

  uint16_t batt_col;
  if(ch)       batt_col=BATTERY_CHARGING; else
  if(pcnum>67) batt_col=BATTERY_HIGH;     else
  if(pcnum>33) batt_col=BATTERY_MED;
  else         batt_col=BATTERY_LOW;

  snprintf(g2dbuf, 64, "%d", pcnum);
  g2d_node_text(g2d_node, 10, 30, g2dbuf, batt_col, G2D_BLACK, 3);

  // hack alert - setting epoch ts from kbd
  if(cursor==0){
    snprintf(edit_word, 64, "%ld", (uint32_t)time_es());
    cursor=strlen(edit_word);
    in_word=true;
  }
  g2d_node_text(g2d_node, 110,20, edit_word, G2D_GREY_F, G2D_BLACK, 2);

  show_keyboard(g2d_node);
  // hack alert - setting epoch ts from kbd
}

static void word_cb(bool down, int16_t dx, int16_t dy, void* wi){
  static bool scrolled=false;
  if(!down){
    if(!scrolled && wi) word_index=(uint16_t)(uint32_t)wi;
    scrolled=false;
  }
  else
  if(dx+dy!=0){
    text_scroll_offset+=dy;
    scrolled=true;
  }
}

static void draw_notes(char* path, uint8_t g2d_node) {

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    snprintf(pathbuf, 64, "%s:text:1", path);
    char* line1=object_property(user, pathbuf);
    snprintf(pathbuf, 64, "%s:text:2", path);
    char* line2=object_property(user, pathbuf);
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_GREEN/6);
    g2d_node_text(g2d_node, 10,20, line1, G2D_WHITE, G2D_GREEN/6, 2);
    g2d_node_text(g2d_node, 10,40, line2, G2D_WHITE, G2D_GREEN/6, 2);
    return;
  }

  #define SIDE_MARGIN  20
  #define TOP_MARGIN   30
  #define LINE_HEIGHT  20
  #define WORD_SPACING  7
  #define CURSOR_WIDTH  3

  snprintf(g2dbuf, 64, "fps: %02d (%d,%d)", fps, touch_info.x, touch_info.y);
  g2d_node_text(g2d_node, 10,5, g2dbuf, G2D_BLUE, G2D_BLACK, 2);

  int16_t wd=g2d_node_width(g2d_node)-2*SIDE_MARGIN;
  int16_t ht=g2d_node_height(g2d_node)-2*TOP_MARGIN;
  if(wd<0 || ht<0) return;

  uint8_t text_container_g2d_node = g2d_node_create(g2d_node,
                                                    SIDE_MARGIN,TOP_MARGIN, wd,ht,
                                                    word_cb, 0);
  if(!text_container_g2d_node) return;

  snprintf(pathbuf, 64, "%s:text", path);
  uint16_t words=object_property_length(user, pathbuf);

  uint16_t lines=(words/2)+1;
  uint16_t scroll_height=lines*LINE_HEIGHT;

  if(in_word){
    word_index=words+1;
  }

  uint8_t text_scroll_g2d_node = g2d_node_create(text_container_g2d_node,
                                                 0, text_scroll_offset,
                                                 wd, scroll_height,
                                                 word_cb, 0);
  if(text_scroll_g2d_node){
    uint16_t k=0;
    uint16_t j=0;
    for(uint16_t w=1; w<=words+1; w++){

      int16_t available_width = wd-k;

      char*    word = 0;
      uint16_t word_width=available_width;
      if(w<=words){
        word = object_property_get_n(user, pathbuf, w);
        word_width=WORD_SPACING+g2d_text_width(word, 2);
      }
      if(k && word_width > available_width){
        // not fab way to "extend" the prev word out to full width for cursor
        g2d_node_create(text_scroll_g2d_node,
                        k, j*LINE_HEIGHT,
                        available_width, LINE_HEIGHT,
                        word_cb,(void*)(uint32_t)(w-1));
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
                                              word_cb,(void*)(uint32_t)w);

      char* word_to_show = word? word: (in_word? edit_word: "");
      g2d_node_text(word_g2d_node, 6,2, word_to_show, G2D_WHITE, G2D_BLACK, 2);
      k+=word_width;
    }
  }

  show_keyboard(g2d_node);

  show_touch_point(g2d_node);
}

extern uint32_t loop_time;

static void draw_about(char* path, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:cpu", path);
  char* cpu=object_property(user, pathbuf);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node, 0,0, g2d_node_width(g2d_node),g2d_node_height(g2d_node), G2D_CYAN/6);
//  uint32_t touch_events_percent = (100*touch_events_seen)/(1+touch_events);
//  snprintf(g2dbuf, 64, "%ld%% %ldms", touch_events_percent, loop_time);
    snprintf(g2dbuf, 64, "%dfps %ldms", fps, loop_time);
    g2d_node_text(g2d_node, 10,20, g2dbuf, G2D_WHITE, G2D_CYAN/6, 3);
    return;
  }

  snprintf(pathbuf, 64, "%s:build-info", path);
  char* bnf=object_property_values(user, pathbuf);

  snprintf(g2dbuf, 64, "fps: %d (%d,%d)", fps, touch_info.x, touch_info.y);
  g2d_node_text(g2d_node, 20, 40, g2dbuf, G2D_BLUE, G2D_BLACK, 2);

  snprintf(g2dbuf, 64, "cpu: %s", cpu);
  g2d_node_text(g2d_node, 10, 110, g2dbuf, G2D_BLUE, G2D_BLACK, 3);

  snprintf(g2dbuf, 64, "build: %s", bnf);
  g2d_node_text(g2d_node, 10, 190, g2dbuf, G2D_BLUE, G2D_BLACK, 1);
}

void draw_default(char* path, uint8_t g2d_node) {

  snprintf(pathbuf, 64, "%s:is", path);
  char* is=object_property_values(user, pathbuf);

  if(g2d_node_height(g2d_node) < ST7789_HEIGHT){
    g2d_node_rectangle(g2d_node,
                       0,0,
                       g2d_node_width(g2d_node),g2d_node_height(g2d_node),
                       G2D_MAGENTA);
    g2d_node_text(g2d_node, 10,20, is, G2D_BLACK, G2D_MAGENTA, 3);
    return;
  }
  snprintf(pathbuf, 64, "%s", path);
  char* uid=object_property(user, pathbuf);
  object* o=onex_get_from_cache(uid);
  static char bigvaluebuf[256];
  object_to_text(o, bigvaluebuf, 256, OBJECT_TO_TEXT_LOG);

  uint16_t p=0;
  uint16_t l=strlen(bigvaluebuf);
  uint16_t y=30;
  while(y<280 && p < l){
    g2d_node_text(g2d_node, 10, y, bigvaluebuf+p, G2D_BLUE, G2D_BLACK, 2);
    y+=20; p+=19;
  }
}

