#ifndef G2D_H
#define G2D_H

#include <onex-kernel/gfx.h>

void gfx_fast_init();
void gfx_fast_clear_screen(uint8_t colour); // 8 bits * 2, so 0x00 => 0x0000, 0x77 => 0x7777, etc
void gfx_fast_text(int32_t x, int32_t y, char* text, uint16_t colour, uint16_t bg, uint32_t size);
void gfx_fast_write_out_buffer();

#endif
