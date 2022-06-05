#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 2) buffer buf2 {
  uint cells[];
} cell_buffer;

layout (set = 0, binding = 3) buffer buf3 {
  vec2 points[];
} point_buffer;

layout (set = 0, binding = 4) uniform sampler2D tex;

layout(location = 0)      in vec2  in_glyph_pos;
layout(location = 1) flat in uvec4 in_cell_info;
layout(location = 2)      in float in_sharpness;
layout(location = 3)      in vec2  in_cell_coord;
layout(location = 4)      in vec4  in_texture_coord;
layout(location = 5)      in vec4  in_frag_pos;
layout(location = 6) flat in uint  in_phase;
layout(location = 7)      in float near;
layout(location = 8)      in float far;
layout(location = 9)      in vec3  nearPoint;
layout(location = 10)     in vec3  farPoint;
layout(location = 11)     in mat4  view;
layout(location = 15)     in mat4  proj;

layout(location = 0) out vec4 out_color;

vec4 grid_colour(vec3 pos, float scale) {
    vec2 coord = pos.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float rgb=clamp(1-line, 0, 1)/20;
    vec4 color = vec4(rgb, rgb, rgb, 1.0) + vec4(0, 0.3, 0, 1.0);
    return color;
}

float linear_depth(vec4 ppos) {
    float d = (ppos.z / ppos.w) * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - d * (far - near)) / far;
}

float calc_t(vec2 a, vec2 b, vec2 p) {
  vec2 dir = b - a;
  float t = dot(p - a, dir) / dot(dir, dir);
  return clamp(t, 0.0, 1.0);
}

float dist_to_line(vec2 a, vec2 b, vec2 p) {
  vec2 dir = b - a;
  vec2 norm = vec2(-dir.y, dir.x);
  return dot(normalize(norm), a - p);
}

float dist_to_bezier2(vec2 p0, vec2 p1, vec2 p2, float t, vec2 p) {
  vec2 q0 = mix(p0, p1, t);
  vec2 q1 = mix(p1, p2, t);
  return dist_to_line(q0, q1, p);
}

#define UDIST_BIAS 0.001

void process_bezier2(vec2 p, uint i, inout float min_udist, inout float v) {

  vec2 p0 = point_buffer.points[i];
  vec2 p1 = point_buffer.points[i + 1];
  vec2 p2 = point_buffer.points[i + 2];

  float t = calc_t(p0, p2, p);
  float udist = distance(mix(p0, p2, t), p);

  if (udist <= min_udist + UDIST_BIAS) {
    float bez = dist_to_bezier2(p0, p1, p2, t, p);

    if (udist >= min_udist - UDIST_BIAS) {
      vec2 prevp = point_buffer.points[i - 2];
      float prevd = dist_to_line(p0, p2, prevp);
      v = mix(min(bez, v), max(bez, v), step(prevd, 0.0));
    }
    else {
      v = bez;
    }
    min_udist = min(min_udist, udist);
  }
}

void process_bezier2_loop(vec2 p,
                          uint begin, uint end,
                          inout float min_udist,
                          inout float v) {

  for (uint i = begin; i < end; i += 2) {
    process_bezier2(p, i, min_udist, v);
  }
}

float cell_signed_dist(uint point_offset, uint cell, vec2 p) {

  float min_udist = 1.0 / 0.0;
  float v = -1.0 / 0.0;

  uvec3 vcell = uvec3(cell, cell, cell);
  uvec3 len = (vcell >> uvec3(0, 2, 5)) & uvec3(3, 7, 7);
  uvec3 begin = point_offset + ((vcell >> uvec3(8, 16, 24)) & 0xFF) * 2;
  uvec3 end = begin + len * 2;

  process_bezier2_loop(p, begin.x, end.x, min_udist, v);
  process_bezier2_loop(p, begin.y, end.y, min_udist, v);
  process_bezier2_loop(p, begin.z, end.z, min_udist, v);

  return v;
}

void main() {

  if(in_phase == 0){ // ground plane

    float t = nearPoint.y / (nearPoint.y - farPoint.y);
    vec3 pos = nearPoint + t * (farPoint - nearPoint);
    vec4 ppos = proj * view * vec4(pos.xyz, 1.0);

    gl_FragDepth = ppos.z / ppos.w;

    out_color = grid_colour(pos, 17) * float(t > 0);

    //out_color.a *= 0.4 + 0.6 * max(0, (0.5 - linear_depth(ppos)));
  }
  else
  if(in_phase == 1){ // panel

    const vec3 lightDir= vec3(-0.4, 0.6, 0.9);
    vec3 dX = dFdx(in_frag_pos.xyz);
    vec3 dY = dFdy(in_frag_pos.xyz);
    vec3 normal = normalize(cross(dX,dY));
    float light = max(0.0, dot(lightDir, normal));
    out_color = light * texture(tex, in_texture_coord.xy) + vec4(0.4, 0.4, 0.4, 1.0);
    gl_FragDepth = in_frag_pos.z / in_frag_pos.w;
  }
  else
  if(in_phase == 2){ // text

    uvec2 c = min(uvec2(in_cell_coord), in_cell_info.zw - 1);
    uint cell_index = in_cell_info.y + in_cell_info.z * c.y + c.x;
    uint cell = cell_buffer.cells[cell_index];
   
    float v = cell_signed_dist(in_cell_info.x, cell, in_glyph_pos);
    float alpha = clamp(v * in_sharpness + 0.5, 0.0, 1.0);

    out_color = vec4(0.0, 0.0, 0.0, 0.8*alpha);
    gl_FragDepth = in_frag_pos.z / in_frag_pos.w;
  }
}





