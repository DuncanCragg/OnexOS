#version 450
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D tex;

struct sub_object {
  vec3  position;
  vec3  rotation;
  ivec4 obj_index;
};

struct scene_object {
  vec3 shape;
  vec3 bb_position;
  vec3 bb_shape;
  sub_object subs[32];
};

layout(std430, binding = 2) buffer buf4 {
  scene_object objects[];
} objects_buf;

layout(location = 0) in  vec2  left_touch;
layout(location = 1) in  vec2  overlay_uv;
layout(location = 2) in  mat4  inv_view;
layout(location = 6) in  mat4  inv_proj;

layout(location = 0) out vec4  color;

// ---------------------------------------------------

const ivec3 GROUND_PLANE = ivec3(-1);
const ivec3 NO_OBJECT    = ivec3(-2);
const float FAR_FAR_AWAY = 1e6;

// ---------------------------------------------------

float dist_to_ground(vec3 ro, vec3 rd) {
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

// ---------------------------------------------------

float sdf_sphere_near(vec3 p, vec3 pos, float r) {
  return length(p - pos) - r;
}

float sdf_sphere_cast(vec3 p, vec3 rd, vec3 pos, float r) {
  vec3 oc = p - pos;
  float b = dot(oc, rd);
  float c = dot(oc, oc) - r * r;
  return b * b - c;
}

// ---------------------------------------------------

float sdf_cuboid_near(vec3 p, vec3 pos, vec3 cube_shape) {
  vec3 d = abs(p - pos) - cube_shape;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
}

int number_of_cuboids_cast;

float sdf_cuboid_cast(vec3 p, vec3 rd, vec3 pos, vec3 cube_shape) {

    number_of_cuboids_cast++;

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

    if (t_near > t_far || t_far < 0.0) return FAR_FAR_AWAY;

    return (t_near >= 0.0 ? t_near : t_far);
}

// ---------------------------------------------------

float sdf_glyph_near(vec3 p, vec3 pos, vec2 shape){
  vec3 local_p = p - pos;
  vec2 glyph_pos_2d = local_p.xy;
  vec2 d = abs(glyph_pos_2d) - shape * 0.5;
  float dist_2d = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
  float dist_z = abs(local_p.z);
  return max(dist_2d, dist_z);
}

float sdf_glyph_cast(vec3 p, vec3 rd, vec3 pos, vec2 shape) {

  vec3 local_p = p - pos;
  float t_near = -(local_p.z / rd.z);
  vec3 intersect_p = local_p + t_near * rd;
  vec2 d = abs(intersect_p.xy) - shape * 0.5;

  if (max(d.x, d.y) > 0.0 || t_near < 0.0) return FAR_FAR_AWAY;

  float circle_radius = shape.x * 0.5;
  float dist_to_center = length(intersect_p.xy);

  if (dist_to_center <= circle_radius) return t_near * 0.999;
  return t_near + (dist_to_center - circle_radius);
}

// ---------------------------------------------------

ivec3 object_index;
float object_dist;

float scene_sdf(vec3 p) {

    vec3 position = objects_buf.objects[object_index.x].subs[object_index.y].position;
    vec3 shape    = objects_buf.objects[object_index.z].shape;

    return sdf_cuboid_near(p, position, shape);
}

// ---------------------------------------------------

void get_nearest_object(vec3 ro, ivec3 current_object_index) {

  object_index = NO_OBJECT;
  object_dist = FAR_FAR_AWAY;

  float ss=1e6;
  int parent;
  for(int p=0; true; p++){

    int par = objects_buf.objects[0].subs[p].obj_index.x;

  ; if(par == 0) break;

    float s = sdf_cuboid_near(ro, objects_buf.objects[par].bb_position,
                                  objects_buf.objects[par].bb_shape);
    if(s < ss){
      parent = par;
      ss = s;
    }
  }

  for(int n = 0; true; n++){

    vec3 position = objects_buf.objects[parent].subs[n].position;
    int  o        = objects_buf.objects[parent].subs[n].obj_index.x;
    vec3 shape    = objects_buf.objects[o].shape;

  ; if(o==0) break;

    if(ivec3(parent,n,o)==current_object_index) continue;

    float s = sdf_cuboid_near(ro, position, shape);

    if(s<object_dist){

      object_index = ivec3(parent,n,o);
      object_dist = s;
    }
  }
}

void get_first_object_hit(vec3 ro, vec3 rd) {

  number_of_cuboids_cast=0;

  object_index = NO_OBJECT;
  object_dist = FAR_FAR_AWAY;

  for(int p=0; true; p++){

    int parent = objects_buf.objects[0].subs[p].obj_index.x;

  ; if(parent == 0) break;

    float s = sdf_cuboid_cast(ro, rd, objects_buf.objects[parent].bb_position,
                                      objects_buf.objects[parent].bb_shape);
  ; if(s==FAR_FAR_AWAY) continue;

    bool show_bbs=false;
    if(show_bbs){
      if(s<object_dist){
        object_index = ivec3(0,p,parent);
        object_dist = s;
      }
    }

    for(int n = 0; true; n++){

      vec3 position = objects_buf.objects[parent].subs[n].position;
      int  o        = objects_buf.objects[parent].subs[n].obj_index.x;
      vec3 shape    = objects_buf.objects[o].shape;

    ; if(o==0) break;

      float s = sdf_cuboid_cast(ro, rd, position, shape);

      if(s<object_dist){

        object_index = ivec3(parent,n,o);
        object_dist = s;
      }
    }
  }
}

// ---------------------------------------------------

vec3 calc_normal_trad(vec3 p) {

  float e = 0.001;
  return normalize(vec3(

    scene_sdf(p+vec3(e, 0.0, 0.0)) -
    scene_sdf(p-vec3(e, 0.0, 0.0)),

    scene_sdf(p+vec3(0.0, e, 0.0)) -
    scene_sdf(p-vec3(0.0, e, 0.0)),

    scene_sdf(p+vec3(0.0, 0.0, e)) -
    scene_sdf(p-vec3(0.0, 0.0, e))
  ));
}

vec3 calc_normal(vec3 p) {

  vec2 e = vec2(.01, 0);
  return normalize(scene_sdf(p      ) -
              vec3(scene_sdf(p-e.xyy),
                   scene_sdf(p-e.yxy),
                   scene_sdf(p-e.yyx)));
}

// ---------------------------------------------------

const int RAY_MARCH_ITERATIONS = 30;

float ray_march(vec3 ro, vec3 rd) {

  float pd = dist_to_ground(ro, rd);

  get_nearest_object(ro, NO_OBJECT);

  float dist = 0.0;

  for (int i = 0; i < RAY_MARCH_ITERATIONS; i++) {

    vec3 p = ro + rd * dist;
    float d = scene_sdf(p);

    if (pd > 0.0 && pd < d) return pd;

    if (d < 0.001) return dist;

    dist += d;
  }
  return pd;
}

void ray_cast(vec3 ro, vec3 rd) {

  float pd = dist_to_ground(ro, rd);

  get_first_object_hit(ro, rd);

  bool hits_ground    = (pd > 0.0);
  bool is_object      = (object_index != NO_OBJECT);
  bool ground_nearest = hits_ground && (!is_object || (is_object && (object_dist > pd)));

  if(ground_nearest){
    object_index = GROUND_PLANE;
    object_dist = pd;
  }
}

// ---------------------------------------------------

const float AO_RADIUS = 0.1;

float ambient_occlusion(vec3 ro) {

  get_nearest_object(ro, object_index);

  return smoothstep(0.0, AO_RADIUS, object_dist);
}

float soft_shadows(vec3 ro, vec3 rd, float hardness) {

  get_nearest_object(ro, object_index);

  float r = 1.0;
  float d = 0.0;
  for(int i = 0; i < 10; i++) {

      vec3 p = ro + rd * d;
      float h = scene_sdf(p);
      if(h < 0.001) return 0.0;
      d += h;
      if(d > 5.0) return r;

      r = min(r, hardness * h / d );
  }
  return r;
}

// ---------------------------------------------------

vec2 uv_from_p_on_obj(vec3 p){
  vec3 position = objects_buf.objects[object_index.x].subs[object_index.y].position;
  vec3 shape    = objects_buf.objects[object_index.z].shape;
  vec3 local_p = p - position;
  vec2 norm_p = local_p.xy / shape.xy;
  return ((norm_p + 1.0) / 2.0) * vec2(1.0,-1.0) + vec2(0,1);
}

// ---------------------------------------------------

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

    ray_cast(ro, rd);

    bool do_ao_ground  = true;
    bool do_ao_objects = true;
    bool do_shadows    = false;

    float ss = 0.2;

    if (object_index != NO_OBJECT) {

      vec3 p = ro + rd * object_dist * 0.999;

      vec3 light_pos = vec3(30.0, 30.0, -30.0);
      vec3 light_dir = normalize(light_pos - p);

      if(object_index == GROUND_PLANE){

        float shadows = (1.0-ss)+ss*(do_ao_ground? ambient_occlusion(p):
                                    (do_shadows? soft_shadows(p, light_dir, 16.0): 1.0));
        color = vec4(grid_pattern(p) * shadows, 1.0);

      } else {

        bool show_number_of_cuboids_cast = false;
        if(!show_number_of_cuboids_cast){

          vec3 light_col = vec3(1.0);
          vec4 ambient_col = vec4(0.6, 0.6, 0.6, 1.0);
          if(object_index.x==0) ambient_col = vec4(0.1,0.1,0.8,1.0);
          vec3 normal = calc_normal(p);
          float shadows = (1.0-ss)+ss*(do_ao_objects? ambient_occlusion(p):
                                      (do_shadows? soft_shadows(p, light_dir, 16.0): 1.0));
          float diffuse = max(dot(normal, light_dir), 0.0);
          vec2 texuv = uv_from_p_on_obj(p);

          color = (ambient_col+0.5*diffuse*texture(tex, texuv))*vec4(vec3(shadows),1.0);

        } else {

          color = vec4(float(number_of_cuboids_cast)/72, 0.0, 0.0, 1.0);
        }
      }

      if(in_left_touch_area) color -= vec4(0.1);

      gl_FragDepth = -1.0;

    } else {

      if(in_left_touch_area) color = vec4(0.1);
      else discard;
    }
}

// ------------------------------



