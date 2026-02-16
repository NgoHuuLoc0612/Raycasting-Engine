#ifndef ADVANCED_FEATURES_H
#define ADVANCED_FEATURES_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

// Forward declarations
typedef struct Engine Engine;
typedef struct Vec3 Vec3;
typedef struct ColorF ColorF;

// =============================================================================
// THREADING SYSTEM
// =============================================================================

#define MAX_THREADS 4

// Render job for multithreading
typedef struct {
    int start_column;
    int end_column;
    Engine* engine;
    bool completed;
} RenderJob;

// Thread pool for parallel rendering
typedef struct {
    bool use_threading;
    int job_count;
    RenderJob jobs[MAX_THREADS];
    void* thread_handles[MAX_THREADS];
} ThreadPool;

// Threading functions
void threading_init(ThreadPool* pool);
void threading_cleanup(ThreadPool* pool);
void* threading_render_job(void* arg);
void threading_render_parallel(Engine* engine);

// =============================================================================
// PHYSICALLY-BASED RENDERING (PBR)
// =============================================================================

// PBR Material properties
typedef struct {
    ColorF albedo;
    float metallic;
    float roughness;
    float ao;
    float emissive_strength;
    
    // Texture map IDs
    int albedo_map;
    int normal_map;
    int metallic_map;
    int roughness_map;
    int ao_map;
    int emissive_map;
} PBRMaterial;

// PBR functions
void pbr_init_material(PBRMaterial* mat);
ColorF pbr_calculate_lighting(PBRMaterial* mat, Vec3 normal, Vec3 view_dir, 
                              Vec3 light_dir, ColorF light_color);
float pbr_distribution_ggx(Vec3 normal, Vec3 halfway, float roughness);
float pbr_geometry_smith(Vec3 normal, Vec3 view_dir, Vec3 light_dir, float roughness);
Vec3 pbr_fresnel_schlick(float cos_theta, Vec3 f0);

// =============================================================================
// GLOBAL ILLUMINATION (GI)
// =============================================================================

#define IRRADIANCE_PROBES 64

// Irradiance probe for global illumination
typedef struct {
    Vec3 position;
    ColorF irradiance[6];  // 6 directions: +X, -X, +Y, -Y, +Z, -Z
    float influence_radius;
    bool needs_update;
} IrradianceProbe;

// GI functions
void gi_init_probes(Engine* engine);
void gi_update_probe(Engine* engine, IrradianceProbe* probe);
ColorF gi_sample_irradiance(Engine* engine, Vec3 position, Vec3 normal);
void gi_propagate_light(Engine* engine);
ColorF pbr_image_based_lighting(PBRMaterial* mat, Vec3 normal, Vec3 view_dir, 
                                IrradianceProbe* probe);

// =============================================================================
// AUDIO SYSTEM
// =============================================================================

#define MAX_AUDIO_SOURCES 32
#define MAX_AUDIO_BUFFERS 32

// Audio source (3D positional audio)
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

// Audio functions
void audio_init(Engine* engine);
void audio_cleanup(Engine* engine);
void audio_update(Engine* engine);
int audio_load_sound(const char* filename);
void audio_play(AudioSource* source);
void audio_stop(AudioSource* source);
void audio_set_listener(Vec3 position, Vec3 forward, Vec3 up);
void audio_update_3d(AudioSource* source, Vec3 listener_pos);

// =============================================================================
// SCRIPTING SYSTEM
// =============================================================================

#define MAX_SCRIPTS 64

// Script value types
typedef enum {
    SCRIPT_TYPE_NULL,
    SCRIPT_TYPE_NUMBER,
    SCRIPT_TYPE_STRING,
    SCRIPT_TYPE_BOOL,
    SCRIPT_TYPE_VECTOR3
} ScriptValueType;

// Script value container
typedef struct {
    ScriptValueType type;
    union {
        float number;
        char* string;
        bool boolean;
        Vec3 vector;
    } data;
} ScriptValue;

// Script function pointer
typedef ScriptValue (*ScriptFunction)(Engine* engine, ScriptValue* args, int arg_count);

// Script property
typedef struct {
    char name[64];
    ScriptValueType type;
    ScriptValue value;
} ScriptProperty;

// Script instance
typedef struct {
    char name[64];
    bool active;
    
    // Script callbacks
    ScriptFunction update;
    ScriptFunction on_collision;
    ScriptFunction on_trigger;
    
    // Script properties
    ScriptProperty properties[32];
    int property_count;
} Script;

// Scripting functions
void script_init(Engine* engine);
void script_cleanup(Engine* engine);
void script_load(Engine* engine, const char* filename);
void script_update_all(Engine* engine);
ScriptValue script_call_function(Script* script, const char* func_name, 
                                 Engine* engine, ScriptValue* args, int arg_count);
void script_register_function(const char* name, ScriptFunction func);

// Script value creation helpers
ScriptValue script_create_value_number(float value);
ScriptValue script_create_value_vector(Vec3 value);
ScriptValue script_create_value_bool(bool value);

// Example script functions
ScriptValue script_example_on_update(Engine* engine, ScriptValue* args, int arg_count);
ScriptValue script_example_on_collision(Engine* engine, ScriptValue* args, int arg_count);

// =============================================================================
// COMPUTE SHADER ACCELERATION
// =============================================================================

// Compute context for GPU-accelerated operations
typedef struct {
    bool use_compute;
    int buffer_size;
    uint32_t* input_buffer;
    uint32_t* output_buffer;
} ComputeContext;

// Compute functions
void compute_init(ComputeContext* ctx, int buffer_size);
void compute_cleanup(ComputeContext* ctx);
void compute_dispatch_post_process(ComputeContext* ctx, uint32_t* input, 
                                   uint32_t* output, int width, int height);
void compute_dispatch_lighting(ComputeContext* ctx, Engine* engine);
void compute_dispatch_post_process_simd(ComputeContext* ctx, uint32_t* input,
                                       uint32_t* output, int width, int height);

#endif // ADVANCED_FEATURES_H