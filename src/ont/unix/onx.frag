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

layout(location = 0) in  vec2  left_touch;
layout(location = 1) in  vec2  overlay_uv;
layout(location = 2) in  mat4  inv_view;
layout(location = 6) in  mat4  inv_proj;

layout(location = 0) out vec4  color;

// ---------------------------------------------------

float ray_plane_intersection(vec3 ro, vec3 rd) {
  if(abs(rd.y) > 0.001) {
    float t = -ro.y / rd.y;
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

bool ray_hits_cuboid(vec3 ro, vec3 rd, vec3 pos, vec3 shape) {
  float s = sdf_cuboid(ro, rd, pos, shape, true);
  return s < 1e6;
}

const int NUM_OBJECTS = 32; // !!
bool objects[NUM_OBJECTS];

int narrow_objects(vec3 ro, vec3 rd) {

  int num_to_scan = 0;
  for(int i = 0; i < objects_buf.size; i++){
    objects[i]=ray_hits_cuboid(ro, rd, objects_buf.cuboids[i].position,
                                       objects_buf.cuboids[i].shape);
    if(objects[i]) num_to_scan++;
  }
  return num_to_scan;
}

vec2 scene_sdf(vec3 p, vec3 rd, bool fast) {

  float d = 1e6;
  int   n = -1;

  for(int i = 0; i < objects_buf.size; i++){
    if(objects[i]) {
      float s = sdf_cuboid(p, rd, objects_buf.cuboids[i].position,
                                  objects_buf.cuboids[i].shape, fast);
      if(s<d){ d=s; n=i; }
    }
  }
  return vec2(d,n);
}

float scene_sdf_fine(vec3 p, vec3 rd){
  return scene_sdf(p, rd, false)[0];
}

vec2 scene_sdf_fast(vec3 p, vec3 rd){
  return scene_sdf(p, rd, true);
}

vec3 calc_normal_trad(vec3 p, vec3 rd, int obj_index) {

  for (int i = 0; i < objects_buf.size; i++) objects[i] = false;
  objects[obj_index] = true;

  float e = 0.001;
  return normalize(vec3(

    scene_sdf_fine(p+vec3(e, 0.0, 0.0), rd) -
    scene_sdf_fine(p-vec3(e, 0.0, 0.0), rd),

    scene_sdf_fine(p+vec3(0.0, e, 0.0), rd) -
    scene_sdf_fine(p-vec3(0.0, e, 0.0), rd),

    scene_sdf_fine(p+vec3(0.0, 0.0, e), rd) -
    scene_sdf_fine(p-vec3(0.0, 0.0, e), rd)
  ));
}

vec3 calc_normal(vec3 p, vec3 rd, int obj_index) {

  for (int i = 0; i < objects_buf.size; i++) objects[i] = false;
  objects[obj_index] = true;

  vec2 e = vec2(.01, 0);
  return normalize(scene_sdf_fine(p,       rd) -
              vec3(scene_sdf_fine(p-e.xyy, rd),
                   scene_sdf_fine(p-e.yxy, rd),
                   scene_sdf_fine(p-e.yyx, rd)));
}

float soft_shadows(vec3 ro, vec3 rd, float hardness) {

  int num_to_scan = narrow_objects(ro, rd);
  if(num_to_scan == 0) return 1.0;

  float r = 1.0;
  float d = 0.0;
  for(int i = 0; i < 10; i++) {

      vec3 p = ro + rd * d;
      float h = scene_sdf_fine(p, rd);
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

  int num_to_scan = narrow_objects(ro, rd);
  if(num_to_scan == 0) return vec2(pd, -1);

  float dist = 0.0;

  int iterations = (num_to_scan == 1)? SINGLE_HOP_ISH: MANY_HOPS;

  for (int i = 0; i < iterations; i++) {

    vec3 p = ro + rd * dist;
    vec2 dn = scene_sdf_fast(p, rd);

    if (pd > 0.0 && pd < dn[0]) return vec2(pd, -1);

    if (dn[0] < 0.001) return vec2(dist, dn[1]);

    dist += dn[0];
  }
  return vec2(pd, -1);
}

vec2 uv_from_p_on_obj(vec3 p, int obj_index){
  vec3 obj_pos   = objects_buf.cuboids[obj_index].position;
  vec3 obj_shape = objects_buf.cuboids[obj_index].shape;
  vec3 local_p = p - obj_pos;
  vec2 norm_p = local_p.xy / obj_shape.xy;
  return ((norm_p + 1.0) / 2.0) * vec2(1,-1) + vec2(0,1);
}

const float LEFT_TOUCH_RADIUS = 0.1;

void main() {

    bool in_left_touch_area = distance(overlay_uv, left_touch) <= LEFT_TOUCH_RADIUS;

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

      if(obj_index == -1){

        float shadows = 0.8 + 0.2 * soft_shadows(p, light_dir, 16.0);
        color = vec4(grid_pattern(p) * shadows, 1.0);

      } else {

        vec3 light_col = vec3(1.0);

        vec4 ambient_col = vec4(0.3, 0.3, 0.3, 1.0);

        vec3 normal = calc_normal(p, rd, obj_index);

        float diffuse = max(dot(normal, light_dir), 0.0);

        vec2 texuv = uv_from_p_on_obj(p, obj_index);

        color = ambient_col + diffuse * texture(tex, texuv);
      }

      if(in_left_touch_area) color -= vec4(0.1);

      gl_FragDepth = -1.0;

    } else {

      if(in_left_touch_area) color = vec4(0.1);
      else discard;
    }
}

// ------------------------------



