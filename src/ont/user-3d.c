// -----------------------------------------------------------

#include <linmath-plus.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include <onl.h>
#include <onn.h>
#include <onr.h>

#include "unix/user-onx-vk.h"

mat4x4 proj_matrix;
mat4x4 view_matrix;

// 1.75m height
// standing back 4m from origin
static vec3  eye = { 0.0, 1.75, -4.0 };
static vec3  up = { 0.0f, -1.0, 0.0 };

static float eye_dir=0;
static float head_hor_dir=0;
static float head_ver_dir=0;

void set_mvp_uniforms() {

    #define VIEWPORT_FOV   70.0f
    #define VIEWPORT_NEAR   0.1f
    #define VIEWPORT_FAR  100.0f

    float swap_aspect_ratio = 1.0f * io.swap_width / io.swap_height;

    Mat4x4_perspective(proj_matrix, (float)degreesToRadians(VIEWPORT_FOV), swap_aspect_ratio, VIEWPORT_NEAR, VIEWPORT_FAR);
    proj_matrix[1][1] *= -1;

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
  log_write("onx_iostate_changed [%d,%d] @(%d %d) buttons=(%d %d %d) key=%d\n",
           io.swap_width, io.swap_height,
           io.mouse_x, io.mouse_y,
           io.left_pressed, io.middle_pressed, io.right_pressed,
           io.key);
  */
  bool bottom_left = io.mouse_x < io.swap_width / 3 && io.mouse_y > io.swap_height / 2;

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

// ---------------------------------

static void show_matrix(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  log_write("\\---------------------/\n");
}

// --------------------------------------------------------------------------------------

