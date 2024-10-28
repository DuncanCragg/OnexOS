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

struct cuboid {
  float position[3];
  float pad1;
  float shape[3];
  float pad2;
};

#define NUM_OBJECTS 2
uint32_t objects_size = (sizeof(uint32_t) * 4) + (sizeof(struct cuboid) * NUM_OBJECTS);

static uint32_t align_uint32(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

void update_objects(){

  uint32_t* size_p = (uint32_t*)objects_data;
  *size_p = NUM_OBJECTS;

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

  set_up_scene_begin();

  if(!draw_by_type(user, "viewing")){

    r = evaluate_user_2d(user, d);
  }

  set_up_scene_end();

  return r;
}

static bool draw_by_type(object* user, char* path) {

  if(object_pathpair_contains(user, path, "is", "3d")) draw_3d(user, path);
  else return false;
  return true;
}

static void draw_3d(object* user, char* path){

  char* px=object_pathpair(user, path, "position:1");
  char* py=object_pathpair(user, path, "position:2");
  char* pz=object_pathpair(user, path, "position:3");

  char* sx=object_pathpair(user, path, "shape:1");
  char* sy=object_pathpair(user, path, "shape:2");
  char* sz=object_pathpair(user, path, "shape:3");

  char* e;

  float pxval=px? strtof(px,&e): 0.0;
  float pyval=py? strtof(py,&e): 0.0;
  float pzval=pz? strtof(pz,&e): 0.0;

  float sxval=sx? strtof(sx,&e): 0.0;
  float syval=sy? strtof(sy,&e): 0.0;
  float szval=sz? strtof(sz,&e): 0.0;

  struct cuboid* obj_array = (struct cuboid*)(objects_data + (sizeof(uint32_t) * 4));

  obj_array[0].position[0] = pxval;
  obj_array[0].position[1] = pyval;
  obj_array[0].position[2] = pzval;
  obj_array[0].shape[0] = sxval;
  obj_array[0].shape[1] = syval;
  obj_array[0].shape[2] = szval;
}

// ---------------------------------

static void show_matrix(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  log_write("\\---------------------/\n");
}

// --------------------------------------------------------------------------------------

