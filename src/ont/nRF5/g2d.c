
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <boards.h>

#include <onex-kernel/display.h>

#include "g2d.h"
#include "font57.h"

// ---------------------------------

// drawing into huge buffer, plus basic fonts, from ATC1441
// XXX using ST7789_* here!

#define LCD_BUFFER_SIZE ((ST7789_WIDTH * ST7789_HEIGHT) * 2)
static uint8_t lcd_buffer[LCD_BUFFER_SIZE + 4];

// ---------------------------------

void g2d_set_pixel(int32_t x, int32_t y, uint16_t colour) {

  if(x<0 || y<0) return;

  int32_t i = 2 * (x + (y * ST7789_WIDTH));

  if(i+1 >= sizeof(lcd_buffer)) return;

  lcd_buffer[i]   = colour >> 8;
  lcd_buffer[i+1] = colour & 0xff;
}

void g2d_display_rect(int32_t x, int32_t y, uint32_t w, uint32_t h, uint16_t colour) {

  for(int px = x; px < (x + w); px++){
    for(int py = y; py < (y + h); py++){
      g2d_set_pixel(px, py,  colour);
    }
  }
}

static bool draw_char(int32_t x, int32_t y,
                      unsigned char c,
                      uint16_t colour, uint16_t bg, uint32_t size) {

  if (c < 32) return false;
  if (c >= 127) return false;

  for (int8_t i = 0; i < 5; i++) {

    uint8_t line = font57[c * 5 + i];

    for (int8_t j = 0; j < 8; j++, line >>= 1){

      if(line & 1)     g2d_display_rect(x + i * size, y + j * size, size, size, colour);
      else
      if(bg != colour) g2d_display_rect(x + i * size, y + j * size, size, size, bg);
    }
  }
  if(bg != colour) g2d_display_rect(x + 5 * size, y, size, 8 * size, bg);

  return true;
}

// ------------

void g2d_init() {

  display_fast_init();
}

void g2d_clear_screen(uint8_t colour) {

  memset(lcd_buffer, colour, LCD_BUFFER_SIZE);
}

// ------------


typedef struct sg_node sg_node;

typedef struct sg_node {

 int16_t x;
 int16_t y;
 // rot scale // not in parent like in the 3D stuff
 uint16_t w;
 uint16_t h;
 // bounding box set and clips, or set by g2d as max
 bool boost;   // give 25% extra bounding box on touch

 g2d_sprite_cb cb;
 void* cb_args;

 uint8_t parent;

} sg_node;

static sg_node scenegraph[256]; // 1..255

static volatile uint8_t next_node=1; // index is same as sprite id, 0th not used

uint8_t g2d_sprite_create(uint8_t  parent_id,
                          int16_t x,
                          int16_t y,
                          uint16_t w,
                          uint16_t h,
                          g2d_sprite_cb cb,
                          void* cb_args){

  if(!parent_id) next_node=1;

  if(next_node==256) return 0;

  int16_t offx=0;
  int16_t offy=0;
  if(parent_id){
    offx=scenegraph[parent_id].x;
    offy=scenegraph[parent_id].y;
  }
  scenegraph[next_node].x=offx+x;
  scenegraph[next_node].y=offy+y;
  scenegraph[next_node].w=w;
  scenegraph[next_node].h=h;
  scenegraph[next_node].boost=false;
  scenegraph[next_node].cb=cb;
  scenegraph[next_node].cb_args=cb_args;
  scenegraph[next_node].parent=parent_id;

  return next_node++;
}

void g2d_sprite_text(uint8_t sprite_id, int16_t x, int16_t y, char* text,
                     uint16_t colour, uint16_t bg, uint8_t size){

  int32_t ox=scenegraph[sprite_id].x+x;
  int32_t oy=scenegraph[sprite_id].y+y;

  int p = 0;
  for(int i = 0; i < strlen(text); i++){
    if(ox + (p * 6 * size) >= (ST7789_WIDTH - 6)){
      ox = -(p * 6 * size);
      oy += (8 * size);
    }
    if(draw_char(ox + (p * 6 * size), oy, text[i], colour, bg, size)) {
      p++;
    }
  }
}

void g2d_sprite_rectangle(uint8_t sprite_id,
                          int16_t x, int16_t y,
                          uint16_t w, uint16_t h,
                          uint16_t colour) {

  for(int py = y; py < (y + h); py++){
    for(int px = x; px < (x + w); px++){
      g2d_sprite_pixel(sprite_id, px, py, colour);
    }
  }
}

uint8_t g2d_sprite_pixel(uint8_t sprite_id,
                         int16_t x, int16_t y,
                         uint16_t colour){

  if(y<0 || y>=scenegraph[sprite_id].h) return G2D_Y_OUTSIDE;
  if(x<0 || x>=scenegraph[sprite_id].w) return G2D_X_OUTSIDE;

  int16_t offx=scenegraph[sprite_id].x;
  int16_t offy=scenegraph[sprite_id].y;

  int32_t i = 2 * ((offx + x) + ((offy + y) * ST7789_WIDTH));

  if(i<0 || i+1 >= sizeof(lcd_buffer)) return G2D_Y_OUTSIDE;

  lcd_buffer[i]   = colour >> 8;
  lcd_buffer[i+1] = colour & 0xff;

  return G2D_OK;
}

uint16_t g2d_sprite_width(uint8_t sprite_id){
  return scenegraph[sprite_id].w;
}

uint16_t g2d_sprite_height(uint8_t sprite_id){
  return scenegraph[sprite_id].h;
}

static bool is_inside(uint8_t n, int16_t x, int16_t y){
  int16_t bbx=scenegraph[n].x;
  int16_t bby=scenegraph[n].y;
  uint16_t bbw=scenegraph[n].w;
  uint16_t bbh=scenegraph[n].h;
  if(x<bbx || x>bbx+bbw) return false;
  if(y<bby || y>bby+bbh) return false;
  return true;
}

bool g2d_sprite_touch_event(bool down, uint16_t tx, uint16_t ty){

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

  for(uint8_t n=next_node-1; n; n--){

    if(!is_inside(n, tx, ty)) continue;

    uint8_t cbn=n;
    while(cbn && !scenegraph[cbn].cb) cbn=scenegraph[cbn].parent;
    if(!cbn) continue;

    scenegraph[cbn].cb(down, dx, dy, cbn, scenegraph[cbn].cb_args);
    return true;
  }
  return false;
}

void g2d_render() {

  display_fast_write_out_buffer(lcd_buffer, LCD_BUFFER_SIZE);
}

// ------------










