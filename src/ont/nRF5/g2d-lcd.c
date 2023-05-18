
// drawing into huge buffer, plus basic fonts, based on code from ATC1441

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <boards.h>

#include <onex-kernel/display.h>

#include <mathlib.h>
#include <g2d.h>
#include <g2d-internal.h>

#include <font57.h>

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

static void set_pixel(uint16_t x, uint16_t y, uint16_t colour) {
  uint32_t i = 2 * (x + (y * ST7789_WIDTH));
  lcd_buffer[i]   = colour >> 8;
  lcd_buffer[i+1] = colour & 0xff;
}

void g2d_internal_rectangle(uint16_t cxtl, uint16_t cytl,
                            uint16_t cxbr, uint16_t cybr,
                            uint16_t colour){

  for(uint16_t py = cytl; py < cybr; py++){
    for(uint16_t px = cxtl; px < cxbr; px++){
      set_pixel(px, py, colour);
    }
  }
}

void g2d_internal_text(int16_t ox, int16_t oy,
                       uint16_t cxtl, uint16_t cytl,
                       uint16_t cxbr, uint16_t cybr,
                       char* text, uint16_t colour, uint16_t bg,
                       uint8_t size){

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

uint16_t g2d_text_width(char* text, uint8_t size){
  uint16_t n=strlen(text);
  return n*6*size;
}

// ---------------------------------------------------
