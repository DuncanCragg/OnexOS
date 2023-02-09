#ifndef G2D_H
#define G2D_H

#define G2D_WHITE   0xffff
#define G2D_BLACK   0x0000
#define G2D_GREY_1A 0xd69a // 1.1.0.1:0/1.1.0:1.0.0/1:1.0.1.0
#define G2D_GREY_F  0x7bef // 0.1.1.1:1/0.1.1:1.1.1/0:1.1.1.1
#define G2D_GREY_7  0x39e7 // 0.0.1.1:1/0.0.1:1.1.1/0:0.1.1.1
#define G2D_GREY_3  0x18e3 // 0.0.0.1:1/0.0.0:1.1.1/0:0.0.1.1
#define G2D_GREY_1  0x0861 // 0.0.0.0:1/0.0.0:0.1.1/0:0.0.0.1
#define G2D_RED     0xf800 // 1.1.1.1:1/0.0.0:0.0.0/0:0.0.0.0
#define G2D_GREEN   0x07e0 // 0.0.0.0:0/1.1.1:1.1.1/0:0.0.0.0
#define G2D_BLUE    0x001f // 0.0.0.0:0/0.0.0:0.0.0/1:1.1.1.1
#define G2D_YELLOW  0xffe0 // 1.1.1.1:1/1.1.1:1.1.1/0:0.0.0.0
#define G2D_MAGENTA 0xf81f // 1.1.1.1:1/0.0.0:0.0.0/1:1.1.1.1
#define G2D_CYAN    0x07ff // 0.0.0.0:0/1.1.1:1.1.1/1:1.1.1.1

#define G2D_RGB256(r,g,b) ((((uint16_t)(r)&0xf8)<<8)|(((uint16_t)(g)&0xfc)<<3)|((uint16_t)(b)>>3))

void g2d_init();
void g2d_clear_screen(uint8_t colour);
                   // 8 bits * 2, so 0x00 => 0x0000, 0x77 => 0x7777, etc
void g2d_text(int32_t x, int32_t y,
              char* text, uint16_t colour, uint16_t bg, uint32_t size);
void g2d_render();

// ------ sprites

typedef void (*g2d_sprite_cb)(uint8_t sprite_id, void* args);

/*
 Create sprite at (x,y) size (w,h) with cb(id, args).
 Returns id (which increments from 1)
 If parent_id==0 then it's the root sprite and the
 whole previous tree is discarded so can be rebuilt.
 max # sprites: 255
*/
uint8_t g2d_sprite_create(uint8_t  parent_id,
                          uint16_t x,
                          uint16_t y,
                          uint16_t w,
                          uint16_t h,
                          g2d_sprite_cb cb,
                          void* cb_args);

#define G2D_OK         0
#define G2D_X_OUTSIDE  1
#define G2D_Y_OUTSIDE  2

uint8_t g2d_sprite_text(uint8_t sprite_id, uint16_t x, uint16_t y, char* text,
                        uint16_t colour, uint16_t bg, uint8_t size);

void g2d_sprite_rectangle(uint8_t sprite_id, uint16_t x, uint16_t y,
                          uint16_t w, uint16_t h, uint16_t colour);

uint8_t g2d_sprite_pixel(uint8_t sprite_id, uint16_t x, uint16_t y, uint16_t colour);

uint16_t g2d_sprite_width(uint8_t sprite_id);
uint16_t g2d_sprite_height(uint8_t sprite_id);

/*
 Fire callback for touch events in deepest/newest bounding box.
*/
bool g2d_sprite_touch_event(uint16_t tx, uint16_t ty);

#endif
