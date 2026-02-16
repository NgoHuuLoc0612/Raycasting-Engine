#include "../include/engine.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static SDL_AudioDeviceID audio_device = 0;
static SDL_AudioSpec audio_spec;
static Vec3 listener_position = {0, 0, 0};
static Vec3 listener_forward = {1, 0, 0};
static Vec3 listener_up = {0, 0, 1};

// Simple audio buffer structure
typedef struct {
    float* samples;
    int sample_count;
    int channels;
} AudioBuffer;

static AudioBuffer audio_buffers[32];
static int audio_buffer_count = 0;

// Audio callback for SDL
static void audio_callback(void* userdata, Uint8* stream, int len) {
    Engine* engine = (Engine*)userdata;
    float* output = (float*)stream;
    int sample_count = len / sizeof(float);
    
    // Clear output
    memset(output, 0, len);
    
    // Mix all active audio sources
    for (int i = 0; i < engine->audio_source_count; i++) {
        AudioSource* source = &engine->audio_sources[i];
        
        if (!source->playing || source->audio_buffer_id < 0) continue;
        
        AudioBuffer* buffer = &audio_buffers[source->audio_buffer_id];
        
        for (int s = 0; s < sample_count && source->playback_position < buffer->sample_count; s++) {
            float sample = buffer->samples[(int)source->playback_position];
            
            // Apply volume
            sample *= source->volume;
            
            // Apply 3D positioning if needed
            if (source->positional) {
                Vec3 to_listener = vec3_sub(listener_position, source->position);
                float distance = vec3_length(to_listener);
                
                // Distance attenuation
                float attenuation = 1.0f / (1.0f + distance * source->rolloff_factor);
                if (distance > source->max_distance) {
                    attenuation = 0.0f;
                }
                
                sample *= attenuation;
            }
            
            output[s] += sample;
            source->playback_position += source->pitch;
        }
        
        // Loop or stop
        if (source->playback_position >= buffer->sample_count) {
            if (source->looping) {
                source->playback_position = 0.0f;
            } else {
                source->playing = false;
            }
        }
    }
    
    // Clamp output
    for (int i = 0; i < sample_count; i++) {
        if (output[i] > 1.0f) output[i] = 1.0f;
        if (output[i] < -1.0f) output[i] = -1.0f;
    }
}

void audio_init(Engine* engine) {
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_F32;
    desired_spec.channels = 2;
    desired_spec.samples = 1024;
    desired_spec.callback = audio_callback;
    desired_spec.userdata = engine;
    
    audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &audio_spec, 0);
    
    if (audio_device == 0) {
        // Audio init failed, continue without audio
        return;
    }
    
    // Start audio playback
    SDL_PauseAudioDevice(audio_device, 0);
    
    engine->audio_source_count = 0;
    memset(audio_buffers, 0, sizeof(audio_buffers));
    audio_buffer_count = 0;
}

void audio_cleanup(Engine* engine) {
    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    
    // Free audio buffers
    for (int i = 0; i < audio_buffer_count; i++) {
        if (audio_buffers[i].samples) {
            free(audio_buffers[i].samples);
            audio_buffers[i].samples = NULL;
        }
    }
}

void audio_update(Engine* engine) {
    // Update 3D audio for all sources
    for (int i = 0; i < engine->audio_source_count; i++) {
        if (engine->audio_sources[i].positional) {
            audio_update_3d(&engine->audio_sources[i], listener_position);
        }
    }
}

int audio_load_sound(const char* filename) {
    // Simplified: Generate procedural sound
    if (audio_buffer_count >= 32) return -1;
    
    AudioBuffer* buffer = &audio_buffers[audio_buffer_count];
    buffer->sample_count = 44100; // 1 second
    buffer->channels = 1;
    buffer->samples = (float*)malloc(buffer->sample_count * sizeof(float));
    
    // Generate simple tone
    float frequency = 440.0f; // A4 note
    for (int i = 0; i < buffer->sample_count; i++) {
        float t = i / 44100.0f;
        buffer->samples[i] = 0.3f * sinf(2.0f * 3.14159f * frequency * t);
        
        // Envelope (fade out)
        float envelope = 1.0f - (t / 1.0f);
        buffer->samples[i] *= envelope;
    }
    
    return audio_buffer_count++;
}

void audio_play(AudioSource* source) {
    if (audio_device == 0) return;
    
    SDL_LockAudioDevice(audio_device);
    source->playing = true;
    source->playback_position = 0.0f;
    SDL_UnlockAudioDevice(audio_device);
}

void audio_stop(AudioSource* source) {
    if (audio_device == 0) return;
    
    SDL_LockAudioDevice(audio_device);
    source->playing = false;
    source->playback_position = 0.0f;
    SDL_UnlockAudioDevice(audio_device);
}

void audio_set_listener(Vec3 position, Vec3 forward, Vec3 up) {
    if (audio_device == 0) return;
    
    SDL_LockAudioDevice(audio_device);
    listener_position = position;
    listener_forward = forward;
    listener_up = up;
    SDL_UnlockAudioDevice(audio_device);
}

void audio_update_3d(AudioSource* source, Vec3 listener_pos) {
    if (!source->positional) return;
    
    Vec3 to_listener = vec3_sub(listener_pos, source->position);
    float distance = vec3_length(to_listener);
    
    // Calculate attenuation based on distance
    float attenuation = 1.0f / (1.0f + distance * source->rolloff_factor);
    if (distance > source->max_distance) {
        attenuation = 0.0f;
    }
    
    // Apply attenuation to volume (this is simplified - real implementation would
    // adjust per-sample in the callback)
}
