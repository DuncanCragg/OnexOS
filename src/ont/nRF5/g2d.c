
// drawing into huge buffer, plus basic fonts, based on code from ATC1441

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <boards.h>

#include <onex-kernel/display.h>

#include "mathlib.h"
#include "g2d.h"
#include "font57.h"

// ---------------------------------
// XXX using ST7789_* here! define SCREEN_WIDTH in board file

#define LCD_BUFFER_SIZE ((ST7789_WIDTH * ST7789_HEIGHT) * 2)
static uint8_t lcd_buffer[LCD_BUFFER_SIZE + 4];

// ---------------------------------

void g2d_init() {

  display_fast_init();
}

void g2d_clear_screen(uint8_t colour) {

  memset(lcd_buffer, colour, LCD_BUFFER_SIZE);
}

void g2d_render() {

  display_fast_write_out_buffer(lcd_buffer, LCD_BUFFER_SIZE);
}

// ------------

typedef struct g2d_node g2d_node;

typedef struct g2d_node {

 int16_t xtl;
 int16_t ytl;
 int16_t xbr;
 int16_t ybr;

 uint16_t clip_xtl;
 uint16_t clip_ytl;
 uint16_t clip_xbr;
 uint16_t clip_ybr;

 g2d_node_cb cb;
 void*       cb_args;

 uint8_t parent;

} g2d_node;

static g2d_node scenegraph[256]; // 1..255

static volatile uint8_t next_node=1; // index is same as node id, 0th not used

uint8_t g2d_node_create(uint8_t parent_id,
                        int16_t x, int16_t y,
                        uint16_t w, uint16_t h,
                        g2d_node_cb cb,
                        void* cb_args){

  if(!parent_id) next_node=1;

  if(!next_node) return 0; // wrapped over 255 to 0

  int16_t  parent_xtl=0;
  int16_t  parent_ytl=0;
  uint16_t parent_clip_xtl=0;
  uint16_t parent_clip_ytl=0;
  uint16_t parent_clip_xbr=32000;
  uint16_t parent_clip_ybr=32000;
  if(parent_id){
    parent_xtl=scenegraph[parent_id].xtl;
    parent_ytl=scenegraph[parent_id].ytl;
    parent_clip_xtl=scenegraph[parent_id].clip_xtl;
    parent_clip_ytl=scenegraph[parent_id].clip_ytl;
    parent_clip_xbr=scenegraph[parent_id].clip_xbr;
    parent_clip_ybr=scenegraph[parent_id].clip_ybr;
  }
  int16_t xtl=parent_xtl+x;
  int16_t ytl=parent_ytl+y;
  int16_t xbr=parent_xtl+x+w;
  int16_t ybr=parent_ytl+y+h;

  if(xtl > parent_clip_xbr) return 0;
  if(ytl > parent_clip_ybr) return 0;
  if(xbr < parent_clip_xtl) return 0;
  if(ybr < parent_clip_ytl) return 0;

  scenegraph[next_node].xtl=xtl;
  scenegraph[next_node].ytl=ytl;
  scenegraph[next_node].xbr=xbr;
  scenegraph[next_node].ybr=ybr;

  scenegraph[next_node].clip_xtl=max(parent_clip_xtl, max(xtl,0));
  scenegraph[next_node].clip_ytl=max(parent_clip_ytl, max(ytl,0));
  scenegraph[next_node].clip_xbr=min(parent_clip_xbr, max(xbr,0));
  scenegraph[next_node].clip_ybr=min(parent_clip_ybr, max(ybr,0));

  scenegraph[next_node].cb=cb;
  scenegraph[next_node].cb_args=cb_args;
  scenegraph[next_node].parent=parent_id;

  return next_node++;
}

static void set_pixel(uint16_t x, uint16_t y, uint16_t colour) {
  uint32_t i = 2 * (x + (y * ST7789_WIDTH));
  lcd_buffer[i]   = colour >> 8;
  lcd_buffer[i+1] = colour & 0xff;
}

// ------------- geometry

void g2d_node_rectangle(uint8_t node_id,
                        int16_t x, int16_t y,
                        uint16_t w, uint16_t h,
                        uint16_t colour) {
  if(!node_id) return;

  uint16_t xtlx=max(scenegraph[node_id].xtl+x,0);
  uint16_t ytly=max(scenegraph[node_id].ytl+y,0);
  uint16_t xbrw=max(xtlx+w,0);
  uint16_t ybrh=max(ytly+h,0);
  uint16_t cxtl=max(xtlx, scenegraph[node_id].clip_xtl);
  uint16_t cytl=max(ytly, scenegraph[node_id].clip_ytl);
  uint16_t cxbr=min(xbrw, scenegraph[node_id].clip_xbr);
  uint16_t cybr=min(ybrh, scenegraph[node_id].clip_ybr);

  for(uint16_t py = cytl; py < cybr; py++){
    for(uint16_t px = cxtl; px < cxbr; px++){
      set_pixel(px, py, colour);
    }
  }
}

// ------------- text

void g2d_node_text(uint8_t node_id, int16_t x, int16_t y,
                   uint16_t colour, uint16_t bg, uint8_t size, char* text){

  if(!node_id) return;

  int16_t ox=scenegraph[node_id].xtl+x;
  int16_t oy=scenegraph[node_id].ytl+y;

  uint16_t cxtl=scenegraph[node_id].clip_xtl;
  uint16_t cytl=scenegraph[node_id].clip_ytl;
  uint16_t cxbr=scenegraph[node_id].clip_xbr;
  uint16_t cybr=scenegraph[node_id].clip_ybr;

  for(uint16_t p = 0; p < strlen(text); p++){

    int16_t xx = ox + (p * 6 * size);

    unsigned char c=text[p];

    if(c < 32 || c >= 127) c=' ';

    for(uint8_t i = 0; i < 6; i++) {

      uint8_t line = i<5? font57[c * 5 + i]: 0;

      int16_t rx=xx + i * size;
      if(rx<cxtl || rx>=cxbr) continue;

      for(uint8_t j = 0; j < 8; j++, line >>= 1){

        int16_t ry=oy + j * size;
        if(ry<cytl || ry>=cybr) continue;

        uint16_t col=(line & 1)? colour: bg;

        uint16_t yh=ry+size;
        uint16_t xw=rx+size;
        if(yh > cybr) yh=cybr;
        if(xw > cxbr) xw=cxbr;

        for(uint16_t py = ry; py < yh; py++){
          for(uint16_t px = rx; px < xw; px++){
            set_pixel(px, py, col);
          }
        }
      }
    }
  }
}

// ---------------------------------------------------

uint16_t g2d_text_width(char* text, uint8_t size){
  uint16_t n=strlen(text);
  return n*6*size;
}

uint16_t g2d_node_width(uint8_t node_id){
  if(!node_id) return 0;
  return scenegraph[node_id].xbr - scenegraph[node_id].xtl;
}

uint16_t g2d_node_height(uint8_t node_id){
  if(!node_id) return 0;
  return scenegraph[node_id].ybr - scenegraph[node_id].ytl;
}

// ---------------------------------------------------

static bool is_inside(uint8_t n, int16_t x, int16_t y){
  if(x<scenegraph[n].clip_xtl || x>scenegraph[n].clip_xbr) return false;
  if(y<scenegraph[n].clip_ytl || y>scenegraph[n].clip_ybr) return false;
  return true;
}

void g2d_node_touch_event(bool down, uint16_t tx, uint16_t ty){

  static uint16_t last_tx=0;
  static uint16_t last_ty=0;

  int16_t dx=0;
  int16_t dy=0;

  if(down){
    if(last_tx + last_ty){
      dx = (tx - last_tx);
      dy = (ty - last_ty);
    }
    last_tx=tx;
    last_ty=ty;
  }
  else{
    last_tx=0;
    last_ty=0;
  }

  static uint8_t     cb_node=0;
  static g2d_node_cb drag_cb=0;
  static void*       drag_cb_args=0;

  for(uint8_t n=next_node-1; n && !cb_node; n--){

    if(!is_inside(n, tx, ty)) continue;

    cb_node=n;
    while(cb_node && !scenegraph[cb_node].cb) cb_node=scenegraph[cb_node].parent;
  }

  if(cb_node){
    if(!drag_cb){
      drag_cb=scenegraph[cb_node].cb;
      drag_cb_args=scenegraph[cb_node].cb_args;
    }
    drag_cb(down, dx, dy, drag_cb_args);
  }

  if(!down){
    cb_node=0;
    drag_cb=0;
    drag_cb_args=0;
  }
}

// ------------










