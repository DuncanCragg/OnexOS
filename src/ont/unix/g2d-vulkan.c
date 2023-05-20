
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "ont/unix/user-onx-vk.h"
#include "ont/unix/linmath-plus.h"

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onn.h>
#include <onr.h>

#include <onex-kernel/log.h>

#include <mathlib.h>

#include <g2d.h>
#include <g2d-internal.h>

// ---------------------------------

uint32_t num_panels;
uint32_t num_glyphs;

mat4x4 model_matrix[MAX_PANELS];
vec4   text_ends[MAX_PANELS];

void*    glyph_data;
uint32_t glyph_data_size;
uint32_t glyph_info_size;
uint32_t glyph_cells_offset;
uint32_t glyph_cells_size;
uint32_t glyph_points_offset;
uint32_t glyph_points_size;

#define NUMBER_OF_GLYPHS 96
uint32_t num_glyph_chars=NUMBER_OF_GLYPHS;

typedef struct panel {

  vec3  dimensions;
  vec3  position;
  vec3  rotation;
  char* text;

} panel;

panel welcome_banner ={
 .dimensions = { 1.0f, 0.4f, 0.03f },
 .position   = { 3.0f, 2.0f, -2.0f },
 .rotation   = { 0.0f, 0.0f, 0.0f },
 .text = "Hello, and welcome to OnexOS and the Object Network! This is your place to hang out with friends and family and chill without surveillance or censorship. Big Tech has had their way for decades, now it's time to bring the power of our technology back to ourselves! OnexOS is a freeing and empowering operating system. We hope you enjoy using it! Hello, and welcome to OnexOS and the Object Network! This is your place to hang out with friends and family and chill without surveillance or censorship. Big Tech has had their way for decades, now it's time to bring the power of our technology back to ourselves! OnexOS is a freeing and empowering operating system. We hope you enjoy using it! ",
};

panel document ={
 .dimensions = {  0.3f, 0.4f,  0.01f },
 .position   = {  0.0f, 1.5f, -14.0f },
 .rotation   = {  0.0f, 180.0f, 0.0f },
 .text = "There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. ",
};

panel info_board ={
 .dimensions = { 4.0f,   2.0f,  0.03f },
 .position   = { 7.0f,   2.0f, -1.0f },
 .rotation   = { 0.0f, -45.0f,  0.0f },
 .text = "OnexOS is an OS with no apps! Instead of apps, there are just things! Your stuff, and you yourselves, exist in this shared space. OnexOS is an OS with no apps! Instead of apps, there are just things! Your stuff, and you yourselves, exist in this shared space. OnexOS is an OS with no apps! Instead of apps, there are just things! Your stuff, and you yourselves, exist in this shared space. ",
};

panel room_floor ={
 .dimensions = {  3.0f, 3.0f,   0.2f },
 .position   = {  0.0f, 0.01f, -14.0f },
 .rotation   = { 90.0f, 0.0f,   0.0f },
 .text = "v",
};

panel room_ceiling ={
 .dimensions = {   3.0f, 3.0f,  0.2f },
 .position   = {   0.0f, 6.0f, -14.0f },
 .rotation   = { -90.0f, 0.0f,  0.0f },
 .text = "^",
};

panel room_wall_1 ={
 .dimensions = {  3.0f,  3.0f,  0.2f },
 .position   = { -3.0f,  3.0f, -14.0f },
 .rotation   = {  0.0f, 90.0f,  0.0f },
 .text = "wall 1",
};

panel room_wall_2 ={
 .dimensions = { 3.0f,   3.0f,  0.2f },
 .position   = { 3.0f,   3.0f, -14.0f },
 .rotation   = { 0.0f, -90.0f,  0.0f },
 .text = "wall 2",
};

panel room_wall_3 ={
 .dimensions = { 3.0f,   3.0f,  0.2f },
 .position   = { 0.0f,   3.0f, -17.0f },
 .rotation   = { 0.0f, 180.0f,  0.0f },
 .text = "wall 3",
};

static float    vertex_buffer_data[MAX_PANELS*6*6*3];
static uint32_t vertex_buffer_end=0;

static float    uv_buffer_data[MAX_PANELS*6*6*2];
static uint32_t uv_buffer_end=0;

static uint32_t align_uint32(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

typedef struct fd_CellInfo {
    uint32_t point_offset;
    uint32_t cell_offset;
    uint32_t cell_count_x;
    uint32_t cell_count_y;
} fd_CellInfo;

typedef struct fd_HostGlyphInfo {
    fd_Rect bbox;
    float advance;
} fd_HostGlyphInfo;

typedef struct fd_DeviceGlyphInfo {
    fd_Rect bbox;
    //fd_Rect cbox;
    fd_CellInfo cell_info;
} fd_DeviceGlyphInfo;

static fd_Outline       outlines[NUMBER_OF_GLYPHS];
static fd_HostGlyphInfo glyph_infos[NUMBER_OF_GLYPHS];

void load_font(char* font_face, uint32_t alignment) {

  FT_Library library;
  FT_CHECK(FT_Init_FreeType(&library));

  FT_Face face;
  int err=FT_New_Face(library, font_face, 0, &face);
  if(err){
    log_write("Font loading failed or font not found: %s\n", font_face);
    onl_exit(-1);
  }

  FT_CHECK(FT_Set_Char_Size(face, 0, 1000 * 64, 96, 96));

  uint32_t total_points = 0;
  uint32_t total_cells = 0;

  for (uint32_t i = 0; i < num_glyph_chars; i++) {

      char c = ' ' + i;
      // log_write("%c", c);

      fd_Outline *o = &outlines[i];
      fd_HostGlyphInfo *hgi = &glyph_infos[i];

      FT_UInt glyph_index = FT_Get_Char_Index(face, c);
      FT_CHECK(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING));

      fd_outline_convert(&face->glyph->outline, o, c);

      hgi->bbox = o->bbox;
      hgi->advance = face->glyph->metrics.horiAdvance / 64.0f;

      total_points += o->num_of_points;
      total_cells += o->cell_count_x * o->cell_count_y;
  }
  log_write("\n");

  glyph_info_size = sizeof(fd_DeviceGlyphInfo) * num_glyph_chars;
  glyph_cells_size = sizeof(uint32_t) * total_cells;
  glyph_points_size = sizeof(vec2) * total_points;

  glyph_cells_offset = align_uint32(glyph_info_size, alignment);
  glyph_points_offset = align_uint32(glyph_info_size + glyph_cells_size, alignment);
  glyph_data_size = glyph_points_offset + glyph_points_size;
  glyph_data = malloc(glyph_data_size);

  fd_DeviceGlyphInfo *device_glyph_infos = (fd_DeviceGlyphInfo*)((char*)glyph_data);

  uint32_t *cells = (uint32_t*)((char*)glyph_data + glyph_cells_offset);
  vec2 *points = (vec2*)((char*)glyph_data + glyph_points_offset);

  uint32_t point_offset = 0;
  uint32_t cell_offset = 0;

  for (uint32_t i = 0; i < num_glyph_chars; i++) {

      fd_Outline *o = &outlines[i];
      fd_DeviceGlyphInfo *dgi = &device_glyph_infos[i];

      dgi->cell_info.cell_count_x = o->cell_count_x;
      dgi->cell_info.cell_count_y = o->cell_count_y;
      dgi->cell_info.point_offset = point_offset;
      dgi->cell_info.cell_offset = cell_offset;
      dgi->bbox = o->bbox;

      uint32_t cell_count = o->cell_count_x * o->cell_count_y;
      memcpy(cells + cell_offset, o->cells, sizeof(uint32_t) * cell_count);
      memcpy(points + point_offset, o->points, sizeof(vec2) * o->num_of_points);

      //fd_outline_u16_points(o, &dgi->cbox, points + point_offset);

      point_offset += o->num_of_points;
      cell_offset += cell_count;
  }

  assert(point_offset == total_points);
  assert(cell_offset == total_cells);

  for (uint32_t i = 0; i < num_glyph_chars; i++){
      fd_outline_destroy(&outlines[i]);
  }

  FT_CHECK(FT_Done_Face(face));
  FT_CHECK(FT_Done_FreeType(library));
}

static void make_box(vec3 dimensions){

  float w=dimensions[0];
  float h=dimensions[1];
  float d=dimensions[2];

  float verts[6*6*3] = {

    -w, -h,  d,  // -X side
    -w, -h,  0,
    -w,  h,  0,

    -w,  h,  0,
    -w,  h,  d,
    -w, -h,  d,

    -w, -h,  d,  // -Z side
     w,  h,  d,
     w, -h,  d,

    -w, -h,  d,
    -w,  h,  d,
     w,  h,  d,

    -w, -h,  d,  // -Y side
     w, -h,  d,
     w, -h,  0,

    -w, -h,  d,
     w, -h,  0,
    -w, -h,  0,

    -w,  h,  d,  // +Y side
    -w,  h,  0,
     w,  h,  0,

    -w,  h,  d,
     w,  h,  0,
     w,  h,  d,

     w,  h,  d,  // +X side
     w,  h,  0,
     w, -h,  0,

     w, -h,  0,
     w, -h,  d,
     w,  h,  d,

    -w,  h,  0,  // +Z side
    -w, -h,  0,
     w,  h,  0,

    -w, -h,  0,
     w, -h,  0,
     w,  h,  0,
  };

  memcpy((void*)vertex_buffer_data + vertex_buffer_end, (const void*)verts, sizeof(verts));

  vertex_buffer_end += sizeof(verts);

  float uvs[6*6*2] = {

    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,

    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,

    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,

    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
  };

  memcpy((void*)uv_buffer_data + uv_buffer_end, (const void*)uvs, sizeof(uvs));

  uv_buffer_end += sizeof(uvs);
}

static void add_panel(panel* panel, int p){

    make_box(panel->dimensions);

    mat4x4_translation(model_matrix[p], panel->position[0],
                                        panel->position[1],
                                        panel->position[2]);
    mat4x4 mm;
    mat4x4_rotate_X(mm, model_matrix[p], (float)degreesToRadians(panel->rotation[0]));
    mat4x4_rotate_Y(model_matrix[p], mm, (float)degreesToRadians(panel->rotation[1]));
    mat4x4_rotate_Z(mm, model_matrix[p], (float)degreesToRadians(panel->rotation[2]));
    mat4x4_orthonormalize(model_matrix[p], mm);
}

static float* vertices;

static fd_GlyphInstance* glyphs;

static void add_text_whs(float left, float top,
                         float w,    float h,
                         float scale,
                         char* text           ){
    float x = left;
    float y = top;

    while (*text) {

        if (num_glyphs >= MAX_VISIBLE_GLYPHS) break;

        uint32_t glyph_index = *text - 32;

        fd_HostGlyphInfo *gi   = &glyph_infos[glyph_index];
        fd_GlyphInstance *inst = &glyphs[num_glyphs];

        inst->rect.min_x =  x + gi->bbox.min_x * scale;
        inst->rect.min_y = -y + gi->bbox.min_y * scale;
        inst->rect.max_x =  x + gi->bbox.max_x * scale;
        inst->rect.max_y = -y + gi->bbox.max_y * scale;

        inst->glyph_index = glyph_index;
        inst->sharpness = scale * 2500;

        if (inst->rect.max_x < w) {
        }
        else
        if(inst->rect.min_y > top) {

            x = left;
            y += scale*2000.0f;

            inst->rect.min_x =  x + gi->bbox.min_x * scale;
            inst->rect.min_y = -y + gi->bbox.min_y * scale;
            inst->rect.max_x =  x + gi->bbox.max_x * scale;
            inst->rect.max_y = -y + gi->bbox.max_y * scale;
        }
        else break;

        num_glyphs++;
        text++;
        x += gi->advance * scale;
    }
}

static void add_text(panel* panel, int p) {

  char *text = panel->text;

  float w = panel->dimensions[0];
  float h = panel->dimensions[1];
  float left = -w/2.17f;
  float top  = -h/2.63f;
  float scale = w/30000.0f;

  uint32_t s=num_glyphs;
  add_text_whs(left, top, w, h, scale, text);
  uint32_t e=num_glyphs;

  text_ends[p][0]=s;
  text_ends[p][1]=e;
}

// -----------------------------------------

static void do_3d_stuff() {

  info_board.rotation[1]+=0.5f;

  if(num_panels<MAX_PANELS){
    add_panel(&welcome_banner, num_panels);
    add_text(&welcome_banner,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");

  if(num_panels<MAX_PANELS){
    add_panel(&document, num_panels);
    add_text(&document,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");

  if(num_panels<MAX_PANELS){
    add_panel(&info_board, num_panels);
    add_text(&info_board,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");

  if(num_panels<MAX_PANELS){
    add_panel(&room_floor, num_panels);
    add_text(&room_floor,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");

  if(num_panels<MAX_PANELS){
    add_panel(&room_ceiling, num_panels);
    add_text(&room_ceiling,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");

  if(num_panels<MAX_PANELS){
    add_panel(&room_wall_1, num_panels);
    add_text(&room_wall_1,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");

  if(num_panels<MAX_PANELS){
    add_panel(&room_wall_2, num_panels);
    add_text(&room_wall_2,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");

  if(num_panels<MAX_PANELS){
    add_panel(&room_wall_3, num_panels);
    add_text(&room_wall_3,  num_panels++);
  }
  else log_write("reached MAX_PANELS\n");
}

// ---------------------------------------------------

// I don't know why this dramatic compensation is needed
#define ODD_Y_COMPENSATION 0.70f
#define Y_UP_OFFSET        2.0f

void g2d_init() {
}

void g2d_clear_screen(uint8_t colour) {

  if(!prepared) return;

  log_write("\n\n");

  set_up_scene_begin(&vertices, &glyphs);

  vertex_buffer_end = 0;
  uv_buffer_end = 0;

  num_panels = 0;
  num_glyphs = 0;

  panel p ={
   .dimensions = { 1.0f, 1.0f, 0.03f },
   .position   = { 0, Y_UP_OFFSET, -2.19f },
   .rotation   = { 0.0f, 0.0f, 0.0f },
  };

  add_panel(&p, 0);

  text_ends[0][0]=0;
  text_ends[0][1]=0;

  num_panels++;
}

void g2d_render() {

  if(!prepared) return;

  do_3d_stuff();

  for(unsigned int i = 0; i < num_panels * 6*6; i++) {
    *(vertices+i*5+0) = vertex_buffer_data[i*3+0];
    *(vertices+i*5+1) = vertex_buffer_data[i*3+1];
    *(vertices+i*5+2) = vertex_buffer_data[i*3+2];
    *(vertices+i*5+3) = uv_buffer_data[i*2+0];
    *(vertices+i*5+4) = uv_buffer_data[i*2+1];
  }
  set_up_scene_end();
}

void g2d_internal_rectangle(uint16_t cxtl, uint16_t cytl,
                            uint16_t cxbr, uint16_t cybr,
                            uint16_t colour){

  if(!prepared) return;

  if(num_panels==MAX_PANELS){
    log_write("reached MAX_PANELS\n");
    return;
  }

  float w=  (cxbr-cxtl)/(SCREEN_WIDTH *1.0f);
  float h=( (cybr-cytl)/(SCREEN_HEIGHT*1.0f))*ODD_Y_COMPENSATION;
  float x=(-1.00f+cxtl/(SCREEN_WIDTH /2.0f)+w);
  float y=( 1.00f-cytl/(SCREEN_HEIGHT/2.0f)-h)*ODD_Y_COMPENSATION+Y_UP_OFFSET;

  log_write("g2d_internal_rectangle() %d,%d %d,%d\n", cxtl, cytl, cxbr, cybr);
  log_write("(x=%.2f,y=%.2f)(w=%.2f,h=%.2f)\n", x,y, w,h);

  panel p ={
   .dimensions = { w, h, 0.03f },
   .position   = { x, y, -2.2f },
   .rotation   = { 0.0f, 0.0f, 0.0f },
  };

  add_panel(&p, num_panels);

  text_ends[num_panels][0]=0;
  text_ends[num_panels][1]=0;

  num_panels++;
}

void g2d_internal_text(int16_t ox, int16_t oy,
                       uint16_t cxtl, uint16_t cytl,
                       uint16_t cxbr, uint16_t cybr,
                       char* text, uint16_t colour, uint16_t bg,
                       uint8_t size){

  log_write("g2d_internal_text() %d,%d %d,%d %d,%d '%s'\n",
                                 ox, oy, cxtl, cytl, cxbr, cybr, text);

  float left = -1.00f+ox/(SCREEN_WIDTH /2.0f);
  float top  =(-1.00f+oy/(SCREEN_HEIGHT/2.0f))*ODD_Y_COMPENSATION+0.05f;
  float w = 1.0f;
  float h = 1.0f;
  float scale = (size/2.0f)*(w/17000.0f);

  uint32_t s=num_glyphs;
  add_text_whs(left, top, w, h, scale, text);
  uint32_t e=num_glyphs;

  text_ends[0][0]=0;
  text_ends[0][1]=e;
}

uint16_t g2d_text_width(char* text, uint8_t size){
  log_write("g2d_text_width() '%s'\n", text);
  uint16_t n=strlen(text);
  return n*3*size;
}



