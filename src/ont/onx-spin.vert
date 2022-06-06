#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct glyph_info {
    vec4 bbox;
    uvec4 cell_info;
};

layout(push_constant) uniform constants {
  uint phase;
} push_constants;

layout(std140, binding = 0) uniform buf0 {
  mat4 proj;
  mat4 view;
  mat4 model;
  vec4 vertices[12*3];
  vec4 uvs[12*3];
} uniforms;

layout (set = 0, binding = 1) buffer buf1 {
  glyph_info glyphs[];
} glyph_buffer;

layout(location = 0) in vec4  in_rect;
layout(location = 1) in uint  in_glyph_index;
layout(location = 2) in float in_sharpness;

layout(location = 0)  out vec2  out_glyph_pos;
layout(location = 1)  out uvec4 out_cell_info;
layout(location = 2)  out float out_sharpness;
layout(location = 3)  out vec2  out_cell_coord;
layout(location = 4)  out vec4  out_texture_coord;
layout(location = 5)  out vec4  out_frag_pos;
layout(location = 6)  out uint  out_phase;
layout(location = 7)  out float near;
layout(location = 8)  out float far;
layout(location = 9)  out vec3  nearPoint;
layout(location = 10) out vec3  farPoint;
layout(location = 11) out mat4  view;
layout(location = 15) out mat4  proj;

out gl_PerVertex {
    vec4 gl_Position;
};

vec3 gridPlane[6] = vec3[] (
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

vec3 unproject(float x, float y, float z, mat4 view, mat4 proj) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(proj);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {

  if(push_constants.phase == 0){ // ground plane

    near = 0.004;
    far = 0.17;
    vec3 p = gridPlane[gl_VertexIndex].xyz;
    nearPoint = unproject(p.x, p.y, 0.0, uniforms.view, uniforms.proj).xyz;
    farPoint = unproject(p.x, p.y, 1.0, uniforms.view, uniforms.proj).xyz;
    view = uniforms.view;
    proj = uniforms.proj;
    gl_Position = vec4(p, 1.0);
  }
  else
  if(push_constants.phase == 1){ // panel

    out_texture_coord = uniforms.uvs[gl_VertexIndex];
    gl_Position = uniforms.proj *
                  uniforms.view *
                  uniforms.model *
                  uniforms.vertices[gl_VertexIndex];
  }
  else
  if(push_constants.phase == 2){ // text

    glyph_info gi = glyph_buffer.glyphs[in_glyph_index];

    vec2 rv;
    if(gl_VertexIndex==0) rv = vec2(in_rect.x, in_rect.y);
    if(gl_VertexIndex==1) rv = vec2(in_rect.z, in_rect.y);
    if(gl_VertexIndex==2) rv = vec2(in_rect.x, in_rect.w);
    if(gl_VertexIndex==3) rv = vec2(in_rect.x, in_rect.w);
    if(gl_VertexIndex==4) rv = vec2(in_rect.z, in_rect.y);
    if(gl_VertexIndex==5) rv = vec2(in_rect.z, in_rect.w);

    vec2 glyph_pos[6] = vec2[](
        vec2(gi.bbox.x, gi.bbox.y),
        vec2(gi.bbox.z, gi.bbox.y),
        vec2(gi.bbox.x, gi.bbox.w),

        vec2(gi.bbox.x, gi.bbox.w),
        vec2(gi.bbox.z, gi.bbox.y),
        vec2(gi.bbox.z, gi.bbox.w)
    );

    vec2 cell_coord[6] = vec2[](
        vec2(0,              0),
        vec2(gi.cell_info.z, 0),
        vec2(0,              gi.cell_info.w),

        vec2(0,              gi.cell_info.w),
        vec2(gi.cell_info.z, 0),
        vec2(gi.cell_info.z, gi.cell_info.w)
    );

    out_glyph_pos = glyph_pos[gl_VertexIndex];
    out_cell_info = gi.cell_info;
    out_sharpness = in_sharpness;
    out_cell_coord = cell_coord[gl_VertexIndex];

    gl_Position = uniforms.proj *
                  uniforms.view *
                  uniforms.model *
                  mat4(-2.0,  0.0, 0.0, 0.0,
                       0.0, -1.0, 0.0, 0.0,
                       0.0,  0.0, 1.0, 0.0,
                       0.0,  0.0, 0.0, 1.0
                  ) *
                  vec4(rv, -0.001, 1.0);

  }
  out_frag_pos = gl_Position;
  out_phase = push_constants.phase;
}




