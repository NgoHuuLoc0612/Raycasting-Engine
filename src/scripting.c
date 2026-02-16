#include "../include/engine.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Script function registry
typedef struct {
    char name[64];
    ScriptFunction function;
} ScriptFunctionRegistry;

static ScriptFunctionRegistry function_registry[256];
static int registry_count = 0;

// Built-in script functions
static ScriptValue builtin_print(Engine* engine, ScriptValue* args, int arg_count) {
    (void)engine;
    for (int i = 0; i < arg_count; i++) {
        switch (args[i].type) {
            case SCRIPT_TYPE_NUMBER:
                printf("%f ", args[i].data.number);
                break;
            case SCRIPT_TYPE_STRING:
                printf("%s ", args[i].data.string);
                break;
            case SCRIPT_TYPE_BOOL:
                printf("%s ", args[i].data.boolean ? "true" : "false");
                break;
            case SCRIPT_TYPE_VECTOR3:
                printf("Vec3(%f, %f, %f) ", 
                       args[i].data.vector.x, 
                       args[i].data.vector.y, 
                       args[i].data.vector.z);
                break;
            default:
                break;
        }
    }
    printf("\n");
    
    ScriptValue result = {SCRIPT_TYPE_NULL};
    return result;
}

static ScriptValue builtin_spawn_particle(Engine* engine, ScriptValue* args, int arg_count) {
    if (arg_count < 2 || args[0].type != SCRIPT_TYPE_VECTOR3) {
        ScriptValue error = {SCRIPT_TYPE_NULL};
        return error;
    }
    
    Vec3 position = args[0].data.vector;
    Vec3 velocity = {0, 0, 5};
    
    if (arg_count > 1 && args[1].type == SCRIPT_TYPE_VECTOR3) {
        velocity = args[1].data.vector;
    }
    
    ColorF color = {1.0f, 0.5f, 0.0f, 1.0f};
    particle_emit(engine, position, velocity, color, 2.0f);
    
    ScriptValue result = {SCRIPT_TYPE_BOOL};
    result.data.boolean = true;
    return result;
}

static ScriptValue builtin_get_player_pos(Engine* engine, ScriptValue* args, int arg_count) {
    (void)args;
    (void)arg_count;
    
    ScriptValue result = {SCRIPT_TYPE_VECTOR3};
    result.data.vector.x = engine->camera.position.x;
    result.data.vector.y = engine->camera.position.y;
    result.data.vector.z = engine->camera.z_position;
    return result;
}

static ScriptValue builtin_play_sound(Engine* engine, ScriptValue* args, int arg_count) {
    if (arg_count < 1 || args[0].type != SCRIPT_TYPE_VECTOR3) {
        ScriptValue error = {SCRIPT_TYPE_NULL};
        return error;
    }
    
    if (engine->audio_source_count >= MAX_AUDIO_SOURCES) {
        ScriptValue error = {SCRIPT_TYPE_NULL};
        return error;
    }
    
    AudioSource* source = &engine->audio_sources[engine->audio_source_count++];
    source->position = args[0].data.vector;
    source->volume = 1.0f;
    source->pitch = 1.0f;
    source->max_distance = 20.0f;
    source->rolloff_factor = 0.1f;
    source->looping = false;
    source->positional = true;
    source->audio_buffer_id = 0; // Use first buffer
    source->playback_position = 0.0f;
    
    audio_play(source);
    
    ScriptValue result = {SCRIPT_TYPE_BOOL};
    result.data.boolean = true;
    return result;
}

void script_init(Engine* engine) {
    engine->script_count = 0;
    registry_count = 0;
    
    // Register built-in functions
    script_register_function("print", builtin_print);
    script_register_function("spawn_particle", builtin_spawn_particle);
    script_register_function("get_player_pos", builtin_get_player_pos);
    script_register_function("play_sound", builtin_play_sound);
}

void script_cleanup(Engine* engine) {
    // Free script resources
    for (int i = 0; i < engine->script_count; i++) {
        for (int j = 0; j < engine->scripts[i].property_count; j++) {
            if (engine->scripts[i].properties[j].type == SCRIPT_TYPE_STRING) {
                free(engine->scripts[i].properties[j].data.string);
            }
        }
    }
    engine->script_count = 0;
}

void script_load(Engine* engine, const char* filename) {
    // Simplified script loading - in real implementation would parse a script file
    if (engine->script_count >= MAX_SCRIPTS) return;
    
    Script* script = &engine->scripts[engine->script_count++];
    strncpy(script->name, filename, 63);
    script->name[63] = '\0';
    script->active = true;
    script->property_count = 0;
    
    // Example: Create a simple trigger script
    script->update = NULL;
    script->on_collision = NULL;
    script->on_trigger = builtin_spawn_particle;
}

void script_update_all(Engine* engine) {
    for (int i = 0; i < engine->script_count; i++) {
        Script* script = &engine->scripts[i];
        
        if (!script->active || !script->update) continue;
        
        // Call update function
        script->update(engine, NULL, 0);
    }
}

ScriptValue script_call_function(Script* script, const char* func_name, 
                                 Engine* engine, ScriptValue* args, int arg_count) {
    ScriptValue result = {SCRIPT_TYPE_NULL};
    
    if (!script->active) return result;
    
    // Check which function to call
    if (strcmp(func_name, "update") == 0 && script->update) {
        return script->update(engine, args, arg_count);
    } else if (strcmp(func_name, "on_collision") == 0 && script->on_collision) {
        return script->on_collision(engine, args, arg_count);
    } else if (strcmp(func_name, "on_trigger") == 0 && script->on_trigger) {
        return script->on_trigger(engine, args, arg_count);
    }
    
    // Try to find in registry
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(function_registry[i].name, func_name) == 0) {
            return function_registry[i].function(engine, args, arg_count);
        }
    }
    
    return result;
}

void script_register_function(const char* name, ScriptFunction func) {
    if (registry_count >= 256) return;
    
    strncpy(function_registry[registry_count].name, name, 63);
    function_registry[registry_count].name[63] = '\0';
    function_registry[registry_count].function = func;
    registry_count++;
}

ScriptValue script_create_value_number(float value) {
    ScriptValue result;
    result.type = SCRIPT_TYPE_NUMBER;
    result.data.number = value;
    return result;
}

ScriptValue script_create_value_vector(Vec3 value) {
    ScriptValue result;
    result.type = SCRIPT_TYPE_VECTOR3;
    result.data.vector = value;
    return result;
}

ScriptValue script_create_value_bool(bool value) {
    ScriptValue result;
    result.type = SCRIPT_TYPE_BOOL;
    result.data.boolean = value;
    return result;
}

// Example script functions that can be called from scripts
ScriptValue script_example_on_update(Engine* engine, ScriptValue* args, int arg_count) {
    (void)args;
    (void)arg_count;
    
    // Example: Spawn particles at player position every frame
    if (engine->frame_count % 60 == 0) {
        Vec3 pos = {engine->camera.position.x, engine->camera.position.y, 1.0f};
        Vec3 vel = {0, 0, 2};
        ColorF color = {1, 1, 0, 1};
        particle_emit(engine, pos, vel, color, 1.0f);
    }
    
    ScriptValue result = {SCRIPT_TYPE_NULL};
    return result;
}

ScriptValue script_example_on_collision(Engine* engine, ScriptValue* args, int arg_count) {
    (void)engine;
    (void)args;
    (void)arg_count;
    
    // Example: Play sound on collision
    printf("Collision detected!\n");
    
    ScriptValue result = {SCRIPT_TYPE_BOOL};
    result.data.boolean = true;
    return result;
}
