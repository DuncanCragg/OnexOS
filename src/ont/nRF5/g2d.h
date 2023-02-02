#ifndef G2D_H
#define G2D_H

#include <onex-kernel/gfx.h>

#define G2D_KEYBOARD_COMMAND_DELETE 0x01

typedef void (*g2d_keyboard_cb)(unsigned char ch, uint8_t command);

void g2d_init();
void g2d_clear_screen(uint8_t colour);
                   // 8 bits * 2, so 0x00 => 0x0000, 0x77 => 0x7777, etc
void g2d_text(int32_t x, int32_t y,
              char* text, uint16_t colour, uint16_t bg, uint32_t size);
void g2d_keyboard(g2d_keyboard_cb kcb);
void g2d_touch_event(uint16_t tx, uint16_t ty);
void g2d_write_out_buffer();

#endif
