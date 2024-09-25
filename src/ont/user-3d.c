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

static const float eye_sep = 0.010; // eyes off-centre dist in m
static const float eye_con = 0.000; // angle of convergence rads

// 1.75m height
// standing back 5m from origin
static vec3  eye_l = { -eye_sep, 1.75, -5.0 };
static vec3  eye_r = {  eye_sep, 1.75, -5.0 };
static vec3  up = { 0.0f, -1.0, 0.0 };

static float body_dir=0;
static float head_hor_dir=0;
static float head_ver_dir=0;

static float xv = 0.0f;
static float zv = 0.0f;

void set_proj_view() {

    eye_l[0] += xv; eye_r[0] += xv;
    eye_l[2] += zv; eye_r[2] += zv;

    #define VIEWPORT_FOV   21.5f
    #define VIEWPORT_NEAR   0.1f
    #define VIEWPORT_FAR  100.0f

    Mat4x4_perspective(proj_matrix,
                       (float)degreesToRadians(VIEWPORT_FOV),
                       onl_vk_aspect_ratio_proj,
                       VIEWPORT_NEAR, VIEWPORT_FAR);

    proj_matrix[1][1] *= -1;

    vec3 looking_at_l;
    vec3 looking_at_r;

    looking_at_l[0] = eye_l[0] + 100.0f * sin(body_dir + eye_con + head_hor_dir);
    looking_at_l[1] = eye_l[1] - 100.0f * sin(                    head_ver_dir);
    looking_at_l[2] = eye_l[2] + 100.0f * cos(body_dir + eye_con + head_hor_dir);

    looking_at_r[0] = eye_r[0] + 100.0f * sin(body_dir - eye_con + head_hor_dir);
    looking_at_r[1] = eye_r[1] - 100.0f * sin(                    head_ver_dir);
    looking_at_r[2] = eye_r[2] + 100.0f * cos(body_dir - eye_con + head_hor_dir);

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

#define LOG_IQ
#ifdef LOG_IO
  log_write("ont_vk_iostate_changed D-pad=(%d %d %d %d) head=(%f %f %f) "
            "joy 1=(%f %f) joy 2=(%f %f) "
            "mouse pos=(%d %d %d) mouse buttons=(%d %d %d) key=%d\n",
                   io.d_pad_left, io.d_pad_right, io.d_pad_up, io.d_pad_down,
                   io.yaw, io.pitch, io.roll,
                   io.joy_1_lr, io.joy_1_ud, io.joy_2_lr, io.joy_2_ud,
                   io.mouse_x, io.mouse_y, io.mouse_scroll,
                   io.mouse_left, io.mouse_middle, io.mouse_right,
                   io.key);
#endif

  float sd = sin(body_dir);
  float cd = cos(body_dir);
  float sp = 0.025f;

  xv = 0.0f; zv = 0.0f;

  if(io.d_pad_up){
    xv +=  sp * sd;
    zv +=  sp * cd;
  }
  if(io.d_pad_down){
    xv += -sp * sd;
    zv += -sp * cd;
  }
  if(io.d_pad_right){
    xv +=  sp * cd;
    zv += -sp * sd;
  }
  if(io.d_pad_left){
    xv += -sp * cd;
    zv +=  sp * sd;
  }

  // ------

  float bd = 3.1415926/200 * io.joy_2_lr;
  body_dir += bd;

  head_hor_dir=io.yaw;
  head_ver_dir=io.pitch;
}

// ---------------------------------

static void show_matrix(mat4x4 m){
  log_write("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) log_write("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  log_write("\\---------------------/\n");
}

// --------------------------------------------------------------------------------------

