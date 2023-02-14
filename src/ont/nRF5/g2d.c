
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <boards.h>

#include <onex-kernel/display.h>

#include "g2d.h"
#include "font57.h"

// ---------------------------------

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

// ---------------------------------

// drawing into huge buffer, plus basic fonts, from ATC1441
// XXX using ST7789_* here!

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

// XXX
//if(parent_y+y+h < 0)             return 0;
//if(parent_y+y   > ST7789_HEIGHT) return 0;
// XXX

  int16_t xtl=parent_xtl+x;
  int16_t ytl=parent_ytl+y;
  int16_t xbr=parent_xtl+x+w;
  int16_t ybr=parent_ytl+y+h;
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

static void show_overflow_warning(){
  #define WARNING_X 100
  #define WARNING_Y 100
  #define WARNING_S 50
  for(uint16_t py = WARNING_Y; py < WARNING_Y+WARNING_S; py++){
    for(uint16_t px = WARNING_X; px < WARNING_X+WARNING_S; px++){
      uint32_t i = 2 * (px + (py * ST7789_WIDTH));
      uint16_t colour = G2D_RED;
      lcd_buffer[i]   = colour >> 8;
      lcd_buffer[i+1] = colour & 0xff;
    }
  }
}

static void set_pixel(uint16_t x, uint16_t y, uint16_t colour) {

  uint32_t i = 2 * (x + (y * ST7789_WIDTH));

#define CHECK_BUFFER_OVERFLOW
#if defined(CHECK_BUFFER_OVERFLOW)  // can drop this once clipping in place
  if(x>=ST7789_WIDTH || y>=ST7789_HEIGHT || i+1 >= sizeof(lcd_buffer)){
    show_overflow_warning();
    return;
  }
#endif
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

static void draw_rect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t colour) {

  uint16_t yh=min(max(y+h,0), ST7789_HEIGHT);
  uint16_t xw=min(max(x+w,0), ST7789_WIDTH);

  for(uint16_t py = y; py < yh; py++){
    for(uint16_t px = x; px < xw; px++){
      set_pixel(px, py, colour);
    }
  }
}

void g2d_node_text(uint8_t node_id, int16_t x, int16_t y, char* text,
                   uint16_t colour, uint16_t bg, uint8_t size){

  if(!node_id) return;

  int16_t ox=scenegraph[node_id].xtl+x;
  int16_t oy=scenegraph[node_id].ytl+y;

  for(uint16_t p = 0; p < strlen(text); p++){

    int16_t xx = ox + (p * 6 * size);

    unsigned char c=text[p];

    if(c < 32 || c >= 127) c=' ';

    for(uint8_t i = 0; i < 5; i++) {

      uint8_t line = font57[c * 5 + i];

      for(uint8_t j = 0; j < 8; j++, line >>= 1){

        if(line & 1)     draw_rect(xx + i * size, oy + j * size, size, size, colour);
        else
        if(bg != colour) draw_rect(xx + i * size, oy + j * size, size, size, bg);
      }
    }
    if(bg != colour) draw_rect(xx + 5 * size, oy, size, 8 * size, bg);
  }
}

// ---------------------------------------------------

uint16_t g2d_node_width(uint8_t node_id){
  return scenegraph[node_id].xbr - scenegraph[node_id].xtl;
}

uint16_t g2d_node_height(uint8_t node_id){
  return scenegraph[node_id].ybr - scenegraph[node_id].ytl;
}

// ---------------------------------------------------

static bool is_inside(uint8_t n, int16_t x, int16_t y){
  if(x<scenegraph[n].xtl || x>scenegraph[n].xbr) return false;
  if(y<scenegraph[n].ytl || y>scenegraph[n].ybr) return false;
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










