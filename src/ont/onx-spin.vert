#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct GlyphInfo {
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
  GlyphInfo glyphs[];
} glyph_buffer;

layout(location = 0) in vec4  in_rect;
layout(location = 1) in uint  in_glyph_index;
layout(location = 2) in float in_sharpness;

layout(location = 0) out vec2  out_glyph_pos;
layout(location = 1) out uvec4 out_cell_info;
layout(location = 2) out float out_sharpness;
layout(location = 3) out vec2  out_cell_coord;
layout(location = 4) out vec4  out_texture_coord;
layout(location = 5) out vec3  out_frag_pos;
layout(location = 6) out uint  out_phase;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {

  if(push_constants.phase == 1){ // panel

    out_texture_coord = uniforms.uvs[gl_VertexIndex];
    gl_Position = uniforms.proj *
                  uniforms.view *
                  uniforms.model *
                  uniforms.vertices[gl_VertexIndex];
  }
  else
  if(push_constants.phase == 2){ // text

    GlyphInfo gi = glyph_buffer.glyphs[in_glyph_index];

    vec2 rect_verts[6] = vec2[](
        vec2(in_rect.x, in_rect.y),
        vec2(in_rect.z, in_rect.y),
        vec2(in_rect.x, in_rect.w),

        vec2(in_rect.x, in_rect.w),
        vec2(in_rect.z, in_rect.y),
        vec2(in_rect.z, in_rect.w)
    );

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
                  vec4(rect_verts[gl_VertexIndex], -0.001, 1.0);

  }
  out_frag_pos = gl_Position.xyz;
  out_phase = push_constants.phase;
}




