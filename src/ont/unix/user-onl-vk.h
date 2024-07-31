// ------- common/shared between user and vk code -------------

#include <pthread.h>
#include <stdbool.h>

#include "outline.h"

#include <onl-vk.h>

#define MAX_PANELS 32 // TODO set src/ont/unix/onx.vert
extern uint32_t num_panels;

#define MAX_VISIBLE_GLYPHS 4096
extern uint32_t num_glyphs;

extern mat4x4 proj_matrix;
extern mat4x4 view_l_matrix;
extern mat4x4 view_r_matrix;
extern mat4x4 model_matrix[MAX_PANELS];
extern vec4   text_ends[MAX_PANELS];

extern void*    glyph_data;
extern uint32_t glyph_data_size;
extern uint32_t glyph_info_size;
extern uint32_t glyph_cells_offset;
extern uint32_t glyph_cells_size;
extern uint32_t glyph_points_offset;
extern uint32_t glyph_points_size;

typedef struct fd_GlyphInstance {
  fd_Rect  rect;
  uint32_t glyph_index;
  float    sharpness;
} fd_GlyphInstance;

void set_up_scene_begin(float** vertices, fd_GlyphInstance** glyphs);
void set_up_scene_end();

void set_proj_view();
void load_font(char* font_face);



