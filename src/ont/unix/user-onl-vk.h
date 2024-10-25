// ------- common/shared between user and vk code -------------

#include <pthread.h>
#include <stdbool.h>

#include "outline.h"

#include <onl-vk.h>

extern mat4x4 proj_matrix;
extern mat4x4 view_l_matrix;
extern mat4x4 view_r_matrix;
extern float  left_touch_vec[2];

extern uint32_t objects_size;
extern void*    objects_data;

void create_objects();

void set_proj_view();
void set_up_scene_begin();
void set_up_scene_end();



