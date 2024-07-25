#version 450
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_multiview : enable

struct glyph_info {
    vec4 bbox;
    uvec4 cell_info;
};

layout(push_constant) uniform constants {
  uint phase;
} push_constants;

// TODO model[MAX_PANELS]
//      text_ends[MAX_PANELS]
layout(std430, binding = 0) uniform buf0 {
  mat4 proj;
  mat4 view_l;
  mat4 view_r;
  mat4 model[32];
  vec4 text_ends[32];
} uniforms;

layout (set = 0, binding = 1) buffer buf1 {
  glyph_info glyphs[];
} glyph_buffer;

layout(location = 0)  in  vec3  vertex;
layout(location = 1)  in  vec2  uv;
layout(location = 2)  in  vec4  rect;
layout(location = 3)  in  uint  glyph_index;
layout(location = 4)  in  float sharpness_i;

layout(location = 0)  out vec2  glyph_pos;
layout(location = 1)  out uvec4 cell_info;
layout(location = 2)  out float sharpness;
layout(location = 3)  out vec2  cell_coord;
layout(location = 4)  out vec4  texture_coord;
layout(location = 5)  out vec4  model_pos;
layout(location = 6)  out vec4  proj_pos;
layout(location = 7)  out uint  phase;
layout(location = 8)  out float near;
layout(location = 9)  out float far;
layout(location = 10) out vec3  near_point;
layout(location = 11) out vec3  far_point;
layout(location = 12) out mat4  view;
layout(location = 16) out mat4  proj;

out gl_PerVertex {
    vec4 gl_Position;
};

vec3 grid_plane[6] = vec3[] (
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

vec3 unproject(float x, float y, float z, mat4 view, mat4 proj) {
    mat4 view_inv = inverse(view);
    mat4 proj_inv = inverse(proj);
    vec4 unprojected_point =  view_inv * proj_inv * vec4(x, y, z, 1.0);
    return unprojected_point.xyz / unprojected_point.w;
}

void main() {

  view = gl_ViewIndex==0? uniforms.view_l: uniforms.view_r;
  proj = uniforms.proj;
  phase = push_constants.phase;

  if(phase == 0){ // ground plane

    near = 0.004;
    far = 0.17;
    vec3 p = grid_plane[gl_VertexIndex].xyz;
    near_point = unproject(p.x, p.y, 0.0, view, proj).xyz;
    far_point  = unproject(p.x, p.y, 1.0, view, proj).xyz;
    gl_Position = vec4(p, 1.0);
  }
  else
  if(phase == 1){ // panels

    texture_coord = vec4(uv, 0, 0);

    gl_Position = proj * view *
                  uniforms.model[gl_InstanceIndex] *
                  vec4(vertex, 1.0);

    model_pos = uniforms.model[gl_InstanceIndex] *
                vec4(vertex, 1.0);
  }
  else
  if(phase == 2){ // text

    vec2 rv;
    if(gl_VertexIndex==0) rv = vec2(rect.x, rect.y);
    if(gl_VertexIndex==1) rv = vec2(rect.z, rect.y);
    if(gl_VertexIndex==2) rv = vec2(rect.x, rect.w);
    if(gl_VertexIndex==3) rv = vec2(rect.x, rect.w);
    if(gl_VertexIndex==4) rv = vec2(rect.z, rect.y);
    if(gl_VertexIndex==5) rv = vec2(rect.z, rect.w);

    glyph_info gi = glyph_buffer.glyphs[glyph_index];

    if(gl_VertexIndex==0) glyph_pos = vec2(gi.bbox.x, gi.bbox.y);
    if(gl_VertexIndex==1) glyph_pos = vec2(gi.bbox.z, gi.bbox.y);
    if(gl_VertexIndex==2) glyph_pos = vec2(gi.bbox.x, gi.bbox.w);
    if(gl_VertexIndex==3) glyph_pos = vec2(gi.bbox.x, gi.bbox.w);
    if(gl_VertexIndex==4) glyph_pos = vec2(gi.bbox.z, gi.bbox.y);
    if(gl_VertexIndex==5) glyph_pos = vec2(gi.bbox.z, gi.bbox.w);

    if(gl_VertexIndex==0) cell_coord = vec2(0,              0);
    if(gl_VertexIndex==1) cell_coord = vec2(gi.cell_info.z, 0);
    if(gl_VertexIndex==2) cell_coord = vec2(0,              gi.cell_info.w);
    if(gl_VertexIndex==3) cell_coord = vec2(0,              gi.cell_info.w);
    if(gl_VertexIndex==4) cell_coord = vec2(gi.cell_info.z, 0);
    if(gl_VertexIndex==5) cell_coord = vec2(gi.cell_info.z, gi.cell_info.w);

    cell_info = gi.cell_info;
    sharpness = sharpness_i;

    int p;
    //  TODO p<MAX_PANELS
    for(p=0; p<32; p++){
        float s=uniforms.text_ends[p][0];
        float e=uniforms.text_ends[p][1];
        if(gl_InstanceIndex >= s && gl_InstanceIndex < e) break;
    }

    float text_lift = -0.02;

    gl_Position = proj * view *
                  uniforms.model[p] *
                  vec4(rv, text_lift, 1.0);
  }
  proj_pos = gl_Position;
}
