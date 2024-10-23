#version 450
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 2) buffer buf2 {
  uint cells[];
} cell_buffer;
/*
layout (set = 0, binding = 3) buffer buf3 {
  vec2 points[];
} point_buffer;
*/
layout (set = 0, binding = 3) uniform sampler2D tex;

struct cuboid {
  vec3 position;
  vec3 shape;
};

layout(std430, binding = 4) buffer buf4 {
  int size;
  cuboid cuboids[];
} objects_buf;

layout(location = 0)      in  vec2  glyph_pos;
layout(location = 1) flat in  uvec4 cell_info;
layout(location = 2)      in  float sharpness;
layout(location = 3)      in  vec2  cell_coord;
layout(location = 4)      in  vec4  texture_coord;
layout(location = 5)      in  vec4  model_pos;
layout(location = 6)      in  vec4  proj_pos;
layout(location = 7) flat in  uint  phase;
layout(location = 8)      in  vec2  left_touch;
layout(location = 9)      in  vec2  overlay_uv;
layout(location = 10)     in  float near;
layout(location = 11)     in  float far;
layout(location = 12)     in  vec3  near_point;
layout(location = 13)     in  vec3  far_point;
layout(location = 14)     in  mat4  view;
layout(location = 18)     in  mat4  proj;
layout(location = 22)     in  mat4  inv_view;
layout(location = 26)     in  mat4  inv_proj; /// !!! too many inputs

layout(location = 0)      out vec4  color;

vec4 grid_colour(vec3 pos, float scale) {
    vec2 coord = pos.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float rgb=clamp(1-line, 0, 1)/20;
    vec4 color = vec4(rgb, rgb, rgb, 1.0) + vec4(0, 0.3, 0, 1.0);
    float minimumx = min(derivative.x, 1);
    float minimumz = min(derivative.y, 1);
    if(pos.x > -1.0 * minimumx && pos.x < 1.0 * minimumx) color = vec4(0.2,0.4,0.2,1.0);
    if(pos.z > -1.0 * minimumz && pos.z < 1.0 * minimumz) color = vec4(0.2,0.4,0.2,1.0);
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
/*
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
*/

// ---------------------------------------------------

vec3 cube1_pos = objects_buf.cuboids[0].position;
vec3 cube2_pos = objects_buf.cuboids[1].position;
vec3 cube3_pos = vec3( -3.0,  1.6, -14.0);
vec3 cube4_pos = vec3(  3.0,  1.6, -14.0);

vec3 cube1_shape = objects_buf.cuboids[0].shape;
vec3 cube2_shape = objects_buf.cuboids[1].shape;
vec3 cube3_shape = vec3(1.5, 1.6, 0.10);
vec3 cube4_shape = vec3(1.5, 1.6, 0.10);

vec3 sphr1_pos = vec3( 0.0, 0.5, 0.0);
vec3 sphr2_pos = vec3(-3.0, 5.0, 5.0);

float sphr1_radius = 0.5;
float sphr2_radius = 5.0;

float plane_height = -0.01;

float ray_plane_intersection(vec3 ro, vec3 rd) {
  if(abs(rd.y) > 0.001) {
    float t = (plane_height - ro.y) / rd.y;
    if(t > 0.0) return t;
  }
  return -1.0;
}

vec3 grid_pattern(vec3 position) {
  vec2 uv = position.xz;
  float gridSize = 1.0;
  float grid = step(0.01, mod(uv.x * gridSize, 1.0)) +
               step(0.01, mod(uv.y * gridSize, 1.0));
  return vec3(0.0, 0.6, 0.0) * (1.0 - grid * 0.1);
}

float sdf_sphere(vec3 p, vec3 pos, float r) {
  return length(p - pos) - r;
}

float max_cube_radius(vec3 cube_shape) {

    return sqrt(cube_shape.x * cube_shape.x +
                cube_shape.y * cube_shape.y +
                cube_shape.z * cube_shape.z);
}

float sdf_cube(vec3 p, vec3 pos, vec3 cube_shape) {
  vec3 d = abs(p - pos) - cube_shape;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdf_cuboid(vec3 p, vec3 rd, vec3 pos, vec3 cube_shape, bool fast) {

    if(!fast) return sdf_cube(p, pos, cube_shape);

    vec3 local_p = p - pos;

    vec3 min_corner = -cube_shape;
    vec3 max_corner =  cube_shape;

    vec3 inv_dir = 1.0 / rd;

    vec3 t_min = (min_corner - local_p) * inv_dir;
    vec3 t_max = (max_corner - local_p) * inv_dir;

    vec3 t1 = min(t_min, t_max);
    vec3 t2 = max(t_min, t_max);

    float t_near = max(max(t1.x, t1.y), t1.z);
    float t_far  = min(min(t2.x, t2.y), t2.z);

    if (t_near > t_far || t_far < 0.0) return 1e6;

    return (t_near >= 0.0 ? t_near : t_far) * 0.999;
}

bool ray_hits_sphere(vec3 ro, vec3 rd, vec3 center, float radius) {
  vec3 oc = ro - center;
  float b = dot(oc, rd);
  float c = dot(oc, oc) - radius * radius;
  float h = b * b - c;
  return h >= 0.0;
}

const int NUM_OBJECTS = 10;

int narrow_objects(vec3 ro, vec3 rd, out bool objects[NUM_OBJECTS]) {

  bool skip_this = false;
  for (int i = 0; i < NUM_OBJECTS; i++) objects[i] = skip_this;
  if(skip_this) return NUM_OBJECTS;

  objects[1]=ray_hits_sphere(ro, rd, cube1_pos, max_cube_radius(cube1_shape));
  objects[2]=ray_hits_sphere(ro, rd, cube2_pos, max_cube_radius(cube2_shape));
  objects[3]=ray_hits_sphere(ro, rd, sphr1_pos, sphr1_radius);
  objects[4]=ray_hits_sphere(ro, rd, sphr2_pos, sphr2_radius);
  objects[5]=ray_hits_sphere(ro, rd, cube3_pos, max_cube_radius(cube3_shape));
  objects[6]=ray_hits_sphere(ro, rd, cube4_pos, max_cube_radius(cube4_shape));

  int num_to_scan = 0;
  for (int i = 0; i < NUM_OBJECTS; i++) if(objects[i]) num_to_scan++;
  return num_to_scan;
}

vec2 scene_sdf(vec3 p, vec3 rd, bool objects[NUM_OBJECTS], bool fast) {

  float d = 1e6;
  int   n = -1;

  float s;

  if (objects[1]) {
    s = sdf_cuboid(p, rd, cube1_pos, cube1_shape, fast);
    if(s<d){ d=s; n=1; }
  }
  if (objects[2]) {
    s = sdf_cuboid(p, rd, cube2_pos, cube2_shape, fast);
    if(s<d){ d=s; n=2; }
  }
  if (objects[3]) {
    s = sdf_sphere(p, sphr1_pos, sphr1_radius);
    if(s<d){ d=s; n=3; }
  }
  if (objects[4]) {
    s = sdf_sphere(p, sphr2_pos, sphr2_radius);
    if(s<d){ d=s; n=4; }
  }
  if (objects[5]) {
    s = sdf_cuboid(p, rd, cube3_pos, cube3_shape, fast);
    if(s<d){ d=s; n=5; }
  }
  if (objects[6]) {
    s = sdf_cuboid(p, rd, cube4_pos, cube4_shape, fast);
    if(s<d){ d=s; n=6; }
  }

  return vec2(d,n);
}

float scene_sdf_fine(vec3 p, vec3 rd, bool objects[NUM_OBJECTS]){
  return scene_sdf(p, rd, objects, false)[0];
}

vec2 scene_sdf_fast(vec3 p, vec3 rd, bool objects[NUM_OBJECTS]){
  return scene_sdf(p, rd, objects, true);
}

float sdf_norm(vec3 p, vec3 rd, int obj){

  bool objects[NUM_OBJECTS];
  for (int i = 0; i < NUM_OBJECTS; i++) objects[i] = false;
  objects[obj] = true;

  return scene_sdf_fine(p, rd, objects);
}

vec3 calc_normal_trad(vec3 p, vec3 rd, int obj) {
  float e = 0.001;
  return normalize(vec3(
    sdf_norm(p+vec3(e, 0.0, 0.0), rd, obj)-sdf_norm(p-vec3(e, 0.0, 0.0), rd, obj),
    sdf_norm(p+vec3(0.0, e, 0.0), rd, obj)-sdf_norm(p-vec3(0.0, e, 0.0), rd, obj),
    sdf_norm(p+vec3(0.0, 0.0, e), rd, obj)-sdf_norm(p-vec3(0.0, 0.0, e), rd, obj)
  ));
}

vec3 calc_normal(vec3 p, vec3 rd, int obj) {
  vec2 e = vec2(.01, 0);
  return normalize(sdf_norm(p,       rd, obj) -
              vec3(sdf_norm(p-e.xyy, rd, obj),
                   sdf_norm(p-e.yxy, rd, obj),
                   sdf_norm(p-e.yyx, rd, obj)));
}

float soft_shadows(vec3 ro, vec3 rd, float min_t, float max_t, float k) {

  bool objects[NUM_OBJECTS];
  int num_to_scan = narrow_objects(ro, rd, objects);
  if(num_to_scan == 0) return 1.0;

  float r = 1.0;
  float t = min_t;
  for(int i = 0; i < 50 && t < max_t; i++) {
      vec3 p = ro + rd * t;
      float h = scene_sdf_fine(p, rd, objects);

      if(h < 0.001) return 0.0;

      r = min(r, k*h/t );
      t += h;
  }
  return r;
}

const int MANY_HOPS      = 64;
const int SINGLE_HOP_ISH =  3;

vec2 ray_march(vec3 ro, vec3 rd) {

  float pd = ray_plane_intersection(ro, rd);

  bool objects[NUM_OBJECTS];
  int num_to_scan = narrow_objects(ro, rd, objects);
  if(num_to_scan == 0) return vec2(pd, 0);

  float dist = 0.0;

  int iterations = (num_to_scan == 1)? SINGLE_HOP_ISH: MANY_HOPS;

  for (int i = 0; i < iterations; i++) {

    vec3 p = ro + rd * dist;
    vec2 dn = scene_sdf_fast(p, rd, objects);

    if (pd > 0.0 && pd < dn[0]) return vec2(pd, 0);

    if (dn[0] < 0.001) return vec2(dist, dn[1]);

    dist += dn[0];
  }
  return vec2(pd, 0);
}

void main() {

  if(phase == 0){ // ground plane

    float t = near_point.y / (near_point.y - far_point.y);
    vec3 pos = near_point + t * (far_point - near_point);
    vec4 ppos = proj * view * vec4(pos.xyz, 1.0);

    gl_FragDepth = ppos.z / ppos.w;

    color = grid_colour(pos, 1) * float(t > 0);

    //color.a *= 0.4 + 0.6 * max(0, (0.5 - linear_depth(ppos)));
  }
  else
  if(phase == 1){ // panels

    const vec3 light_dir= vec3(1.0,  1.0, -1.0);
    float light = max(0.8, dot(light_dir, normalize(cross(dFdx(model_pos.xyz),dFdy(model_pos.xyz)))));
    color = light * 0.5 * texture(tex, texture_coord.xy) + vec4(0.5, 0.5, 0.5, 1.0);
    gl_FragDepth = proj_pos.z / proj_pos.w;
  }
  else
  if(phase == 2){ // text

    uvec2 c = min(uvec2(cell_coord), cell_info.zw - 1);
    uint cell_index = cell_info.y + cell_info.z * c.y + c.x;
    uint cell = cell_buffer.cells[cell_index];

    float v = 0.0f; //cell_signed_dist(cell_info.x, cell, glyph_pos);
    float alpha = clamp(v * sharpness / (2.1*proj_pos.z) + 0.5, 0.0, 1.0);

    color = vec4(0.0, 0.0, 0.0, 0.8*alpha);
    gl_FragDepth = proj_pos.z / proj_pos.w;
  }
  else
  if(phase == 3 || phase == 4){ // overlay or SDF

   if(phase == 3){

    float r = 0.1;
    float d = distance(overlay_uv, left_touch);

    if (d <= r) {
        color = vec4(0.7, 0.7, 0.7, 0.2);
        gl_FragDepth = -1.0;
    } else {
        discard;
    }

   } else { // SDF

    vec2 resolution = vec2(1920.0, 1080.0);

    vec2 uv = (gl_FragCoord.xy / resolution.xy) * 2.0 - 1.0;

    vec4 clip_pos  = vec4(uv, -1.0, 1.0);
    vec4 view_pos  = inv_proj * clip_pos; view_pos /= view_pos.w;
    vec4 world_pos = inv_view * vec4(view_pos.xyz, 1.0);

    vec3 ro = vec3(inv_view[3]);
    vec3 rd = normalize(world_pos.xyz - ro);

    vec2 dn = ray_march(ro, rd);
    float dist = dn[0];

    if (dist > 0.0) {

      vec3 p = ro + rd * dist;

      vec3 light_pos = vec3(30.0, 30.0, -30.0);
      vec3 light_dir = normalize(light_pos - p);

      int obj_index = int(dn[1]);

      if(obj_index == 0){

        float shadows = 0.8 + 0.2 * soft_shadows(p, light_dir, 0.1, 5.0, 16.0);
        color = vec4(grid_pattern(p) * shadows, 1.0);

      } else {

        vec3 normal = calc_normal(p, rd, obj_index);

        vec3 light_col = vec3(1.0);
        vec3 ambient_col = vec3(0.3);

        float diffuse = max(dot(normal, light_dir), 0.0);

        color = vec4(ambient_col + light_col * diffuse, 1.0);
      }

      gl_FragDepth = -1.0;

    } else {
      discard;
    }
   }
  }
}





