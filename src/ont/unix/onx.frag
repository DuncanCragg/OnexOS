#version 450
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D tex;

struct cuboid {
  vec3 position;
  vec3 shape;
};

layout(std430, binding = 2) buffer buf4 {
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

vec3 glph1_pos = objects_buf.cuboids[1].position +
                 objects_buf.cuboids[1].shape * vec3(0,0,-1.00001);
vec2 glph1_shape = vec2(0.5, 0.5);

float plane_height = 0.0;

// ---------------------------------------------------

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

float sdf_cuboid_true(vec3 p, vec3 pos, vec3 cube_shape) {
  vec3 d = abs(p - pos) - cube_shape;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

float sdf_cuboid(vec3 p, vec3 rd, vec3 pos, vec3 cube_shape, bool fast) {

    if(!fast) return sdf_cuboid_true(p, pos, cube_shape);

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

float sdf_glyph_true(vec3 p, vec3 rd, vec3 pos, vec2 shape){
  vec3 local_p = p - pos;
  vec2 glyph_pos_2d = local_p.xy;
  vec2 d = abs(glyph_pos_2d) - shape * 0.5;
  float dist_2d = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
  float dist_z = abs(local_p.z);
  return max(dist_2d, dist_z);
}

float sdf_glyph(vec3 p, vec3 rd, vec3 pos, vec2 shape, bool fast) {

  if (!fast) return sdf_glyph_true(p, rd, pos, shape);

  vec3 local_p = p - pos;
  float t_near = -(local_p.z / rd.z);
  vec3 intersect_p = local_p + t_near * rd;
  vec2 d = abs(intersect_p.xy) - shape * 0.5;

  if (max(d.x, d.y) > 0.0 || t_near < 0.0) return 1e6;

  float circle_radius = shape.x * 0.5;
  float dist_to_center = length(intersect_p.xy);

  if (dist_to_center <= circle_radius) return t_near * 0.999;
  return t_near + (dist_to_center - circle_radius);
}

bool ray_hits_sphere(vec3 ro, vec3 rd, vec3 center, float radius) {
  vec3 oc = ro - center;
  float b = dot(oc, rd);
  float c = dot(oc, oc) - radius * radius;
  float h = b * b - c;
  return h >= 0.0;
}

const int NUM_OBJECTS = 10;

int narrow_objects(vec3 ro, vec3 rd, out bool objects[NUM_OBJECTS], bool really) {

  for (int i = 0; i < NUM_OBJECTS; i++) objects[i] = !really;
  if(!really) return NUM_OBJECTS;

  objects[1]=ray_hits_sphere(ro, rd, cube1_pos, max_cube_radius(cube1_shape));
  objects[2]=ray_hits_sphere(ro, rd, cube2_pos, max_cube_radius(cube2_shape));
  objects[3]=ray_hits_sphere(ro, rd, sphr1_pos, sphr1_radius);
  objects[4]=ray_hits_sphere(ro, rd, sphr2_pos, sphr2_radius);
  objects[5]=ray_hits_sphere(ro, rd, cube3_pos, max_cube_radius(cube3_shape));
  objects[6]=ray_hits_sphere(ro, rd, cube4_pos, max_cube_radius(cube4_shape));
  objects[7]=ray_hits_sphere(ro, rd, glph1_pos, max_cube_radius(vec3(glph1_shape,0.0)));

  int num_to_scan = 0;
  for (int i = 0; i < NUM_OBJECTS; i++){
    if(objects[i]){
      num_to_scan++;
      if(i==3 || i==4) num_to_scan++;
    }
  }
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
  if (objects[7]) {
    s = sdf_glyph(p, rd, glph1_pos, glph1_shape, fast);
    if(s<d){ d=s; n=7; }
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

float soft_shadows(vec3 ro, vec3 rd, float hardness) {

  bool objects[NUM_OBJECTS];
  int num_to_scan = narrow_objects(ro, rd, objects, true);
  if(num_to_scan == 0) return 1.0;

  float r = 1.0;
  float d = 0.0;
  for(int i = 0; i < 10; i++) {

      vec3 p = ro + rd * d;
      float h = scene_sdf_fine(p, rd, objects);
      if(h < 0.001) return 0.0;
      d += h;
      if(d > 5.0) return r;

      r = min(r, hardness * h / d );
  }
  return r;
}

const int MANY_HOPS      = 64;
const int SINGLE_HOP_ISH =  3;

vec2 ray_march(vec3 ro, vec3 rd) {

  float pd = ray_plane_intersection(ro, rd);

  bool objects[NUM_OBJECTS];
  int num_to_scan = narrow_objects(ro, rd, objects, true);
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

   if(false){

    float r = 0.1;
    float d = distance(overlay_uv, left_touch);

    if (d <= r) {
        color = vec4(0.7, 0.7, 0.7, 0.2);
        gl_FragDepth = -1.0;
    } else {
        discard;
    }

   } else {

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

        float shadows = 0.8 + 0.2 * soft_shadows(p, light_dir, 16.0);
        color = vec4(grid_pattern(p) * shadows, 1.0);

      } else
      if(obj_index == 7){

        color = vec4(0,0,0,1);

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

// ------------------------------



