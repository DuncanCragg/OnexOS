
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

bool g2d_draw_char(int32_t x, int32_t y, unsigned char c, uint16_t colour, uint16_t bg, uint32_t size) {

  if (c < 32) return false;
  if (c >= 127) return false;

  for (int8_t i = 0; i < 5; i++) {

    uint8_t line = font57[c * 5 + i];

    for (int8_t j = 0; j < 8; j++, line >>= 1){

      if(line & 1)          g2d_display_rect(x + i * size, y + j * size, size, size, colour);
      else if(bg != colour) g2d_display_rect(x + i * size, y + j * size, size, size, bg);
    }
  }
  if(bg != colour) g2d_display_rect(x + 5 * size, y, size, 8 * size, bg);

  return true;
}



// ------ keyboard -----

uint8_t key_index(uint16_t x, uint16_t y) {

  uint8_t xstart=35;
  uint8_t xend  =170;
  uint8_t ystart=130;
  uint8_t xinc  =30;
  if(x<xstart) x=xstart;
  if(x>xend)   x=xend;
  if(y<ystart) y=ystart;
  if(y>=130 && y<=165){
    return 1+(x-xstart)/xinc;
  }
  else
  if(y>165 && y<=200){
    return 6+(x-xstart)/xinc;
  }
  else
  if(y>200 && y<=235){
    return 11+(x-xstart)/xinc;
  }
  else
  if(y>235 && y<=280){
    return 16+(x-xstart)/xinc;
  }
  return 0;
} // key_index

static uint8_t kbpg=1;

static char key_indexes_page[7][21]={

  { "      " },
  { " "
    "ERTIO"
    "ASDGH"
    "CBNM>"
    "^#  ~" },
  { " "
    "QWYUP"
    "FJKL "
    " ZXV<"
    "^#  ~" },
  { " "
    "+ ()-"
    "/'#@*"
    "%!;:>"
    "^#,.~" },
  { " "
    "^&[]_"
    " \"{}~"
    " ?<><"
    "^#,.~" },
  { " "
    "+123-"
    "/456*"
    "%789>"
    "^#0.=" },
  { " "
    "^123e"
    " 456 "
    " 789<"
    "^#0.=" }};

static g2d_keyboard_cb kbd_cb=0;

void g2d_keyboard(g2d_keyboard_cb kcb) {

    kbd_cb = kcb;

    uint8_t kbdstart_x=15;
    uint8_t kbdstart_y=115;
    uint8_t rowspacing=40;

    #define SELECT_PAGE 15
    #define SELECT_TYPE 16
    #define DELETE_LAST 17

    static char buf[64];

    for(uint8_t j=0; j<4; j++){

      snprintf(buf, 64, "%c %c %c %c %c",
                         key_indexes_page[kbpg][1+j*5],
                         key_indexes_page[kbpg][2+j*5],
                         key_indexes_page[kbpg][3+j*5],
                         key_indexes_page[kbpg][4+j*5],
                         key_indexes_page[kbpg][5+j*5]);

      g2d_text(kbdstart_x, kbdstart_y+rowspacing*(j), buf, G2D_BLACK, G2D_WHITE, 4);
    }
}

void g2d_touch_event(uint16_t tx, uint16_t ty) {

  uint8_t ki = key_index(tx, ty);
  if(ki==SELECT_TYPE){
    if(kbpg==1 || kbpg==2) kbpg=3;
    else
    if(kbpg==3 || kbpg==4) kbpg=5;
    else
    if(kbpg==5 || kbpg==6) kbpg=1;
  }
  else
  if(ki==SELECT_PAGE){
    if(kbpg==1) kbpg=2;
    else
    if(kbpg==2) kbpg=1;
    else
    if(kbpg==3) kbpg=4;
    else
    if(kbpg==4) kbpg=3;
    else
    if(kbpg==5) kbpg=6;
    else
    if(kbpg==6) kbpg=5;
  }
  else
  if(ki==DELETE_LAST){

    if(kbd_cb) kbd_cb(0, G2D_KEYBOARD_COMMAND_DELETE);
  }
  else
  if(ki>=1){

    if(kbd_cb) kbd_cb(key_indexes_page[kbpg][ki], 0);

    if(kbpg==2 || kbpg==6) kbpg--;
    else
    if(kbpg==3 || kbpg==4) kbpg=1;
  }
}

// ------------

void g2d_text(int32_t x, int32_t y, char* text, uint16_t colour, uint16_t bg, uint32_t size) {

  int p = 0;
  for(int i = 0; i < strlen(text); i++){
    if(x + (p * 6 * size) >= (ST7789_WIDTH - 6)){
      x = -(p * 6 * size);
      y += (8 * size);
    }
    if(g2d_draw_char(x + (p * 6 * size), y, text[i], colour, bg, size)) {
      p++;
    }
  }
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

 uint16_t x;
 uint16_t y;
 // rot scale // not in parent like in the 3D stuff
 uint16_t w;
 uint16_t h;
 // bounding box set and clips, or set by g2d as max
 bool boost;   // give 25% extra bounding box on touch

 g2d_sprite_cb cb;
 void* cb_args;

 uint8_t parent;   // back up to parent from any child
 uint8_t siblings; // list of parent's children
 uint8_t children; // first in siblings list

} sg_node;

static sg_node scenegraph[256]; // 1..255

static uint8_t next_node=1; // index is same as sprite id, 0th not used

uint8_t g2d_sprite_create(uint16_t parent_id,
                          uint16_t x,
                          uint16_t y,
                          uint16_t w,
                          uint16_t h,
                          g2d_sprite_cb cb,
                          void* cb_args){

  if(next_node==256) return 0;

  scenegraph[next_node].x=x;
  scenegraph[next_node].y=y;
  scenegraph[next_node].w=w;
  scenegraph[next_node].h=h;
  scenegraph[next_node].boost=false;
  scenegraph[next_node].cb=cb;
  scenegraph[next_node].cb_args=cb_args;

  scenegraph[next_node].parent=parent_id;
  scenegraph[next_node].siblings=0;
  scenegraph[next_node].children=0;

  if(parent_id){
    uint8_t siblings=scenegraph[parent_id].children;
    if(!siblings){
      scenegraph[parent_id].children=next_node;
    }
    else{
      do{
        uint8_t sibling=scenegraph[siblings].siblings;
        if(!sibling){
          scenegraph[siblings].siblings=next_node;
          break;
        }
        siblings=sibling;
      } while(true);
    }
  }

  return next_node++;
}

static void get_offsets(uint8_t sprite_id, uint16_t* x, uint16_t* y){
  // climb the ancestor tree
  *x=scenegraph[sprite_id].x;
  *y=scenegraph[sprite_id].y;
}

uint8_t g2d_sprite_text(uint8_t sprite_id, uint16_t x, uint16_t y, char* text,
                        uint16_t colour, uint16_t bg, uint8_t size){

  uint16_t offx, offy;
  get_offsets(sprite_id, &offx, &offy);
  g2d_text(offx+x,offy+y, text, colour, bg, size);

  return G2D_OK;
}

void g2d_sprite_rectangle(uint8_t sprite_id,
                          uint16_t x, uint16_t y,
                          uint16_t w, uint16_t h,
                          uint16_t colour) {
  uint8_t r;
  for(int py = y; py < (y + h); py++){
    for(int px = x; px < (x + w); px++){
      r=g2d_sprite_pixel(sprite_id, px, py, colour);
      if(r!=G2D_OK) break;
    }
    if(r==G2D_Y_OUTSIDE) break;
  }
}

uint8_t g2d_sprite_pixel(uint8_t sprite_id,
                         uint16_t x, uint16_t y,
                         uint16_t colour){

  if(x<0) return G2D_X_OUTSIDE;
  if(y<0) return G2D_Y_OUTSIDE;
  if(x>=scenegraph[sprite_id].w) return G2D_X_OUTSIDE;
  if(y>=scenegraph[sprite_id].h) return G2D_Y_OUTSIDE;

  uint16_t offx, offy;
  get_offsets(sprite_id, &offx, &offy);

  int32_t i = 2 * ((offx + x) + ((offy + y) * ST7789_WIDTH));

  if(i+1 >= sizeof(lcd_buffer)) return G2D_OK;

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

void g2d_render() {

  next_node=1;
  display_fast_write_out_buffer(lcd_buffer, LCD_BUFFER_SIZE);
}

// ------------










