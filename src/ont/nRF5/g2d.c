
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <boards.h>

#include <onex-kernel/display.h>

#include "g2d.h"
#include "font57.h"

// ---------------------------------

// drawing into huge buffer, plus basic fonts, from ATC1441

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

void g2d_write_out_buffer() {

  display_fast_write_out_buffer(lcd_buffer, LCD_BUFFER_SIZE);
}

// ------------


