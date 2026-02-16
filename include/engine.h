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

// Advanced features configuration
#define MAX_THREADS 4
#define IRRADIANCE_PROBES 64
#define MAX_AUDIO_SOURCES 32
#define MAX_SCRIPTS 64

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

// =============================================================================
// ADVANCED FEATURES - TYPE DEFINITIONS
// =============================================================================

// --- Threading ---
typedef struct {
    int start_column;
    int end_column;
    struct Engine* engine;
    bool completed;
} RenderJob;

typedef struct {
    bool use_threading;
    int job_count;
    RenderJob jobs[MAX_THREADS];
    void* thread_handles[MAX_THREADS];
} ThreadPool;

// --- PBR (Physically-Based Rendering) ---
typedef struct {
    ColorF albedo;
    float metallic;
    float roughness;
    float ao;
    float emissive_strength;
    int albedo_map;
    int normal_map;
    int metallic_map;
    int roughness_map;
    int ao_map;
    int emissive_map;
} PBRMaterial;

// --- Global Illumination ---
typedef struct {
    Vec3 position;
    ColorF irradiance[6];
    float influence_radius;
    bool needs_update;
} IrradianceProbe;

// --- Audio System ---
typedef struct {
    Vec3 position;
    float volume;
    float pitch;
    float max_distance;
    float rolloff_factor;
    bool looping;
    bool playing;
    bool positional;
    int audio_buffer_id;
    float playback_position;
} AudioSource;

// --- Scripting System ---
typedef enum {
    SCRIPT_TYPE_NULL,
    SCRIPT_TYPE_NUMBER,
    SCRIPT_TYPE_STRING,
    SCRIPT_TYPE_BOOL,
    SCRIPT_TYPE_VECTOR3
} ScriptValueType;

typedef struct ScriptValue {
    ScriptValueType type;
    union {
        float number;
        char* string;
        bool boolean;
        Vec3 vector;
    } data;
} ScriptValue;

typedef ScriptValue (*ScriptFunction)(struct Engine* engine, ScriptValue* args, int arg_count);

typedef struct {
    char name[64];
    ScriptValueType type;
    union {
        float number;
        char* string;
        bool boolean;
        Vec3 vector;
    } data;
} ScriptProperty;

typedef struct {
    char name[64];
    bool active;
    ScriptFunction update;
    ScriptFunction on_collision;
    ScriptFunction on_trigger;
    ScriptProperty properties[32];
    int property_count;
} Script;

// --- Compute Shader Acceleration ---
typedef struct {
    bool use_compute;
    int buffer_size;
    uint32_t* input_buffer;
    uint32_t* output_buffer;
} ComputeContext;

// =============================================================================
// MAIN ENGINE STATE
// =============================================================================

typedef struct Engine {
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
    
    // Advanced features
    bool use_multithreading;
    ThreadPool thread_pool;
    
    bool use_gi;
    IrradianceProbe gi_probes[IRRADIANCE_PROBES];
    int probe_count;
    
    AudioSource audio_sources[MAX_AUDIO_SOURCES];
    int audio_source_count;
    
    Script scripts[MAX_SCRIPTS];
    int script_count;
    
    ComputeContext compute_ctx;
} Engine;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

// Core engine functions
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

// =============================================================================
// ADVANCED FEATURES - FUNCTION DECLARATIONS
// =============================================================================

// Threading
void threading_init(ThreadPool* pool);
void threading_cleanup(ThreadPool* pool);
void* threading_render_job(void* arg);
void threading_render_parallel(Engine* engine);

// PBR
void pbr_init_material(PBRMaterial* mat);
ColorF pbr_calculate_lighting(PBRMaterial* mat, Vec3 normal, Vec3 view_dir, 
                              Vec3 light_dir, ColorF light_color);
float pbr_distribution_ggx(Vec3 normal, Vec3 halfway, float roughness);
float pbr_geometry_smith(Vec3 normal, Vec3 view_dir, Vec3 light_dir, float roughness);
Vec3 pbr_fresnel_schlick(float cos_theta, Vec3 f0);
ColorF pbr_image_based_lighting(PBRMaterial* mat, Vec3 normal, Vec3 view_dir, 
                                IrradianceProbe* probe);

// Global Illumination
void gi_init_probes(Engine* engine);
void gi_update_probe(Engine* engine, IrradianceProbe* probe);
ColorF gi_sample_irradiance(Engine* engine, Vec3 position, Vec3 normal);
void gi_propagate_light(Engine* engine);

// Audio
void audio_init(Engine* engine);
void audio_cleanup(Engine* engine);
void audio_update(Engine* engine);
int audio_load_sound(const char* filename);
void audio_play(AudioSource* source);
void audio_stop(AudioSource* source);
void audio_set_listener(Vec3 position, Vec3 forward, Vec3 up);
void audio_update_3d(AudioSource* source, Vec3 listener_pos);

// Scripting
void script_init(Engine* engine);
void script_cleanup(Engine* engine);
void script_load(Engine* engine, const char* filename);
void script_update_all(Engine* engine);
ScriptValue script_call_function(Script* script, const char* func_name, 
                                 Engine* engine, ScriptValue* args, int arg_count);
void script_register_function(const char* name, ScriptFunction func);
ScriptValue script_create_value_number(float value);
ScriptValue script_create_value_vector(Vec3 value);
ScriptValue script_create_value_bool(bool value);
ScriptValue script_example_on_update(Engine* engine, ScriptValue* args, int arg_count);
ScriptValue script_example_on_collision(Engine* engine, ScriptValue* args, int arg_count);

// Compute Shader Acceleration
void compute_init(ComputeContext* ctx, int buffer_size);
void compute_cleanup(ComputeContext* ctx);
void compute_dispatch_post_process(ComputeContext* ctx, uint32_t* input, 
                                   uint32_t* output, int width, int height);
void compute_dispatch_lighting(ComputeContext* ctx, Engine* engine);
void compute_dispatch_post_process_simd(ComputeContext* ctx, uint32_t* input,
                                       uint32_t* output, int width, int height);

#endif // ENGINE_H
