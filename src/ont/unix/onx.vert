#version 450
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_multiview : enable

// TODO model[MAX_PANELS]
//      text_ends[MAX_PANELS]
layout(std430, binding = 0) uniform buf0 {
  mat4 proj;
  mat4 view_l;
  mat4 view_r;
  vec2 left_touch;
} uniforms;

layout(location = 0)  out vec2  glyph_pos;
layout(location = 1)  out uvec4 cell_info;
layout(location = 2)  out float sharpness;
layout(location = 3)  out vec2  cell_coord;
layout(location = 4)  out vec4  texture_coord;
layout(location = 5)  out vec4  model_pos;
layout(location = 6)  out vec4  proj_pos;
layout(location = 7)  out uint  phase;
layout(location = 8)  out vec2  left_touch;
layout(location = 9)  out vec2  overlay_uv;
layout(location = 10) out float near;
layout(location = 11) out float far;
layout(location = 12) out vec3  near_point;
layout(location = 13) out vec3  far_point;
layout(location = 14) out mat4  view;
layout(location = 18) out mat4  proj;
layout(location = 22) out mat4  inv_view;
layout(location = 26) out mat4  inv_proj;

out gl_PerVertex {
    vec4 gl_Position;
};

vec2 overlay_quad[4] = vec2[](
    vec2(-1.0, -1.0), // bottom-left
    vec2( 1.0, -1.0), // bottom-right
    vec2(-1.0,  1.0), // top-left
    vec2( 1.0,  1.0)  // top-right
);

int overlay_quad_indices[6] = int[](
    2, 3, 0,
    0, 3, 1
);

void main() {

  view = gl_ViewIndex==0? uniforms.view_l: uniforms.view_r;
  proj = uniforms.proj;
  left_touch = uniforms.left_touch;

  if(true){

    inv_view = inverse(view);
    inv_proj = inverse(proj);

    gl_Position = vec4(overlay_quad[overlay_quad_indices[gl_VertexIndex]], 0.0, 1.0);
    float aspect_ratio = 1.778;
    overlay_uv = vec2((gl_Position.x * 0.5 + 0.5)*aspect_ratio, gl_Position.y * 0.5 + 0.5);
  }
  proj_pos = gl_Position;
}
