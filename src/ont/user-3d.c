// -----------------------------------------------------------

#include <math.h>

#include <linmath-plus.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "unix/user-onl-vk.h"
#include <g2d.h>

#include <onn.h>
#include <onr.h>

// ---------------------------------

static void vec3_show(char* name, vec3 v){
  log_write("%s={ %0.4f, %0.4f, %0.4f }\n", name, v[0], v[1], v[2]);
}

static void mat4x4_show(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++){
    log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  }
  log_write("\\---------------------/\n");
}

// ---------------------------------

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

static uint32_t align_uint32(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

// -----------------------------------------

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
static void draw_3d(object* user, char* path);

extern bool evaluate_user_2d(object* usr, void* d);

bool evaluate_user(object* user, void* d) {

  bool r = true;

  if(set_up_scene_begin()){

    if(!draw_by_type(user, "viewing")){

      r = evaluate_user_2d(user, d);
    }

    set_up_scene_end();
  }

  return r;
}

static bool draw_by_type(object* user, char* path) {

  if(object_pathpair_contains(user, path, "is", "3d")) draw_3d(user, path);
  else return false;
  return true;
}

void update_bb(struct scene_object* objects,
               uint32_t oi,
               float position[3],
               float shape[3]              ) {

    if(objects[oi].bb_shape[0] < 0.0){

      for (int i = 0; i < 3; i++) {
        objects[oi].bb_position[i] =  position[i];
        objects[oi].bb_shape[i]    =  shape[i];
      }
      return;
    }

    float bb_min[3] = { objects[oi].bb_position[0] - objects[oi].bb_shape[0],
                        objects[oi].bb_position[1] - objects[oi].bb_shape[1],
                        objects[oi].bb_position[2] - objects[oi].bb_shape[2] };

    float bb_max[3] = { objects[oi].bb_position[0] + objects[oi].bb_shape[0],
                        objects[oi].bb_position[1] + objects[oi].bb_shape[1],
                        objects[oi].bb_position[2] + objects[oi].bb_shape[2] };

    float obj_min[3] = { position[0] - shape[0],
                         position[1] - shape[1],
                         position[2] - shape[2] };

    float obj_max[3] = { position[0] + shape[0],
                         position[1] + shape[1],
                         position[2] + shape[2] };

    for (int i = 0; i < 3; i++) {
        bb_min[i] = fmin(bb_min[i], obj_min[i]);
        bb_max[i] = fmax(bb_max[i], obj_max[i]);
    }

    for (int i = 0; i < 3; i++) {
        objects[oi].bb_position[i] = (bb_min[i] + bb_max[i]) / 2.0f;
        objects[oi].bb_shape[i]    = (bb_max[i] - bb_min[i]) / 2.0f;
    }
}

void object_pathpair_vec3(vec3 dest, object* o, char* p1, char* p2){

  dest[0] = 0.0;
  dest[1] = 0.0;
  dest[2] = 0.0;

  char* v=object_pathpair(o, p1, p2);
  if(!v) return;

  if(*v!='[') return;
  v++;
  char* e;
  float x=strtof(v,&e);
  if(*e!='/') return;
  v=e+1;
  float y=strtof(v,&e);
  if(*e!='/') return;
  v=e+1;
  float z=strtof(v,&e);
  if(*e!=']') return;

  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
}

struct scene_object* objects;

static uint32_t top_object=0;

static void draw_path_3d(object* user, char* path, uint32_t parent_idx, vec3 aggpos){

  vec3 shape; object_pathpair_vec3(shape, user, path, "shape");
  vec3_dup(objects[parent_idx].shape, shape);

  objects[parent_idx].bb_shape[0] = -1.0f;
  update_bb(objects, parent_idx, aggpos, shape);

  uint32_t c;
  uint32_t num_subs = object_pathpair_length(user, path, "position");
  for(c=0; c<num_subs; c++){

    uint32_t child_idx = ++top_object;

    char pos_i[64]; snprintf(pos_i, 64, "position:%d", c+1);
    vec3 pos; object_pathpair_vec3(pos, user, path, pos_i);

    vec3_dup(objects[parent_idx].subs[c].position, pos);
    ;        objects[parent_idx].subs[c].obj_index = child_idx;

    vec3 aggpos2; vec3_dup(aggpos2, aggpos);
    vec3_add_(aggpos2, pos);

    char path2[64]; snprintf(path2, 64, "%s:contains:%d", path, c+1);
    draw_path_3d(user, path2, child_idx, aggpos2);

    update_bb(objects, parent_idx, objects[child_idx].bb_position,
                                   objects[child_idx].bb_shape);
  }
  objects[parent_idx].subs[c].obj_index = 0;
}

static void draw_3d(object* user, char* path){

  objects = (struct scene_object*)objects_data;
  top_object = 0;

  uint32_t parent_idx = 0;
  uint32_t child_idx = ++top_object;

  vec3 origin = { 0.0, 0.0, 0.0 };

  vec3_dup(objects[parent_idx].subs[0].position, origin);
  ;        objects[parent_idx].subs[0].obj_index = child_idx;
  ;        objects[parent_idx].subs[1].obj_index = 0;

  draw_path_3d(user, path, child_idx, origin);
}

// --------------------------------------------------------------------------------------

