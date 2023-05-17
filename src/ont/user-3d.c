// -----------------------------------------------------------

#include "ont/unix/user-onx-vk.h"
#include "ont/unix/linmath-plus.h"

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onn.h>
#include <onr.h>

uint32_t num_panels=MAX_PANELS;
uint32_t num_glyphs;

mat4x4 proj_matrix;
mat4x4 view_matrix;
mat4x4 model_matrix[MAX_PANELS];
vec4   text_ends[MAX_PANELS];

void*    glyph_data;
uint32_t glyph_data_size;
uint32_t glyph_info_size;
uint32_t glyph_cells_offset;
uint32_t glyph_cells_size;
uint32_t glyph_points_offset;
uint32_t glyph_points_size;

// ---------------------------------

static vec3 up = { 0.0f, -1.0, 0.0 };

// 1.75m height
// standing back 4m from origin
static vec3  eye = { 0.0, 1.75, -4.0 };
static float eye_dir=0;
static float head_hor_dir=0;
static float head_ver_dir=0;

// ---------------------------------

#define NUMBER_OF_GLYPHS 96
uint32_t num_glyph_chars=NUMBER_OF_GLYPHS;

typedef struct panel {

  vec3  dimensions;
  vec3  position;
  vec3  rotation;
  char* text;

} panel;

panel welcome_banner ={
 .dimensions = { 2.0f, 0.8f, 0.03f },
 .position   = { 1.0f, 2.0f, -2.0f },
 .rotation   = { 0.0f, 0.0f, 0.0f },
 .text = "Hello, and welcome to OnexOS and the Object Network! This is your place to hang out with friends and family and chill without surveillance or censorship. Big Tech has had their way for decades, now it's time to bring the power of our technology back to ourselves! OnexOS is a freeing and empowering operating system. We hope you enjoy using it! Hello, and welcome to OnexOS and the Object Network! This is your place to hang out with friends and family and chill without surveillance or censorship. Big Tech has had their way for decades, now it's time to bring the power of our technology back to ourselves! OnexOS is a freeing and empowering operating system. We hope you enjoy using it! ",
};

panel document ={
 .dimensions = {  0.5f, 0.7f,  0.01f },
 .position   = {  0.0f, 1.5f, -14.0f },
 .rotation   = {  0.0f, 180.0f, 0.0f },
 .text = "There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. There is plenty to do here and this document is here to get you started! First thing is to play with the movement controls. ",
};

panel info_board ={
 .dimensions = { 8.0f,   4.0f,  0.03f },
 .position   = { 5.0f,   2.0f, -1.0f },
 .rotation   = { 0.0f, -45.0f,  0.0f },
 .text = "OnexOS is an OS with no apps! Instead of apps, there are just things! Your stuff, and you yourselves, exist in this shared space. OnexOS is an OS with no apps! Instead of apps, there are just things! Your stuff, and you yourselves, exist in this shared space. OnexOS is an OS with no apps! Instead of apps, there are just things! Your stuff, and you yourselves, exist in this shared space. ",
};

panel room_floor ={
 .dimensions = {  6.0f, 6.0f,   0.2f },
 .position   = {  0.0f, 0.01f, -14.0f },
 .rotation   = { 90.0f, 0.0f,   0.0f },
 .text = "v",
};

panel room_ceiling ={
 .dimensions = {   6.0f, 6.0f,  0.2f },
 .position   = {   0.0f, 6.0f, -14.0f },
 .rotation   = { -90.0f, 0.0f,  0.0f },
 .text = "^",
};

panel room_wall_1 ={
 .dimensions = {  6.0f,  6.0f,  0.2f },
 .position   = { -3.0f,  3.0f, -14.0f },
 .rotation   = {  0.0f, 90.0f,  0.0f },
 .text = "wall 1",
};

panel room_wall_2 ={
 .dimensions = { 6.0f,   6.0f,  0.2f },
 .position   = { 3.0f,   3.0f, -14.0f },
 .rotation   = { 0.0f, -90.0f,  0.0f },
 .text = "wall 2",
};

panel room_wall_3 ={
 .dimensions = { 6.0f,   6.0f,  0.2f },
 .position   = { 0.0f,   3.0f, -17.0f },
 .rotation   = { 0.0f, 180.0f,  0.0f },
 .text = "wall 3",
};

// -----------------------------------------

extern object* user;

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

void load_font(const char * font_face, uint32_t alignment) {

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

  float w=dimensions[0]/2;
  float h=dimensions[1]/2;
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

static void add_panel(panel* panel, int o){

    make_box(panel->dimensions);

    mat4x4_translation(model_matrix[o], panel->position[0],
                                        panel->position[1],
                                        panel->position[2]);
    mat4x4 mm;
    mat4x4_rotate_X(mm, model_matrix[o], (float)degreesToRadians(panel->rotation[0]));
    mat4x4_rotate_Y(model_matrix[o], mm, (float)degreesToRadians(panel->rotation[1]));
    mat4x4_rotate_Z(mm, model_matrix[o], (float)degreesToRadians(panel->rotation[2]));
    mat4x4_orthonormalize(model_matrix[o], mm);
}

static void add_text(panel* panel, int o, fd_GlyphInstance* glyphs) {

    float w = panel->dimensions[0];
    float h = panel->dimensions[1];

    float left = -w/2.17f;
    float top  = -h/2.63f;

    float x = left;
    float y = top;

    float scale = w/30000.0f;

    const char *text = panel->text;

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

        if (inst->rect.max_x < w/2.2f) {
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
    text_ends[o][0]=num_glyphs-1;
}

bool evaluate_user(object* o, void* d) {

  // model changes, vertex changes, text changes

  char* ts=object_property(user, "viewing:io:ts");
  if(ts) welcome_banner.text=ts;

  info_board.rotation[1]+=4.0f;

  // -----------------------------------

  float* vertices;
  fd_GlyphInstance* glyphs;

  if(set_up_scene_begin(&vertices, &glyphs)) {

    vertex_buffer_end=0;
    uv_buffer_end=0;
    num_glyphs = 0;

    add_panel(&welcome_banner, 0);
    add_panel(&document,       1);
    add_panel(&info_board,     2);
    add_panel(&room_floor,     3);
    add_panel(&room_ceiling,   4);
    add_panel(&room_wall_1,    5);
    add_panel(&room_wall_2,    6);
    add_panel(&room_wall_3,    7);

    for (unsigned int i = 0; i < num_panels * 6*6; i++) {
        *(vertices+i*5+0) = vertex_buffer_data[i*3+0];
        *(vertices+i*5+1) = vertex_buffer_data[i*3+1];
        *(vertices+i*5+2) = vertex_buffer_data[i*3+2];
        *(vertices+i*5+3) = uv_buffer_data[i*2+0];
        *(vertices+i*5+4) = uv_buffer_data[i*2+1];
    }

    add_text(&welcome_banner, 0, glyphs);
    add_text(&document,       1, glyphs);
    add_text(&info_board,     2, glyphs);
    add_text(&room_floor,     3, glyphs);
    add_text(&room_ceiling,   4, glyphs);
    add_text(&room_wall_1,    5, glyphs);
    add_text(&room_wall_2,    6, glyphs);
    add_text(&room_wall_3,    7, glyphs);

    set_up_scene_end();
  }

  return true;
}

// ---------------------------------

static void show_matrix(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  log_write("\\---------------------/\n");
}

// ---------------------------------

void set_mvp_uniforms() {

    #define VIEWPORT_FOV   70.0f
    #define VIEWPORT_NEAR   0.1f
    #define VIEWPORT_FAR  100.0f

    float swap_aspect_ratio = 1.0f * io.swap_width / io.swap_height;

    Mat4x4_perspective(proj_matrix, (float)degreesToRadians(VIEWPORT_FOV), swap_aspect_ratio, VIEWPORT_NEAR, VIEWPORT_FAR);
    proj_matrix[1][1] *= -1;
    if(io.rotation_angle){
      mat4x4 pm;
      mat4x4_dup(pm, proj_matrix);
      mat4x4_rotate_Z(proj_matrix, pm, (float)degreesToRadians(-io.rotation_angle));
    }

    vec3 looking_at;

    looking_at[0] = eye[0] + 100.0f * sin(eye_dir + head_hor_dir);
    looking_at[1] = eye[1] - 100.0f * sin(          head_ver_dir);
    looking_at[2] = eye[2] + 100.0f * cos(eye_dir + head_hor_dir);

    mat4x4_look_at(view_matrix, eye, looking_at, up);
}

static bool     head_moving=false;
static bool     body_moving=false;
static uint32_t x_on_press;
static uint32_t y_on_press;

static float dwell(float delta, float width){
  return delta > 0? max(delta - width, 0.0f):
                    min(delta + width, 0.0f);
}

void onx_iostate_changed() {
  /*
  log_write("onx_iostate_changed %d' [%d,%d][%d,%d] @(%d %d) buttons=(%d %d %d) key=%d\n",
           io.rotation_angle,
           io.view_width, io.view_height, io.swap_width, io.swap_height,
           io.mouse_x, io.mouse_y,
           io.left_pressed, io.middle_pressed, io.right_pressed,
           io.key);
  */
  bool bottom_left = io.mouse_x < io.view_width / 3 && io.mouse_y > io.view_height / 2;

  if(io.left_pressed && !body_moving && bottom_left){
    body_moving=true;

    x_on_press = io.mouse_x;
    y_on_press = io.mouse_y;
  }
  else
  if(io.left_pressed && body_moving){

    float delta_x =  0.00007f * ((int32_t)io.mouse_x - (int32_t)x_on_press);
    float delta_y = -0.00007f * ((int32_t)io.mouse_y - (int32_t)y_on_press);

    delta_x = dwell(delta_x, 0.0015f);
    delta_y = dwell(delta_y, 0.0015f);

    eye_dir += 0.5f* delta_x;

    eye[0] += 4.0f * delta_y * sin(eye_dir);
    eye[2] += 4.0f * delta_y * cos(eye_dir);
  }
  else
  if(!io.left_pressed && body_moving){
    body_moving=false;
  }
  else
  if(io.left_pressed && !head_moving){

    head_moving=true;

    x_on_press = io.mouse_x;
    y_on_press = io.mouse_y;
  }
  else
  if(io.left_pressed && head_moving){

    float delta_x = 0.00007f * ((int32_t)io.mouse_x - (int32_t)x_on_press);
    float delta_y = 0.00007f * ((int32_t)io.mouse_y - (int32_t)y_on_press);

    head_hor_dir = 35.0f*dwell(delta_x, 0.0015f);
    head_ver_dir = 35.0f*dwell(delta_y, 0.0015f);
  }
  else
  if(!io.left_pressed && head_moving){

    head_moving=false;

    head_hor_dir=0;
    head_ver_dir=0;
  }
}

// ------------------------------------------------------------------------------------------------------------------------------------

