// ------- common/shared between user and vk code -------------

#include <pthread.h>
#include <stdbool.h>

#include "outline.h"

#include <onl-vk.h>

extern mat4x4 proj_matrix;
extern mat4x4 view_l_matrix;
extern mat4x4 view_r_matrix;
extern float  left_touch_vec[2];

struct scene_object {
  float shape[3];    float pad2;
  float position[3]; float pad1;
};

#define OBJECTS_MAX_NUM 1024
#define OBJECTS_SIZE ((sizeof(uint32_t) * 4) + (sizeof(struct scene_object) * OBJECTS_MAX_NUM))

extern void* objects_data;

void set_proj_view();
bool set_up_scene_begin();
void set_up_scene_end();



