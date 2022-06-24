
#include <pthread.h>

#include "ont/linmath-plus.h"
#include "ont/outline.h"

#include "ont/vulkan/vulkan.h"

#include <onex-kernel/time.h>
#include <onex-kernel/log.h>
#include <onn.h>
#include <onr.h>

// ---------------------------------

#define MAX_PANELS 8

vec3 up = { 0.0f, -1.0, 0.0 };

// 1.75m height
// standing back 4m from origin
vec3  eye = { 0.0, 1.75, -4.0 };
float eye_dir=0;
float head_hor_dir=0;
float head_ver_dir=0;

mat4x4 proj_matrix;
mat4x4 view_matrix;
mat4x4 model_matrix[MAX_PANELS];
vec4   text_ends[MAX_PANELS];

struct uniforms {
    float proj[4][4];
    float view[4][4];
    float model[MAX_PANELS][4][4];
    vec4  text_ends[MAX_PANELS];
};

struct push_constants {
  uint32_t phase;
};

// -----------------------------------------

object* config;
object* user;
object* oclock;

char* userUID=0;
char* clockUID=0;

bool evaluate_default(object* o, void* d) {

  log_write("evaluate_default data=%p\n", d); object_log(o);
  return true;
}

static bool evaluate_user(object* o, void* d);

static void set_up_scene();

static void every_second(){ onex_run_evaluators(clockUID, 0); }

static void init_onex() {

  onex_set_evaluators((char*)"default", evaluate_object_setter, evaluate_default, 0);
  onex_set_evaluators((char*)"device",                          evaluate_device_logic, 0);
  onex_set_evaluators((char*)"user",                            evaluate_user, 0);
  onex_set_evaluators((char*)"clock",   evaluate_object_setter, evaluate_clock, 0);

  onex_init((char*)"./onex.ondb");

  config=onex_get_from_cache((char*)"uid-0");

  if(!config){

    user=object_new(0, (char*)"user", (char*)"user", 8);
    userUID=object_property(user, (char*)"UID");

    oclock=object_new(0, (char*)"clock", (char*)"clock event", 12);
    object_property_set(oclock, (char*)"title", (char*)"OnexOS Clock");
    clockUID=object_property(oclock, (char*)"UID");

    object_set_evaluator(onex_device_object, (char*)"device");
    char* deviceUID=object_property(onex_device_object, (char*)"UID");

    object_property_add(onex_device_object, (char*)"user", userUID);
    object_property_add(onex_device_object, (char*)"io", clockUID);

    object_property_set(user, (char*)"viewing", deviceUID);

    config=object_new((char*)"uid-0", 0, (char*)"config", 10);
    object_property_set(config, (char*)"user",      userUID);
    object_property_set(config, (char*)"clock",     clockUID);
  }
  else{
    userUID=     object_property(config, (char*)"user");
    clockUID=    object_property(config, (char*)"clock");

    user     =onex_get_from_cache(userUID);
    oclock   =onex_get_from_cache(clockUID);
  }

  time_ticker(every_second, 1000);
}

static pthread_t loop_onex_thread_id;

static void* loop_onex_thread(void* d) {
  while(true){
    if(!onex_loop()){
      time_delay_ms(5);
    }
  }
  return 0;
}

void onx_init(bool restart){
  if(!restart){
    init_onex();
    pthread_create(&loop_onex_thread_id, 0, loop_onex_thread, 0);
  }
  onex_run_evaluators(userUID, 0);
}

// ---------------------------------

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

static bool evaluate_user(object* o, void* d) {

  if(prepared){

    printf("evaluate_user\n");

    // model changes, vertex changes, text changes
    /*
    {
     is: user
     viewing: {
       is: device
       user: uid-user
       io: {
         is: clock event
         title: OnexOS Clock
         ts: 1656097056
         tz: BST 3600
       }
     }
    }
    */
    char* ts=object_property(user, (char*)"viewing:io:ts");
    if(ts) welcome_banner.text=ts;

    info_board.rotation[1]+=4.0f;

    set_up_scene();
  }
  return true;
}

// ---------------------------------

static float    vertex_buffer_data[MAX_PANELS*6*6*3];
static uint32_t vertex_buffer_end=0;

static float    uv_buffer_data[MAX_PANELS*6*6*2];
static uint32_t uv_buffer_end=0;

VkFormat texture_format = VK_FORMAT_R8G8B8A8_UNORM;

struct texture_object {
    int32_t texture_width;
    int32_t texture_height;
    VkSampler sampler;
    VkBuffer buffer;
    VkImageLayout image_layout;
    VkDeviceMemory device_memory;
    VkImage image;
    VkImageView image_view;
};

struct {
    VkFormat format;
    VkDeviceMemory device_memory;
    VkImage image;
    VkImageView image_view;
} depth;

static char *texture_files[] = {"ivory.ppm"};

#include "ont/ivory.ppm.h"

#define TEXTURE_COUNT 1

struct texture_object textures[TEXTURE_COUNT];
// struct texture_object staging_texture;

bool use_staging_buffer = false;

// --------------------------------------

#define NUMBER_OF_GLYPHS 96

#define MAX_VISIBLE_GLYPHS 4096

uint32_t align_uint32(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

typedef struct fd_CellInfo {
    uint32_t point_offset;
    uint32_t cell_offset;
    uint32_t cell_count_x;
    uint32_t cell_count_y;
} fd_CellInfo;

typedef struct fd_GlyphInstance {
  fd_Rect  rect;
  uint32_t glyph_index;
  float    sharpness;
} fd_GlyphInstance;

typedef struct fd_HostGlyphInfo {
    fd_Rect bbox;
    float advance;
} fd_HostGlyphInfo;

typedef struct fd_DeviceGlyphInfo {
    fd_Rect bbox;
    //fd_Rect cbox;
    fd_CellInfo cell_info;
} fd_DeviceGlyphInfo;

fd_Outline       outlines[NUMBER_OF_GLYPHS];
fd_HostGlyphInfo glyph_infos[NUMBER_OF_GLYPHS];

uint32_t glyph_instance_count;

void*    glyph_data;
uint32_t glyph_data_size;

uint32_t glyph_info_offset;
uint32_t glyph_info_size;
uint32_t glyph_cells_offset;
uint32_t glyph_cells_size;
uint32_t glyph_points_offset;
uint32_t glyph_points_size;

// ---------------------------------

typedef struct {
    VkBuffer        uniform_buffer;
    VkDeviceMemory  uniform_memory;
    void*           uniform_memory_ptr;
    VkDescriptorSet descriptor_set;
} uniform_mem_t;

uniform_mem_t *uniform_mem;

typedef struct {
    VkFramebuffer   framebuffer;
    VkImageView     image_view;
    VkCommandBuffer command_buffer;
    VkFence         command_buffer_fence;
} SwapchainImageResources;

uint32_t image_count;
uint32_t image_index;

SwapchainImageResources *swapchain_image_resources;

VkRenderPass render_pass;

VkSemaphore image_acquired_semaphore;
VkSemaphore render_complete_semaphore;

VkPipeline       pipeline;
VkPipelineLayout pipeline_layout;
VkPipelineCache  pipeline_cache;

VkDescriptorSetLayout descriptor_layout;
VkDescriptorPool      descriptor_pool;

VkBuffer vertex_buffer;
VkBuffer staging_buffer;
VkBuffer storage_buffer;
VkBuffer instance_buffer;
VkBuffer instance_staging_buffer;

VkDeviceMemory vertex_buffer_memory;
VkDeviceMemory staging_buffer_memory;
VkDeviceMemory storage_buffer_memory;
VkDeviceMemory instance_buffer_memory;
VkDeviceMemory instance_staging_buffer_memory;

VkShaderModule  vert_shader_module;
VkShaderModule  frag_shader_module;

VkFormatProperties               format_properties;
VkPhysicalDeviceMemoryProperties memory_properties;

// ---------------------------------

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

        if (glyph_instance_count >= MAX_VISIBLE_GLYPHS) break;

        uint32_t glyph_index = *text - 32;

        fd_HostGlyphInfo *gi   = &glyph_infos[glyph_index];
        fd_GlyphInstance *inst = &glyphs[glyph_instance_count];

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

        glyph_instance_count++;
        text++;
        x += gi->advance * scale;
    }
    text_ends[o][0]=glyph_instance_count-1;
}

static void do_render_pass() {

  vkWaitForFences(device, 1, &swapchain_image_resources[image_index].command_buffer_fence, VK_TRUE, UINT64_MAX);

  VkCommandBuffer cmd_buf = swapchain_image_resources[image_index].command_buffer;

  const VkCommandBufferBeginInfo command_buffer_bi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    .pInheritanceInfo = NULL,
    .pNext = 0,
  };

  VK_CHECK(vkBeginCommandBuffer(cmd_buf, &command_buffer_bi));

  // --------------------------------------------

  uint32_t size = MAX_VISIBLE_GLYPHS * sizeof(fd_GlyphInstance);
  uint32_t offset = 0;

  VkBufferMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .buffer = instance_buffer,
      .offset = 0,
      .size = size,
  };

  vkCmdPipelineBarrier(
      cmd_buf,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, NULL,
      1, &barrier,
      0, NULL);

  VkBufferCopy copy = {
      .srcOffset = offset,
      .dstOffset = 0,
      .size = size,
  };

  vkCmdCopyBuffer(cmd_buf, instance_staging_buffer, instance_buffer, 1, &copy);

  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

  vkCmdPipelineBarrier(
      cmd_buf,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      0,
      0, NULL,
      1, &barrier,
      0, NULL);

  // --------------------------------------------

  const VkClearValue clear_values[] = {
    { .color.float32 = { 0.2f, 0.8f, 1.0f, 0.0f } },
    { .depthStencil = { 1.0f, 0 }},
  };

  const VkRenderPassBeginInfo render_pass_bi = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = render_pass,
      .framebuffer = swapchain_image_resources[image_index].framebuffer,
      .renderArea.offset = { 0, 0 },
      .renderArea.extent = swapchain_extent,
      .clearValueCount = 2,
      .pClearValues = clear_values,
      .pNext = 0,
  };

  vkCmdBeginRenderPass(cmd_buf, &render_pass_bi, VK_SUBPASS_CONTENTS_INLINE);

  // --------------------------------------------

  VkBuffer vertex_buffers[] = {
    vertex_buffer,
    instance_buffer,
  };
  VkDeviceSize offsets[] = { 0, 0 };
  vkCmdBindVertexBuffers(cmd_buf, 0, 2, vertex_buffers, offsets);

  vkCmdBindDescriptorSets(cmd_buf,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout,
                          0, 1,
                          &uniform_mem[image_index].descriptor_set,
                          0, NULL);

  vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  // --------------------------------------------

  struct push_constants pc;

  pc.phase = 0, // ground plane
  vkCmdPushConstants(cmd_buf, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct push_constants), &pc);
  vkCmdDraw(cmd_buf, 6, 1, 0, 0);

  pc.phase = 1, // panels
  vkCmdPushConstants(cmd_buf, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct push_constants), &pc);
  for(int o=0; o<MAX_PANELS; o++){
    vkCmdDraw(cmd_buf, 6*6, 1, o*6*6, o);
  }

  pc.phase = 2, // text
  vkCmdPushConstants(cmd_buf, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct push_constants), &pc);
  vkCmdDraw(cmd_buf, 6, glyph_instance_count, 0, 0);

  vkCmdEndRenderPass(cmd_buf);

  VK_CHECK(vkEndCommandBuffer(cmd_buf));
}

// ---------------------------------

static void show_matrix(mat4x4 m){
  printf("/---------------------\\\n");
  for(uint32_t i=0; i<4; i++) printf("%0.4f, %0.4f, %0.4f, %0.4f\n", m[i][0], m[i][1], m[i][2], m[i][3]);
  printf("\\---------------------/\n");
}

static void set_mvp_uniforms() {

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

    memcpy(uniform_mem[image_index].uniform_memory_ptr,
           (const void*)&proj_matrix,  sizeof(proj_matrix));

    memcpy(uniform_mem[image_index].uniform_memory_ptr+sizeof(proj_matrix),
           (const void*)&view_matrix,  sizeof(view_matrix));

    memcpy(uniform_mem[image_index].uniform_memory_ptr+sizeof(proj_matrix)+sizeof(view_matrix),
           (const void*)&model_matrix, sizeof(model_matrix));

    memcpy(uniform_mem[image_index].uniform_memory_ptr+sizeof(proj_matrix)+sizeof(view_matrix)+sizeof(model_matrix),
           (const void*)&text_ends, sizeof(text_ends));
}

// ---------------------------------

static bool            scene_ready = false;
static pthread_mutex_t scene_lock;

static void set_up_scene() {

  pthread_mutex_lock(&scene_lock);
  scene_ready = false;

  // -------------------------------------------------

  vertex_buffer_end=0;
  uv_buffer_end=0;

  size_t vertex_size = MAX_PANELS * 6*6 * (3 * sizeof(vertex_buffer_data[0]) +
                                           2 * sizeof(uv_buffer_data[0])      );
  float* vertices;

  VK_CHECK(vkMapMemory(device, vertex_buffer_memory, 0, vertex_size, 0, &vertices));

  add_panel(&welcome_banner, 0);
  add_panel(&document,       1);
  add_panel(&info_board,     2);
  add_panel(&room_floor,     3);
  add_panel(&room_ceiling,   4);
  add_panel(&room_wall_1,    5);
  add_panel(&room_wall_2,    6);
  add_panel(&room_wall_3,    7);

  for (unsigned int i = 0; i < MAX_PANELS * 6*6; i++) {
      *(vertices+i*5+0) = vertex_buffer_data[i*3+0];
      *(vertices+i*5+1) = vertex_buffer_data[i*3+1];
      *(vertices+i*5+2) = vertex_buffer_data[i*3+2];
      *(vertices+i*5+3) = uv_buffer_data[i*2+0];
      *(vertices+i*5+4) = uv_buffer_data[i*2+1];
  }
  vkUnmapMemory(device, vertex_buffer_memory);

  // -------------------------------------------------

  glyph_instance_count = 0;

  size_t glyph_size = MAX_VISIBLE_GLYPHS * sizeof(fd_GlyphInstance);

  fd_GlyphInstance* glyphs;

  VK_CHECK(vkMapMemory(device, instance_staging_buffer_memory, 0, glyph_size, 0, &glyphs));

  add_text(&welcome_banner, 0, glyphs);
  add_text(&document,       1, glyphs);
  add_text(&info_board,     2, glyphs);
  add_text(&room_floor,     3, glyphs);
  add_text(&room_ceiling,   4, glyphs);
  add_text(&room_wall_1,    5, glyphs);
  add_text(&room_wall_2,    6, glyphs);
  add_text(&room_wall_3,    7, glyphs);

  vkUnmapMemory(device, instance_staging_buffer_memory);

  // -------------------------------------------------

  for (uint32_t i = 0; i < image_count; i++) {
      image_index = i;
      do_render_pass();
  }
  image_index = 0;

  // -------------------------------------------------

  scene_ready = true;
  pthread_mutex_unlock(&scene_lock);
}

void onx_render_frame() {

  VkFence previous_fence = swapchain_image_resources[image_index].command_buffer_fence;
  vkWaitForFences(device, 1, &previous_fence, VK_TRUE, UINT64_MAX);

  pthread_mutex_lock(&scene_lock);
  if(!scene_ready){
    pthread_mutex_unlock(&scene_lock);
    return;
  }

  VkResult err;
  do {
      err = vkAcquireNextImageKHR(device,
                                  swapchain,
                                  UINT64_MAX,
                                  image_acquired_semaphore,
                                  VK_NULL_HANDLE,
                                  &image_index);

      if (err == VK_SUCCESS || err == VK_SUBOPTIMAL_KHR){
        break;
      }
      else
      if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        ont_vk_restart();
      }
      else {
        pthread_mutex_unlock(&scene_lock);
        return;
      }
  } while(true);

  VkFence current_fence = swapchain_image_resources[image_index].command_buffer_fence;
  vkWaitForFences(device, 1, &current_fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &current_fence);

  set_mvp_uniforms();

  VkSemaphore wait_semaphores[] = { image_acquired_semaphore };
  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };
  VkSemaphore signal_semaphores[] = { render_complete_semaphore };

  VkSubmitInfo submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = wait_semaphores,
    .pWaitDstStageMask = wait_stages,
    .commandBufferCount = 1,
    .pCommandBuffers = &swapchain_image_resources[image_index].command_buffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signal_semaphores,
  };

  err = vkQueueSubmit(queue, 1, &submit_info, current_fence);

  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signal_semaphores,
    .swapchainCount = 1,
    .pSwapchains = &swapchain,
    .pImageIndices = &image_index,
    .pNext = NULL,
  };

  err = vkQueuePresentKHR(queue, &present_info);

  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    ont_vk_restart();
  }
  pthread_mutex_unlock(&scene_lock);
}

bool     head_moving=false;
bool     body_moving=false;
uint32_t x_on_press;
uint32_t y_on_press;

float dwell(float delta, float width){
  return delta > 0? max(delta - width, 0.0f):
                    min(delta + width, 0.0f);
}

void onx_iostate_changed() {
/*
  printf("onx_iostate_changed %d' [%d,%d][%d,%d] @(%d %d) buttons=(%d %d %d) key=%d\n",
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

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

static bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                *typeIndex = i;
                return true;
            }
        }
        typeBits >>= 1;
    }
    return false;
}

static uint32_t create_buffer_with_memory(VkBufferCreateInfo*   buffer_ci,
                                          VkMemoryPropertyFlags prop_flags,
                                          VkBuffer*             buffer,
                                          VkDeviceMemory*       memory){

  VK_CHECK(vkCreateBuffer(device, buffer_ci, 0, buffer));

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(device, *buffer, &mem_reqs);

  VkMemoryAllocateInfo memory_ai = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_reqs.size,
  };
  assert(memory_type_from_properties(mem_reqs.memoryTypeBits,
                                     prop_flags,
                                     &memory_ai.memoryTypeIndex));

  VK_CHECK(vkAllocateMemory(device, &memory_ai, 0, memory));

  VK_CHECK(vkBindBufferMemory(device, *buffer, *memory, 0));

  return memory_ai.allocationSize;
}

static uint32_t create_image_with_memory(VkImageCreateInfo*    image_ci,
                                         VkMemoryPropertyFlags prop_flags,
                                         VkImage*              image,
                                         VkDeviceMemory*       memory) {

  VK_CHECK(vkCreateImage(device, image_ci, 0, image));

  VkMemoryRequirements mem_reqs;
  vkGetImageMemoryRequirements(device, *image, &mem_reqs);

  VkMemoryAllocateInfo memory_ai = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = mem_reqs.size,
  };
  assert(memory_type_from_properties(mem_reqs.memoryTypeBits,
                                     prop_flags,
                                     &memory_ai.memoryTypeIndex));

  VK_CHECK(vkAllocateMemory(device, &memory_ai, 0, memory));

  VK_CHECK(vkBindImageMemory(device, *image, *memory, 0));

  return memory_ai.allocationSize;
}

static void create_uniform_buffer_with_memory(VkBufferCreateInfo* buffer_ci,
                                              VkMemoryPropertyFlags prop_flags,
                                              uint32_t ii){
    VK_CHECK(vkCreateBuffer(device,
                            buffer_ci,
                            0,
                            &uniform_mem[ii].uniform_buffer
    ));

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(
                            device,
                            uniform_mem[ii].uniform_buffer,
                            &mem_reqs);

    VkMemoryAllocateInfo memory_ai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_reqs.size,
    };
    assert(memory_type_from_properties(mem_reqs.memoryTypeBits,
                                       prop_flags,
                                       &memory_ai.memoryTypeIndex));

    VK_CHECK(vkAllocateMemory(device,
                             &memory_ai,
                              0,
                             &uniform_mem[ii].uniform_memory));

    VK_CHECK(vkMapMemory(device,
                         uniform_mem[ii].uniform_memory,
                         0, sizeof(struct uniforms), 0,
                        &uniform_mem[ii].uniform_memory_ptr));

    VK_CHECK(vkBindBufferMemory(device,
                                uniform_mem[ii].uniform_buffer,
                                uniform_mem[ii].uniform_memory,
                                0));
}

// ---------------------------------

static bool load_texture(const char *filename, uint8_t *rgba_data, VkSubresourceLayout *layout, int32_t *w, int32_t *h) {
    (void)filename;
    char *cPtr;
    cPtr = (char *)texture_array;
    if ((unsigned char *)cPtr >= (texture_array + texture_len) || strncmp(cPtr, "P6\n", 3)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    sscanf(cPtr, "%u %u", w, h);
    if (rgba_data == NULL) {
        return true;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    if ((unsigned char *)cPtr >= (texture_array + texture_len) || strncmp(cPtr, "255\n", 4)) {
        return false;
    }
    while (strncmp(cPtr++, "\n", 1))
        ;
    for (int y = 0; y < *h; y++) {
        uint8_t *rowPtr = rgba_data;
        for (int x = 0; x < *w; x++) {
            memcpy(rowPtr, cPtr, 3);
            rowPtr[3] = 255; /* Alpha of 1 */
            rowPtr += 4;
            cPtr += 3;
        }
        rgba_data += layout->rowPitch;
    }
    return true;
}

static VkShaderModule load_shader_module(const char *path) {

    FILE *f = fopen(path, "rb");
    if(!f){
      fprintf(stderr, "Cannot open shader %s\n", path);
      exit(-1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    void *code = malloc(size);
    fread(code, size, 1, f);
    fclose(f);

    VkShaderModuleCreateInfo module_ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code,
        .flags = 0,
        .pNext = 0,
    };

    VkShaderModule module;
    VK_CHECK(vkCreateShaderModule(device,
                                  &module_ci,
                                  0,
                                  &module));
    free(code);
    return module;
}

static void load_font(const char * font_face) {

  FT_Library library;
  FT_CHECK(FT_Init_FreeType(&library));

  FT_Face face;
  int err=FT_New_Face(library, font_face, 0, &face);
  if(err){
    fprintf(stderr, "Font loading failed or font not found: %s\n", font_face);
    exit(-1);
  }

  FT_CHECK(FT_Set_Char_Size(face, 0, 1000 * 64, 96, 96));

  uint32_t total_points = 0;
  uint32_t total_cells = 0;

  for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++) {

      char c = ' ' + i;
      printf("%c", c);

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
  printf("\n");

  glyph_info_size = sizeof(fd_DeviceGlyphInfo) * NUMBER_OF_GLYPHS;
  glyph_cells_size = sizeof(uint32_t) * total_cells;
  glyph_points_size = sizeof(vec2) * total_points;

  VkPhysicalDeviceProperties gpu_props;
  vkGetPhysicalDeviceProperties(gpu, &gpu_props);
  uint32_t alignment = gpu_props.limits.minStorageBufferOffsetAlignment;

  glyph_info_offset = 0;
  glyph_cells_offset = align_uint32(glyph_info_size, alignment);
  glyph_points_offset = align_uint32(glyph_info_size + glyph_cells_size, alignment);
  glyph_data_size = glyph_points_offset + glyph_points_size;
  glyph_data = malloc(glyph_data_size);

  fd_DeviceGlyphInfo *device_glyph_infos = (fd_DeviceGlyphInfo*)
                                             ((char*)glyph_data + glyph_info_offset);

  uint32_t *cells = (uint32_t*)((char*)glyph_data + glyph_cells_offset);
  vec2 *points = (vec2*)((char*)glyph_data + glyph_points_offset);

  uint32_t point_offset = 0;
  uint32_t cell_offset = 0;

  for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++) {

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

  for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++){
      fd_outline_destroy(&outlines[i]);
  }

  FT_CHECK(FT_Done_Face(face));
  FT_CHECK(FT_Done_FreeType(library));
}

// ---------------------------------

static void prepare_texture_image(const char *filename,
                                  struct texture_object *texture_obj,
                                  VkImageTiling tiling,
                                  VkImageUsageFlags usage,
                                  VkFlags prop_flags) {
    int32_t texture_width;
    int32_t texture_height;
    VkResult err;

    if (!load_texture(filename, NULL, NULL, &texture_width, &texture_height)) {
        ERR_EXIT("Failed to load textures");
    }

    texture_obj->texture_width = texture_width;
    texture_obj->texture_height = texture_height;

    VkImageCreateInfo image_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = texture_format,
        .extent = {texture_width, texture_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = tiling,
        .usage = usage,
        .flags = 0,
        .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
    };

    uint32_t size = create_image_with_memory(&image_ci,
                                             prop_flags,
                                             &texture_obj->image,
                                             &texture_obj->device_memory);

    if (prop_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        VkSubresourceLayout layout;

        vkGetImageSubresourceLayout(device, texture_obj->image, &subres, &layout);

        void *data;
        VK_CHECK(vkMapMemory(device, texture_obj->device_memory, 0, size, 0, &data));

        if (!load_texture(filename, data, &layout, &texture_width, &texture_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }
        vkUnmapMemory(device, texture_obj->device_memory);
    }
    texture_obj->image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static void prepare_texture_buffer(const char *filename, struct texture_object *texture_obj) {
    int32_t texture_width;
    int32_t texture_height;
    VkResult err;

    if (!load_texture(filename, NULL, NULL, &texture_width, &texture_height)) {
        ERR_EXIT("Failed to load textures");
    }

    texture_obj->texture_width = texture_width;
    texture_obj->texture_height = texture_height;

    VkBufferCreateInfo buffer_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .flags = 0,
      .size = texture_width * texture_height * 4,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 0,
      .pQueueFamilyIndices = NULL,
      .pNext = 0,
    };

    VkFlags prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t size = create_buffer_with_memory(&buffer_ci,
                                              prop_flags,
                                              &texture_obj->buffer,
                                              &texture_obj->device_memory);
    VkSubresourceLayout layout;
    memset(&layout, 0, sizeof(layout));
    layout.rowPitch = texture_width * 4;

    void *data;
    err = vkMapMemory(device, texture_obj->device_memory, 0, size, 0, &data);
    assert(!err);

    if (!load_texture(filename, data, &layout, &texture_width, &texture_height)) {
        fprintf(stderr, "Error loading texture: %s\n", filename);
    }

    vkUnmapMemory(device, texture_obj->device_memory);
}

static void set_image_layout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout,
                             VkImageLayout new_image_layout, VkAccessFlagBits srcAccessMask, VkPipelineStageFlags src_stages,
                             VkPipelineStageFlags dest_stages) {

    VkImageMemoryBarrier image_memory_barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                 .pNext = NULL,
                                                 .srcAccessMask = srcAccessMask,
                                                 .dstAccessMask = 0,
                                                 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                                                 .oldLayout = old_image_layout,
                                                 .newLayout = new_image_layout,
                                                 .image = image,
                                                 .subresourceRange = {aspectMask, 0, 1, 0, 1}};

    switch (new_image_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            /* Make sure anything that was copying from this image has completed */
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            break;

        default:
            image_memory_barrier.dstAccessMask = 0;
            break;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    vkCmdPipelineBarrier(initcmd, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, pmemory_barrier);
}

static void prepare_textures(){

    uint32_t i;

    for (i = 0; i < TEXTURE_COUNT; i++) {
        VkResult err;

        if ((format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !use_staging_buffer) {
            /* Device can texture using linear textures */
            prepare_texture_image(texture_files[i], &textures[i], VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            set_image_layout(textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                    textures[i].image_layout, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
/*
            staging_texture.image = 0;
        } else if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {

            memset(&staging_texture, 0, sizeof(staging_texture));
            prepare_texture_buffer(texture_files[i], &staging_texture);

            prepare_texture_image(texture_files[i], &textures[i], VK_IMAGE_TILING_OPTIMAL,
                                       (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            set_image_layout(textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                  VK_PIPELINE_STAGE_TRANSFER_BIT);

            VkBufferImageCopy copy_region = {
                .bufferOffset = 0,
                .bufferRowLength = staging_texture.texture_width,
                .bufferImageHeight = staging_texture.texture_height,
                .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
                .imageOffset = {0, 0, 0},
                .imageExtent = {staging_texture.texture_width, staging_texture.texture_height, 1},
            };

            vkCmdCopyBufferToImage(initcmd, staging_texture.buffer, textures[i].image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

            set_image_layout(textures[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  textures[i].image_layout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

*/
        } else {
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_FILTER_NEAREST,
            .minFilter = VK_FILTER_NEAREST,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = texture_format,
            .components =
                {
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                    VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
            .flags = 0,
        };

        err = vkCreateSampler(device, &sampler, NULL, &textures[i].sampler);
        assert(!err);

        image_view_ci.image = textures[i].image;
        VK_CHECK(vkCreateImageView(device, &image_view_ci, NULL, &textures[i].image_view));
    }
/*
    if (staging_texture.buffer) {
        vkFreeMemory(device, staging_texture.device_memory, NULL);
        if(staging_texture.image) vkDestroyImage(device, staging_texture.image, NULL);
        vkDestroyBuffer(device, staging_texture.buffer, NULL);
    }
*/
}

static void prepare_depth() {
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkImageCreateInfo image_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = { io.swap_width, io.swap_height, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0,
    };

    VkImageViewCreateInfo image_view_ci = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = VK_NULL_HANDLE,
        .format = depth_format,
        .subresourceRange =
            {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };

    VkMemoryRequirements mem_reqs;
    VkResult err;
    bool pass;

    depth.format = depth_format;

    VkMemoryPropertyFlags prop_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    create_image_with_memory(&image_ci,
                             prop_flags,
                             &depth.image,
                             &depth.device_memory);

    image_view_ci.image = depth.image;
    VK_CHECK(vkCreateImageView(device, &image_view_ci, NULL, &depth.image_view));
}

static void prepare_glyph_buffers() {

  // ------------------

  VkBufferCreateInfo storage_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = glyph_data_size,
      .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  create_buffer_with_memory(&storage_ci,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            &storage_buffer,
                            &storage_buffer_memory);

  // ------------------

  VkBufferCreateInfo staging_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = glyph_data_size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };


  create_buffer_with_memory(&staging_ci,
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                            &staging_buffer,
                            &staging_buffer_memory);

  void *staging_buffer_ptr;
  VK_CHECK(vkMapMemory(device,
                       staging_buffer_memory,
                       0,
                       glyph_data_size,
                       0,
                       &staging_buffer_ptr));

  memcpy(staging_buffer_ptr, glyph_data, glyph_data_size);

  vkUnmapMemory(device, staging_buffer_memory);


  VkBufferCopy copy = { 0, 0, glyph_data_size };

  vkCmdCopyBuffer(initcmd, staging_buffer, storage_buffer, 1, &copy);

  free(glyph_data);
  glyph_data = NULL;

  // ------------------

  VkBufferCreateInfo instance_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = MAX_VISIBLE_GLYPHS * sizeof(fd_GlyphInstance),
      .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  create_buffer_with_memory(&instance_ci,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                            &instance_buffer,
                            &instance_buffer_memory);

  // ------------------

  VkBufferCreateInfo instance_staging_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = instance_ci.size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  create_buffer_with_memory(&instance_staging_ci,
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                            &instance_staging_buffer,
                            &instance_staging_buffer_memory);
}

// --------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------

void onx_prepare_swapchain_images(bool restart) {
    VkResult err;
    err = vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    assert(!err);

    VkImage *swapchainImages = (VkImage *)malloc(image_count * sizeof(VkImage));
    assert(swapchainImages);
    err = vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchainImages);
    assert(!err);

    swapchain_image_resources = (SwapchainImageResources *)malloc(sizeof(SwapchainImageResources) * image_count);

    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo image_view_ci = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = surface_format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
        };

        image_view_ci.image = swapchainImages[i];
        VK_CHECK(vkCreateImageView(device, &image_view_ci, NULL, &swapchain_image_resources[i].image_view));
    }

    if (NULL != swapchainImages) {
        free(swapchainImages);
    }
}

void onx_prepare_semaphores_and_fences(bool restart) {

  VkFenceCreateInfo fence_ci = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
      .pNext = 0,
  };

  for (uint32_t i = 0; i < image_count; i++) {
      VK_CHECK(vkCreateFence(device, &fence_ci, 0, &swapchain_image_resources[i].command_buffer_fence));
  }

  VkSemaphoreCreateInfo semaphore_ci = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = 0,
  };

  VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, 0, &image_acquired_semaphore));
  VK_CHECK(vkCreateSemaphore(device, &semaphore_ci, 0, &render_complete_semaphore));
}

void onx_prepare_command_buffers(bool restart){

  VkCommandBufferAllocateInfo command_buffer_ai = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
      .pNext = 0,
  };

  for (uint32_t i = 0; i < image_count; i++) {
      VK_CHECK(vkAllocateCommandBuffers(
                       device,
                       &command_buffer_ai,
                       &swapchain_image_resources[i].command_buffer
      ));
  }
}

void onx_prepare_render_data(bool restart) {

  const char* font_face = "./fonts/Roboto-Medium.ttf";
  load_font(font_face);

  vkGetPhysicalDeviceFormatProperties(gpu, texture_format, &format_properties);
  vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);

  prepare_depth();
  prepare_glyph_buffers();
  prepare_textures();
}

static void prepare_vertex_buffers(){

  VkBufferCreateInfo buffer_ci = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = MAX_PANELS * 6*6 * (3 * sizeof(vertex_buffer_data[0]) +
                                2 * sizeof(uv_buffer_data[0])),
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  create_buffer_with_memory(&buffer_ci,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            &vertex_buffer,
                            &vertex_buffer_memory);
}

void onx_prepare_uniform_buffers(bool restart) {

  prepare_vertex_buffers();

  uniform_mem = (uniform_mem_t*)malloc(sizeof(uniform_mem_t) * image_count);

  VkBufferCreateInfo buffer_ci = {
     .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
     .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
     .size = sizeof(struct uniforms),
  };
  for (uint32_t i = 0; i < image_count; i++) {

    create_uniform_buffer_with_memory(&buffer_ci,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      i);
  }
}

void onx_prepare_descriptor_layout(bool restart) {

  VkDescriptorSetLayoutBinding bindings[] = {
      {
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .pImmutableSamplers = NULL,
      },
      {
          .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      },
      {
          .binding = 2,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      {
          .binding = 3,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      {
          .binding = 4,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = TEXTURE_COUNT,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .pImmutableSamplers = NULL,
      },
  };

  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_ci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 5,
      .pBindings = bindings,
  };

  VK_CHECK(vkCreateDescriptorSetLayout(device,
                                       &descriptor_set_layout_ci,
                                       0,
                                       &descriptor_layout));

  VkPushConstantRange push_constant_range = {
    .offset = 0,
    .size = sizeof(struct push_constants),
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };

  VkPipelineLayoutCreateInfo pipeline_layout_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &descriptor_layout,
      .pPushConstantRanges = &push_constant_range,
      .pushConstantRangeCount = 1,
      .pNext = 0,
  };

  VK_CHECK(vkCreatePipelineLayout(device,
                                  &pipeline_layout_ci,
                                  0,
                                  &pipeline_layout));
}

void onx_prepare_descriptor_pool(bool restart) {

  VkDescriptorPoolSize pool_sizes[] = {
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        image_count                 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        image_count * 3             },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        image_count * TEXTURE_COUNT },
  };

  VkDescriptorPoolCreateInfo descriptor_pool_ci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = (image_count * 3) + image_count + (image_count * TEXTURE_COUNT) + 23, //// ??
      .poolSizeCount = 3,
      .pPoolSizes = pool_sizes,
      .pNext = 0,
  };

  VK_CHECK(vkCreateDescriptorPool(device, &descriptor_pool_ci, NULL, &descriptor_pool));
}

void onx_prepare_descriptor_set(bool restart) {

  VkDescriptorSetAllocateInfo descriptor_set_ai = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &descriptor_layout,
    .pNext = 0,
  };

  VkDescriptorBufferInfo uniform_info = {
    .offset = 0,
    .range = sizeof(struct uniforms),
  };

  VkDescriptorBufferInfo glyph_info = {
    .buffer = storage_buffer,
    .offset = glyph_info_offset,
    .range = glyph_info_size,
  };

  VkDescriptorBufferInfo cells_info = {
    .buffer = storage_buffer,
    .offset = glyph_cells_offset,
    .range = glyph_cells_size,
  };

  VkDescriptorBufferInfo points_info = {
    .buffer = storage_buffer,
    .offset = glyph_points_offset,
    .range = glyph_points_size,
  };

  VkDescriptorImageInfo texture_descs[TEXTURE_COUNT];
  memset(&texture_descs, 0, sizeof(texture_descs));

  for (unsigned int i = 0; i < TEXTURE_COUNT; i++) {
    texture_descs[i].sampler = textures[i].sampler;
    texture_descs[i].imageView = textures[i].image_view;
    texture_descs[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  VkWriteDescriptorSet writes[] = {
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .dstBinding = 0,
      .pBufferInfo = &uniform_info,
      .descriptorCount = 1,
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .dstBinding = 1,
      .pBufferInfo = &glyph_info,
      .dstArrayElement = 0,
      .descriptorCount = 1,
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .dstBinding = 2,
      .pBufferInfo = &cells_info,
      .dstArrayElement = 0,
      .descriptorCount = 1,
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .dstBinding = 3,
      .pBufferInfo = &points_info,
      .dstArrayElement = 0,
      .descriptorCount = 1,
    },
    {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .dstBinding = 4,
      .pImageInfo = texture_descs,
      .descriptorCount = TEXTURE_COUNT,
    },
  };

  for (uint32_t i = 0; i < image_count; i++) {

    VK_CHECK(vkAllocateDescriptorSets(
                              device,
                             &descriptor_set_ai,
                             &uniform_mem[i].descriptor_set));

    uniform_info.buffer = uniform_mem[i].uniform_buffer;
    writes[0].dstSet = uniform_mem[i].descriptor_set;
    writes[1].dstSet = uniform_mem[i].descriptor_set;
    writes[2].dstSet = uniform_mem[i].descriptor_set;
    writes[3].dstSet = uniform_mem[i].descriptor_set;
    writes[4].dstSet = uniform_mem[i].descriptor_set;

    vkUpdateDescriptorSets(device, 5, writes, 0, 0);
  }
}

void onx_prepare_render_pass(bool restart) {
    const VkAttachmentDescription attachments[2] = {
        [0] =
            {
                .format = surface_format,
                .flags = 0,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            },
        [1] =
            {
                .format = depth.format,
                .flags = 0,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
    };
    const VkAttachmentReference color_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkAttachmentReference depth_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .flags = 0,
        .inputAttachmentCount = 0,
        .pInputAttachments = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_reference,
        .pResolveAttachments = NULL,
        .pDepthStencilAttachment = &depth_reference,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = NULL,
    };

    VkSubpassDependency attachmentDependencies[2] = {
        [0] =
            {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = 0,
            },
        [1] =
            {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                .dependencyFlags = 0,
            },
    };

    const VkRenderPassCreateInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 2,
        .pDependencies = attachmentDependencies,
    };
    VkResult err;

    err = vkCreateRenderPass(device, &rp_info, NULL, &render_pass);
    assert(!err);
}

void onx_prepare_pipeline(bool restart) {

  VkShaderModule vert_shader_module = load_shader_module("./shaders/onx.vert.spv");
  VkShaderModule frag_shader_module = load_shader_module("./shaders/onx.frag.spv");

  VkPipelineShaderStageCreateInfo shader_stages[] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = vert_shader_module,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = frag_shader_module,
      .pName = "main",
    }
  };

  VkVertexInputBindingDescription vertices_input_binding = {
    .binding = 0,
    .stride = 3 * sizeof(vertex_buffer_data[0]) +
              2 * sizeof(    uv_buffer_data[0]),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  VkVertexInputBindingDescription vertex_input_binding = {
    .binding = 1,
    .stride = sizeof(fd_GlyphInstance),
    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
  };

  VkVertexInputAttributeDescription vertex_input_attributes[] = {
    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },     // vertex
    { 1, 0, VK_FORMAT_R32G32_SFLOAT, 12 },       // uv
    { 2, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },  // fd_GlyphInstance.rect
    { 3, 1, VK_FORMAT_R32_UINT, 16 },            // fd_GlyphInstance.glyph_index
    { 4, 1, VK_FORMAT_R32_SFLOAT, 20 },          // fd_GlyphInstance.sharpness
  };

  VkVertexInputBindingDescription vibds[] = {
    vertices_input_binding,
    vertex_input_binding,
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 2,
      .pVertexBindingDescriptions = vibds,
      .vertexAttributeDescriptionCount = 5,
      .pVertexAttributeDescriptions = vertex_input_attributes,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width  = io.swap_width,
      .height = io.swap_height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  VkRect2D scissor = {
      .offset = { 0, 0 },
      .extent = swapchain_extent,
  };

  VkPipelineViewportStateCreateInfo viewport_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
      .pViewports = &viewport,
      .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterizer_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisample_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .pSampleMask = NULL,
  };

  VkPipelineColorBlendAttachmentState color_blend_as = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_MAX,
  };

  VkPipelineColorBlendStateCreateInfo blend_state_ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_blend_as,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY, // enabled?
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_ci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    .depthBoundsTestEnable = VK_FALSE,
    .back.failOp = VK_STENCIL_OP_KEEP,
    .back.passOp = VK_STENCIL_OP_KEEP,
    .back.compareOp = VK_COMPARE_OP_ALWAYS,
    .stencilTestEnable = VK_FALSE,
  };
  depth_stencil_ci.front = depth_stencil_ci.back;

  VkPipelineCacheCreateInfo pipeline_cache_ci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
  };

  VK_CHECK(vkCreatePipelineCache(device,
                                 &pipeline_cache_ci,
                                 0,
                                 &pipeline_cache));

  VkGraphicsPipelineCreateInfo graphics_pipeline_ci[] = {
    {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pViewportState = &viewport_state,
      .pRasterizationState = &rasterizer_state,
      .pMultisampleState = &multisample_ci,
      .pColorBlendState = &blend_state_ci,
      .pDepthStencilState = &depth_stencil_ci,
      .pStages = shader_stages,
      .pVertexInputState = &vertex_input_state,
      .pInputAssemblyState = &input_assembly_state,
      .layout = pipeline_layout,
      .renderPass = render_pass,
      .subpass = 0,
    },
  };

  VK_CHECK(vkCreateGraphicsPipelines(device,
                                     pipeline_cache,
                                     1,
                                     graphics_pipeline_ci,
                                     0,
                                     &pipeline));

  vkDestroyShaderModule(device, frag_shader_module, NULL);
  vkDestroyShaderModule(device, vert_shader_module, NULL);
}

void onx_prepare_framebuffers(bool restart) {
    VkImageView attachments[2];
    attachments[1] = depth.image_view;

    const VkFramebufferCreateInfo fb_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = render_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width =  io.swap_width,
        .height = io.swap_height,
        .layers = 1,
    };
    VkResult err;
    uint32_t i;

    for (i = 0; i < image_count; i++) {
        attachments[0] = swapchain_image_resources[i].image_view;
        err = vkCreateFramebuffer(device, &fb_info, NULL, &swapchain_image_resources[i].framebuffer);
        assert(!err);
    }
}

// --------------------------------------------------------------------------------------------------------

void onx_finish() {

  scene_ready = false;

  printf("onx_finish\n");

  for (uint32_t i = 0; i < image_count; i++) {
    vkWaitForFences(device, 1, &swapchain_image_resources[i].command_buffer_fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, swapchain_image_resources[i].command_buffer_fence, NULL);
  }

  vkDestroyPipeline(device, pipeline, NULL);
  vkDestroyPipelineCache(device, pipeline_cache, NULL);

  // ---------------------------------

  vkFreeMemory(device, vertex_buffer_memory, NULL);
  vkFreeMemory(device, staging_buffer_memory, NULL);
  vkFreeMemory(device, storage_buffer_memory, NULL);
  vkFreeMemory(device, instance_buffer_memory, NULL);
  vkFreeMemory(device, instance_staging_buffer_memory, NULL);

  vkDestroyBuffer(device, vertex_buffer, NULL);
  vkDestroyBuffer(device, staging_buffer, NULL);
  vkDestroyBuffer(device, storage_buffer, NULL);
  vkDestroyBuffer(device, instance_buffer, NULL);
  vkDestroyBuffer(device, instance_staging_buffer, NULL);

  // ---------------------------------

  vkDestroyDescriptorPool(device, descriptor_pool, NULL);

  // ---------------------------------

  vkDestroyPipelineLayout(device, pipeline_layout, NULL);
  vkDestroyDescriptorSetLayout(device, descriptor_layout, NULL);

  // ---------------------------------

  for (uint32_t i = 0; i < image_count; i++) {
      vkDestroyBuffer(device, uniform_mem[i].uniform_buffer, NULL);
      vkUnmapMemory(device, uniform_mem[i].uniform_memory);
      vkFreeMemory(device, uniform_mem[i].uniform_memory, NULL);
  }
  free(uniform_mem);

  // ---------------------------------

  for (uint32_t i = 0; i < TEXTURE_COUNT; i++) {
    vkDestroyImageView(device, textures[i].image_view, NULL);
    vkDestroyImage(device, textures[i].image, NULL);
    vkFreeMemory(device, textures[i].device_memory, NULL);
    vkDestroySampler(device, textures[i].sampler, NULL);
  }

  vkDestroyImageView(device, depth.image_view, NULL);
  vkDestroyImage(device, depth.image, NULL);
  vkFreeMemory(device, depth.device_memory, NULL);

  uint32_t i;
  if (swapchain_image_resources) {
     for (i = 0; i < image_count; i++) {
         vkFreeCommandBuffers(device, command_pool, 1, &swapchain_image_resources[i].command_buffer);
         vkDestroyFramebuffer(device, swapchain_image_resources[i].framebuffer, NULL);
         vkDestroyImageView(device, swapchain_image_resources[i].image_view, NULL);
     }
     free(swapchain_image_resources);
  }

  // ---------------------------------

  VK_DESTROY(vkDestroySemaphore, device, image_acquired_semaphore);
  VK_DESTROY(vkDestroySemaphore, device, render_complete_semaphore);

  vkDestroyRenderPass(device, render_pass, NULL);
}

// ---------------------------------
