
#include <stdint.h>
#include <stdbool.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "user-onl-vk.h"

#include <g2d.h>
#include <g2d-internal.h>

// ---------------------------------------------------

// I don't know why this dramatic compensation is needed
#define ODD_Y_COMPENSATION 0.70f
#define Y_UP_OFFSET        2.0f

void g2d_init() {
}

bool g2d_clear_screen(uint8_t colour) {
/*
  panel p ={
   .dimensions = { 1.0f, 1.0f, 0.03f },
   .position   = { 0, Y_UP_OFFSET, -2.19f },
   .rotation   = { 0.0f, 0.0f, 0.0f },
  };
*/
  return true;
}

void g2d_render() {
}

void g2d_internal_rectangle(uint16_t cxtl, uint16_t cytl,
                            uint16_t cxbr, uint16_t cybr,
                            uint16_t colour){

  float w=  (cxbr-cxtl)/(SCREEN_WIDTH *1.0f);
  float h=( (cybr-cytl)/(SCREEN_HEIGHT*1.0f))*ODD_Y_COMPENSATION;
  float x=(-1.00f+cxtl/(SCREEN_WIDTH /2.0f)+w);
  float y=( 1.00f-cytl/(SCREEN_HEIGHT/2.0f)-h)*ODD_Y_COMPENSATION+Y_UP_OFFSET;

  #if defined(G2D_VULKAN_VERBOSE)
  log_write("g2d_internal_rectangle() %d,%d %d,%d\n", cxtl, cytl, cxbr, cybr);
  log_write("(x=%.2f,y=%.2f)(w=%.2f,h=%.2f)\n", x,y, w,h);
  #endif
/*
  panel p ={
   .dimensions = { w, h, 0.03f },
   .position   = { x, y, -2.2f },
   .rotation   = { 0.0f, 0.0f, 0.0f },
  };
*/
}


void g2d_internal_text(int16_t ox, int16_t oy,
                       uint16_t cxtl, uint16_t cytl,
                       uint16_t cxbr, uint16_t cybr,
                       char* text, uint16_t colour, uint16_t bg,
                       uint8_t size){

  #if defined(G2D_VULKAN_VERBOSE)
  log_write("g2d_internal_text() %d,%d %d,%d %d,%d '%s'\n",
                                 ox, oy, cxtl, cytl, cxbr, cybr, text);
  #endif

  float left = -1.00f+ox/(SCREEN_WIDTH /2.0f);
  float top  =(-1.00f+oy/(SCREEN_HEIGHT/2.0f))*ODD_Y_COMPENSATION+0.05f;
  float w = 1.0f;
  float h = 1.0f;
  float scale = (size/2.0f)*(w/17000.0f);

  // add_text_whs(left, top, w, h, scale, text);
}


uint16_t g2d_text_width(char* text, uint8_t size){

  #if defined(G2D_VULKAN_VERBOSE)
  log_write("g2d_text_width() '%s'\n", text);
  #endif

  uint16_t n=strlen(text);
  return n*3*size;
}



