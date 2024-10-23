// -----------------------------------------------------------

#include <linmath-plus.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "unix/user-onl-vk.h"
#include <g2d.h>

#include <onn.h>
#include <onr.h>

mat4x4 proj_matrix;
mat4x4 view_l_matrix;
mat4x4 view_r_matrix;
float  left_touch_vec[] = { 0.25, 0.75 };

// 1.75m height
// standing back 5m from origin
static vec3  body_pos = { 1.667, 1.75, -10.0 }; // 1.75 is still nose bridge pos
static float body_dir = 0.0;
static float body_xv = 0.0f;
static float body_zv = 0.0f;

static float head_hor_dir=0;
static float head_ver_dir=0;

static const float eye_sep = 0.010; // eyes off-centre dist in m
static const float eye_con = 0.000; // angle of convergence rads

static vec3  up = { 0.0f, -1.0, 0.0 };

// ---------------------------------

static float*            vertices;
static fd_GlyphInstance* glyphs;

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

struct cuboid {
  float position[3];
  float pad1;
  float shape[3];
  float pad2;
};

uint32_t objects_size;
void*    objects_data;

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

float    vertex_buffer_data[MAX_PANELS*6*6*3];
uint32_t vertex_buffer_end=0;

float    uv_buffer_data[MAX_PANELS*6*6*2];
uint32_t uv_buffer_end=0;

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

void load_font(char* font_face) {

  FT_Library library;
  FT_CHECK(FT_Init_FreeType(&library));

  FT_Face face;
  int err=FT_New_Face(library, font_face, 0, &face);
  if(err){
    log_write("Font loading failed or font not found: %s\n", font_face);
    onl_vk_quit();
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
  // log_write("\n");

  glyph_info_size = sizeof(fd_DeviceGlyphInfo) * num_glyph_chars;
  glyph_cells_size = sizeof(uint32_t) * total_cells;
  glyph_points_size = sizeof(vec2) * total_points;

  uint32_t alignment = onl_vk_min_storage_buffer_offset_alignment;

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

void create_objects(){

  uint32_t num_objects = 2;

  objects_size = (sizeof(uint32_t) * 4) + (sizeof(struct cuboid) * num_objects);
  objects_data = malloc(objects_size);

  uint32_t* size_p = (uint32_t*)objects_data;
  *size_p = num_objects;

  struct cuboid* obj_array = (struct cuboid*)(objects_data + (sizeof(uint32_t) * 4));

  obj_array[0].position[0] =  3.0f;
  obj_array[0].position[1] =  0.2f;
  obj_array[0].position[2] = -2.0f;
  obj_array[0].shape[0] = 0.5f;
  obj_array[0].shape[1] = 0.2f;
  obj_array[0].shape[2] = 0.01f;

  obj_array[1].position[0] =  7.0f;
  obj_array[1].position[1] =  1.0f;
  obj_array[1].position[2] = -1.0f;
  obj_array[1].shape[0] = 2.0f;
  obj_array[1].shape[1] = 1.0f;
  obj_array[1].shape[2] = 0.01f;
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

void add_panel(panel* panel, int p){

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

void add_text_whs(float left, float top,
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
            // still fits on line, keep going
        }
        else
        if(inst->rect.min_y > top) {
            // need to wrap, but check top of glyph is ...?

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

void do_3d_stuff() {

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

bool scene_begin() {

  set_up_scene_begin(&vertices, &glyphs);

  vertex_buffer_end = 0;
  uv_buffer_end = 0;

  num_panels = 0;
  num_glyphs = 0;

  return true;
}


void scene_end() {

  for(unsigned int i = 0; i < num_panels * 6*6; i++) {
    *(vertices+i*5+0) = vertex_buffer_data[i*3+0];
    *(vertices+i*5+1) = vertex_buffer_data[i*3+1];
    *(vertices+i*5+2) = vertex_buffer_data[i*3+2];
    *(vertices+i*5+3) = uv_buffer_data[i*2+0];
    *(vertices+i*5+4) = uv_buffer_data[i*2+1];
  }
  set_up_scene_end();
}


// ---------------

void set_proj_view() {

    body_pos[0] += body_xv;
    body_pos[2] += body_zv;

    #define VIEWPORT_FOV   21.5f
    #define VIEWPORT_NEAR   0.1f
    #define VIEWPORT_FAR  100.0f

    Mat4x4_perspective(proj_matrix,
                       (float)degreesToRadians(VIEWPORT_FOV),
                       onl_vk_aspect_ratio,
                       VIEWPORT_NEAR, VIEWPORT_FAR);

    proj_matrix[1][1] *= -1;

    vec3 looking_at_l;
    vec3 looking_at_r;
    vec3 eye_l;
    vec3 eye_r;

    eye_l[0]=body_pos[0]-eye_sep;
    eye_l[1]=body_pos[1]        ;
    eye_l[2]=body_pos[2]        ;

    eye_r[0]=body_pos[0]+eye_sep;
    eye_r[1]=body_pos[1]        ;
    eye_r[2]=body_pos[2]        ;

    looking_at_l[0] = eye_l[0] + 100.0f * sin(body_dir + head_hor_dir + eye_con);
    looking_at_l[1] = eye_l[1] - 100.0f * sin(           head_ver_dir          );
    looking_at_l[2] = eye_l[2] + 100.0f * cos(body_dir + head_hor_dir + eye_con);

    looking_at_r[0] = eye_r[0] + 100.0f * sin(body_dir + head_hor_dir - eye_con);
    looking_at_r[1] = eye_r[1] - 100.0f * sin(           head_ver_dir          );
    looking_at_r[2] = eye_r[2] + 100.0f * cos(body_dir + head_hor_dir - eye_con);

    mat4x4_look_at(view_l_matrix, eye_l, looking_at_l, up);
    mat4x4_look_at(view_r_matrix, eye_r, looking_at_r, up);
}


static float dwell(float delta, float width){
  return delta > 0? max(delta - width, 0.0f):
                    min(delta + width, 0.0f);
}

static bool head_moving=false;
static bool body_moving=false;

void ont_vk_iostate_changed() {

#define LOG_IO
#ifdef LOG_IO
  log_write("D-pad=(%d %d %d %d) head=(%f %f %f) "
            "joy 1=(%f %f) joy 2=(%f %f) touch=(%d %d) "
            "mouse pos=(%d %d %d) mouse buttons=(%d %d %d) key=%d\n",
                   io.d_pad_left, io.d_pad_right, io.d_pad_up, io.d_pad_down,
                   io.yaw, io.pitch, io.roll,
                   io.joy_1_lr, io.joy_1_ud, io.joy_2_lr, io.joy_2_ud,
                   io.touch_x, io.touch_y,
                   io.mouse_x, io.mouse_y, io.mouse_scroll,
                   io.mouse_left, io.mouse_middle, io.mouse_right,
                   io.key);
#endif

  float sd = sin(body_dir);
  float cd = cos(body_dir);
  float sp = 0.04f;

  body_xv = 0.0f; body_zv = 0.0f;

  if(io.d_pad_up){
    body_xv +=  sp * sd * 2.0f;
    body_zv +=  sp * cd * 2.0f;
  }
  if(io.d_pad_down){
    body_xv += -sp * sd * 2.0f;
    body_zv += -sp * cd * 2.0f;
  }
  if(io.d_pad_right){
    body_xv +=  sp * cd;
    body_zv += -sp * sd;
  }
  if(io.d_pad_left){
    body_xv += -sp * cd;
    body_zv +=  sp * sd;
  }

  float bd = 3.1415926/200 * io.joy_2_lr;
  body_dir += bd;

  bool bottom_left = io.touch_x < onl_vk_width / 3 && io.touch_y > onl_vk_height / 2;

  left_touch_vec[0]=0.25;
  left_touch_vec[1]=0.75;

  if(io.touch_x==0 && io.touch_y==0) {
    body_moving=false;
    head_moving=false;
    body_dir += head_hor_dir;
    head_hor_dir = 0;
  }
  else
  if(!head_moving && (bottom_left || body_moving)){

    left_touch_vec[0] = (float)io.touch_x / (float)onl_vk_height; // 1.0 unit normalised
    left_touch_vec[1] = (float)io.touch_y / (float)onl_vk_height;

    if(!body_moving){
      body_moving = true;
    }
    float x =  (float)io.touch_x                    / (onl_vk_width  / 3);
    float y = ((float)io.touch_y-(onl_vk_height/2)) / (onl_vk_height / 2);

    if(x>0.333 && x< 0.666 && y < 0.333){ // forwards
      body_xv +=  sp * sd * 2.0f;
      body_zv +=  sp * cd * 2.0f;
    }
    if(x>0.333 && x< 0.666 && y > 0.666){ // backwards
      body_xv += -sp * sd * 2.0f;
      body_zv += -sp * cd * 2.0f;
    }
    if(x>0.666 && y > 0.333 && y < 0.666){ // right
      body_xv +=  sp * cd;
      body_zv += -sp * sd;
    }
    if(x<0.333 && y > 0.333 && y < 0.666){ // left
      body_xv += -sp * cd;
      body_zv +=  sp * sd;
    }
  }
  else {

    static uint32_t x_on_touch;
    static uint32_t y_on_touch;
    static float    hhd_on_touch;
    static float    hvd_on_touch;

    if(!head_moving){
      head_moving=true;
      x_on_touch=io.touch_x;
      y_on_touch=io.touch_y;
      hhd_on_touch=head_hor_dir;
      hvd_on_touch=head_ver_dir;
    }
    else{
      float mx=(float)((int16_t)io.touch_x-(int16_t)x_on_touch);
      float my=(float)((int16_t)io.touch_y-(int16_t)y_on_touch);
      float dx=mx/onl_vk_width *3.1415926;
      float dy=my/onl_vk_height*3.1415926;
      printf("mx=%f my=%f dx=%f dy=%f\n", mx, my, dx, dy);
      head_hor_dir=hhd_on_touch+dx;
      head_ver_dir=hvd_on_touch+dy;
    }
  }

  head_hor_dir+=io.yaw;
  head_ver_dir+=io.pitch;
}

// ---------------------------------

static bool draw_by_type(object* user, char* path);
static void draw_3d(char* path);

extern bool evaluate_user_2d(object* usr, void* d);

bool evaluate_user(object* user, void* d) {

  bool r = true;

  scene_begin();

  if(!draw_by_type(user, "viewing")){

    r = evaluate_user_2d(user, d);
  }

  scene_end();

  return r;
}

static bool draw_by_type(object* user, char* path) {

  if(object_pathpair_contains(user, path, "is", "3d")) draw_3d(path);
  else return false;
  return true;
}

static void draw_3d(char* path){

  do_3d_stuff();
}

// ---------------------------------

static void show_matrix(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  log_write("\\---------------------/\n");
}

// --------------------------------------------------------------------------------------

