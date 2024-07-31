// -----------------------------------------------------------

#include <linmath-plus.h>

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>

#include "unix/user-onl-vk.h"

#include <onn.h>
#include <onr.h>

mat4x4 proj_matrix;
mat4x4 view_l_matrix;
mat4x4 view_r_matrix;

static const float eye_sep = 0.020; // eyes off-centre dist in m
static const float eye_con = 0.020; // angle of convergence rads

// 1.75m height
// standing back 5m from origin
static vec3  eye_l = { -eye_sep, 1.75, -5.0 };
static vec3  eye_r = {  eye_sep, 1.75, -5.0 };
static vec3  up = { 0.0f, -1.0, 0.0 };

static float eye_dir=0;
static float head_hor_dir=0;
static float head_ver_dir=0;

void set_proj_view() {

    #define VIEWPORT_FOV   43.0f
    #define VIEWPORT_NEAR   0.1f
    #define VIEWPORT_FAR  100.0f

    Mat4x4_perspective(proj_matrix,
                       (float)degreesToRadians(VIEWPORT_FOV),
                       onl_vk_aspect_ratio_proj,
                       VIEWPORT_NEAR, VIEWPORT_FAR);

    proj_matrix[1][1] *= -1;

    vec3 looking_at_l;
    vec3 looking_at_r;

    looking_at_l[0] = eye_l[0] + 100.0f * sin(eye_dir + eye_con + head_hor_dir);
    looking_at_l[1] = eye_l[1] - 100.0f * sin(                    head_ver_dir);
    looking_at_l[2] = eye_l[2] + 100.0f * cos(eye_dir + eye_con + head_hor_dir);

    looking_at_r[0] = eye_r[0] + 100.0f * sin(eye_dir - eye_con + head_hor_dir);
    looking_at_r[1] = eye_r[1] - 100.0f * sin(                    head_ver_dir);
    looking_at_r[2] = eye_r[2] + 100.0f * cos(eye_dir - eye_con + head_hor_dir);

    mat4x4_look_at(view_l_matrix, eye_l, looking_at_l, up);
    mat4x4_look_at(view_r_matrix, eye_r, looking_at_r, up);
}

static bool     head_moving=false;
static bool     body_moving=false;
static uint32_t x_on_press;
static uint32_t y_on_press;

static float dwell(float delta, float width){
  return delta > 0? max(delta - width, 0.0f):
                    min(delta + width, 0.0f);
}

void ont_vk_iostate_changed() {
  /*
  log_write("ont_vk_iostate_changed [%d,%d] @(%d %d) buttons=(%d %d %d) key=%d\n",
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

    eye_l[0] += 4.0f * delta_y * sin(eye_dir);
    eye_l[2] += 4.0f * delta_y * cos(eye_dir);
    eye_r[0] += 4.0f * delta_y * sin(eye_dir);
    eye_r[2] += 4.0f * delta_y * cos(eye_dir);
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

