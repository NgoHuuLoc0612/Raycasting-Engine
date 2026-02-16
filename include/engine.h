#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Configuration constants
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define FOV 60.0
#define MAX_RENDER_DISTANCE 50.0
#define TEXTURE_SIZE 64
#define MAX_TEXTURES 32
#define MAP_WIDTH 64
#define MAP_HEIGHT 64
#define SHADOW_MAP_SIZE 512
#define MAX_LIGHTS 16
#define MAX_SPRITES 256
#define MAX_PARTICLES 2048
#define PHYSICS_SUBSTEPS 4

// Vector and matrix structures
typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float m[4][4];
} Mat4;

typedef struct {
    float x, y, z, w;
} Quaternion;

// Color structures
typedef struct {
    uint8_t r, g, b, a;
} Color;

typedef struct {
    float r, g, b, a;
} ColorF;

// Texture data
typedef struct {
    uint32_t* pixels;
    int width, height;
    bool has_alpha;
    uint8_t* normal_map;
    uint8_t* specular_map;
    uint8_t* emission_map;
} Texture;

// Dynamic lighting
typedef struct {
    Vec3 position;
    ColorF color;
    float intensity;
    float radius;
    bool cast_shadows;
    float flickering;
} Light;

// Sprite system
typedef struct {
    Vec2 position;
    float z_height;
    int texture_id;
    Vec2 scale;
    float rotation;
    bool billboarding;
    ColorF tint;
    bool cast_shadow;
    int animation_frame;
    float animation_speed;
} Sprite;

// Particle system
typedef struct {
    Vec3 position;
    Vec3 velocity;
    ColorF color;
    float lifetime;
    float size;
    float gravity_scale;
    int texture_id;
} Particle;

// Physics collision
typedef struct {
    Vec2 position;
    Vec2 velocity;
    float radius;
    float friction;
    float bounce;
    bool affected_by_gravity;
} PhysicsBody;

// Ray data structure
typedef struct {
    Vec2 origin;
    Vec2 direction;
    float distance;
    int map_x, map_y;
    int side;
    Vec2 hit_point;
    float perpendicular_distance;
    int texture_id;
    float texture_x;
    float z_height;
    bool hit_door;
    int door_state;
} Ray;

// Door system
typedef struct {
    int x, y;
    float open_amount;
    bool is_opening;
    bool is_closing;
    int texture_id;
    bool horizontal;
} Door;

// Fog system
typedef struct {
    ColorF color;
    float density;
    float start_distance;
    float end_distance;
} Fog;

// Player camera
typedef struct {
    Vec2 position;
    Vec2 direction;
    Vec2 plane;
    float pitch;
    float z_position;
    float velocity_z;
    float bob_offset;
    float bob_phase;
    bool crouching;
    PhysicsBody physics;
} Camera;

// World map data
typedef struct {
    int tiles[MAP_HEIGHT][MAP_WIDTH];
    float floor_heights[MAP_HEIGHT][MAP_WIDTH];
    float ceiling_heights[MAP_HEIGHT][MAP_WIDTH];
    int floor_textures[MAP_HEIGHT][MAP_WIDTH];
    int ceiling_textures[MAP_HEIGHT][MAP_WIDTH];
    int wall_textures[MAP_HEIGHT][MAP_WIDTH];
    Door doors[64];
    int door_count;
} WorldMap;

// Render buffers
typedef struct {
    float* z_buffer;
    uint32_t* color_buffer;
    float* shadow_buffer;
    uint8_t* light_buffer;
    uint32_t* post_process_buffer;
} RenderBuffers;

// Post-processing effects
typedef struct {
    bool bloom_enabled;
    float bloom_threshold;
    float bloom_intensity;
    bool motion_blur_enabled;
    float motion_blur_strength;
    bool chromatic_aberration;
    float aberration_strength;
    bool vignette;
    float vignette_intensity;
    bool fxaa_enabled;
    float gamma;
    float exposure;
} PostProcessing;

// Main engine state
typedef struct {
    Camera camera;
    WorldMap world;
    Texture textures[MAX_TEXTURES];
    int texture_count;
    Light lights[MAX_LIGHTS];
    int light_count;
    Sprite sprites[MAX_SPRITES];
    int sprite_count;
    Particle particles[MAX_PARTICLES];
    int particle_count;
    RenderBuffers buffers;
    Fog fog;
    PostProcessing post_fx;
    bool vsync;
    float delta_time;
    uint64_t frame_count;
    float time_accumulator;
} Engine;

// Function declarations
void engine_init(Engine* engine);
void engine_cleanup(Engine* engine);
void engine_update(Engine* engine, float delta_time);
void engine_render(Engine* engine);

// Math utilities
Vec2 vec2_add(Vec2 a, Vec2 b);
Vec2 vec2_sub(Vec2 a, Vec2 b);
Vec2 vec2_mul(Vec2 v, float scalar);
Vec2 vec2_normalize(Vec2 v);
float vec2_dot(Vec2 a, Vec2 b);
float vec2_length(Vec2 v);
Vec2 vec2_rotate(Vec2 v, float angle);

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_mul(Vec3 v, float scalar);
Vec3 vec3_normalize(Vec3 v);
float vec3_dot(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);
float vec3_length(Vec3 v);

Mat4 mat4_identity(void);
Mat4 mat4_perspective(float fov, float aspect, float near, float far);
Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up);
Mat4 mat4_multiply(Mat4 a, Mat4 b);
Vec3 mat4_transform_vec3(Mat4 m, Vec3 v);

Quaternion quat_from_euler(float pitch, float yaw, float roll);
Quaternion quat_multiply(Quaternion a, Quaternion b);
Vec3 quat_rotate_vec3(Quaternion q, Vec3 v);

// Raycasting core algorithms
void raycast_dda(Engine* engine, int x, Ray* ray);
void raycast_floor_ceiling(Engine* engine, int y, int x);
void raycast_vertical_line(Engine* engine, int x, Ray* ray);

// Advanced rendering
void render_textured_wall(Engine* engine, int x, Ray* ray);
void render_transparent_surfaces(Engine* engine);
void render_sprites(Engine* engine);
void render_particles(Engine* engine);
void apply_lighting(Engine* engine);
void apply_shadows(Engine* engine);
void apply_fog(Engine* engine);

// Post-processing pipeline
void post_process_bloom(Engine* engine);
void post_process_motion_blur(Engine* engine);
void post_process_chromatic_aberration(Engine* engine);
void post_process_fxaa(Engine* engine);
void post_process_tone_mapping(Engine* engine);
void post_process_vignette(Engine* engine);

// Physics and collision
bool physics_check_collision(Engine* engine, Vec2 position, float radius);
void physics_resolve_collision(PhysicsBody* body, Vec2 normal, float penetration);
void physics_update(Engine* engine, PhysicsBody* body, float delta_time);
void physics_apply_gravity(PhysicsBody* body, float delta_time);

// Door system
void door_open(Door* door);
void door_close(Door* door);
void door_update(Door* door, float delta_time);
bool door_check_collision(Door* door, Vec2 position);

// Sprite sorting and rendering
void sprite_sort_by_distance(Sprite* sprites, int count, Vec2 camera_pos);
void sprite_render(Engine* engine, Sprite* sprite);
void sprite_animate(Sprite* sprite, float delta_time);

// Particle effects
void particle_emit(Engine* engine, Vec3 position, Vec3 velocity, ColorF color, float lifetime);
void particle_update(Engine* engine, float delta_time);
void particle_render(Engine* engine);

// Texture operations
bool texture_load(Texture* texture, const char* filename);
void texture_generate_mipmaps(Texture* texture);
Color texture_sample(Texture* texture, float u, float v);
Color texture_sample_bilinear(Texture* texture, float u, float v);
Color texture_sample_trilinear(Texture* texture, float u, float v, float mip_level);
Vec3 texture_sample_normal(Texture* texture, float u, float v);

// Lighting calculations
ColorF lighting_calculate_diffuse(Vec3 normal, Vec3 light_dir, ColorF light_color);
ColorF lighting_calculate_specular(Vec3 normal, Vec3 light_dir, Vec3 view_dir, 
                                    ColorF light_color, float shininess);
float lighting_calculate_shadow(Engine* engine, Vec3 position, Vec3 light_pos);
void lighting_apply_point_light(Engine* engine, Light* light);

// Camera controls
void camera_move_forward(Camera* cam, float distance);
void camera_move_backward(Camera* cam, float distance);
void camera_strafe_left(Camera* cam, float distance);
void camera_strafe_right(Camera* cam, float distance);
void camera_rotate(Camera* cam, float angle);
void camera_look_up(Camera* cam, float angle);
void camera_look_down(Camera* cam, float angle);
void camera_crouch(Camera* cam, bool state);
void camera_update_headbob(Camera* cam, float delta_time, bool moving);

// Map utilities
int map_get_tile(WorldMap* map, int x, int y);
void map_set_tile(WorldMap* map, int x, int y, int value);
float map_get_floor_height(WorldMap* map, int x, int y);
float map_get_ceiling_height(WorldMap* map, int x, int y);
void map_load_from_file(WorldMap* map, const char* filename);
void map_generate_procedural(WorldMap* map, uint32_t seed);

// Optimization utilities
void optimize_frustum_culling(Engine* engine);
void optimize_occlusion_culling(Engine* engine);
void optimize_lod_system(Engine* engine);
void optimize_spatial_partitioning(Engine* engine);

// Color blending
Color color_blend_alpha(Color src, Color dst);
Color color_multiply(Color c, float factor);
ColorF colorf_add(ColorF a, ColorF b);
ColorF colorf_multiply(ColorF c, float factor);
uint32_t color_to_uint32(Color c);
Color uint32_to_color(uint32_t c);

// Fast math approximations
float fast_sqrt(float x);
float fast_inv_sqrt(float x);
float fast_sin(float x);
float fast_cos(float x);
float fast_atan2(float y, float x);

// Noise generation
float perlin_noise_2d(float x, float y);
float perlin_noise_3d(float x, float y, float z);
float simplex_noise_2d(float x, float y);

// Performance profiling
typedef struct {
    const char* name;
    uint64_t start_time;
    uint64_t total_time;
    uint32_t call_count;
} ProfileSection;

void profile_begin(ProfileSection* section);
void profile_end(ProfileSection* section);
void profile_reset(ProfileSection* section);
float profile_get_ms(ProfileSection* section);

#endif // ENGINE_H
