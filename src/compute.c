#include "../include/engine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void compute_init(ComputeContext* ctx, int buffer_size) {
    ctx->buffer_size = buffer_size;
    ctx->input_buffer = (uint32_t*)malloc(buffer_size * sizeof(uint32_t));
    ctx->output_buffer = (uint32_t*)malloc(buffer_size * sizeof(uint32_t));
    ctx->use_compute = true;
    
    memset(ctx->input_buffer, 0, buffer_size * sizeof(uint32_t));
    memset(ctx->output_buffer, 0, buffer_size * sizeof(uint32_t));
}

void compute_cleanup(ComputeContext* ctx) {
    if (ctx->input_buffer) {
        free(ctx->input_buffer);
        ctx->input_buffer = NULL;
    }
    if (ctx->output_buffer) {
        free(ctx->output_buffer);
        ctx->output_buffer = NULL;
    }
}

// Simulated compute shader for post-processing
// In real GPU implementation, this would run massively parallel
static void compute_kernel_blur(uint32_t* input, uint32_t* output, 
                                int x, int y, int width, int height) {
    const int kernel_size = 5;
    const int half_kernel = kernel_size / 2;
    
    float r = 0, g = 0, b = 0;
    float weight_sum = 0;
    
    // Gaussian weights
    float weights[5] = {0.06f, 0.24f, 0.40f, 0.24f, 0.06f};
    
    for (int ky = -half_kernel; ky <= half_kernel; ky++) {
        for (int kx = -half_kernel; kx <= half_kernel; kx++) {
            int sx = x + kx;
            int sy = y + ky;
            
            if (sx < 0 || sx >= width || sy < 0 || sy >= height) continue;
            
            int idx = sy * width + sx;
            uint32_t pixel = input[idx];
            
            float weight = weights[abs(kx)] * weights[abs(ky)];
            
            r += ((pixel >> 16) & 0xFF) * weight;
            g += ((pixel >> 8) & 0xFF) * weight;
            b += (pixel & 0xFF) * weight;
            weight_sum += weight;
        }
    }
    
    if (weight_sum > 0) {
        r /= weight_sum;
        g /= weight_sum;
        b /= weight_sum;
    }
    
    output[y * width + x] = 0xFF000000 | 
                           ((int)r << 16) | 
                           ((int)g << 8) | 
                           (int)b;
}

// Simulated compute shader for tone mapping
static void compute_kernel_tonemapping(uint32_t* input, uint32_t* output,
                                       int x, int y, int width, int height,
                                       float exposure, float gamma) {
    int idx = y * width + x;
    uint32_t pixel = input[idx];
    
    float r = ((pixel >> 16) & 0xFF) / 255.0f;
    float g = ((pixel >> 8) & 0xFF) / 255.0f;
    float b = (pixel & 0xFF) / 255.0f;
    
    // Apply exposure
    r *= exposure;
    g *= exposure;
    b *= exposure;
    
    // Reinhard tone mapping
    r = r / (1.0f + r);
    g = g / (1.0f + g);
    b = b / (1.0f + b);
    
    // Gamma correction
    r = powf(r, 1.0f / gamma);
    g = powf(g, 1.0f / gamma);
    b = powf(b, 1.0f / gamma);
    
    // Clamp
    if (r > 1.0f) r = 1.0f;
    if (g > 1.0f) g = 1.0f;
    if (b > 1.0f) b = 1.0f;
    
    output[idx] = 0xFF000000 | 
                 ((int)(r * 255) << 16) | 
                 ((int)(g * 255) << 8) | 
                 (int)(b * 255);
}

// Simulated compute shader for SSAO (Screen-Space Ambient Occlusion)
static void compute_kernel_ssao(uint32_t* input, uint32_t* output, float* depth_buffer,
                                int x, int y, int width, int height) {
    int idx = y * width + x;
    float depth = depth_buffer[x];
    
    if (depth >= MAX_RENDER_DISTANCE) {
        output[idx] = input[idx];
        return;
    }
    
    // Sample surrounding depths
    float occlusion = 0.0f;
    int sample_count = 0;
    
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int sx = x + dx;
            int sy = y + dy;
            
            if (sx < 0 || sx >= width || sy < 0 || sy >= height) continue;
            
            float sample_depth = depth_buffer[sx];
            float depth_diff = sample_depth - depth;
            
            if (depth_diff > 0.1f && depth_diff < 2.0f) {
                occlusion += 1.0f;
            }
            sample_count++;
        }
    }
    
    if (sample_count > 0) {
        occlusion /= sample_count;
    }
    
    // Apply occlusion
    uint32_t pixel = input[idx];
    float factor = 1.0f - occlusion * 0.5f;
    
    int r = (int)(((pixel >> 16) & 0xFF) * factor);
    int g = (int)(((pixel >> 8) & 0xFF) * factor);
    int b = (int)((pixel & 0xFF) * factor);
    
    output[idx] = 0xFF000000 | (r << 16) | (g << 8) | b;
}

void compute_dispatch_post_process(ComputeContext* ctx, uint32_t* input, 
                                   uint32_t* output, int width, int height) {
    if (!ctx->use_compute) {
        memcpy(output, input, width * height * sizeof(uint32_t));
        return;
    }
    
    // Copy to compute buffers
    memcpy(ctx->input_buffer, input, width * height * sizeof(uint32_t));
    
    // Simulate compute shader dispatch (parallel on GPU, serial here)
    // In real GPU: Each thread processes one pixel in parallel
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            compute_kernel_blur(ctx->input_buffer, ctx->output_buffer, 
                              x, y, width, height);
        }
    }
    
    // Copy result back
    memcpy(output, ctx->output_buffer, width * height * sizeof(uint32_t));
}

void compute_dispatch_lighting(ComputeContext* ctx, Engine* engine) {
    if (!ctx->use_compute) return;
    
    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;
    
    // Simulate parallel lighting calculation
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            
            // Get pixel depth
            float depth = engine->buffers.z_buffer[x % width];
            if (depth >= MAX_RENDER_DISTANCE) continue;
            
            // Calculate world position
            float camera_x = 2.0f * (x % width) / (float)width - 1.0f;
            Vec2 ray_dir;
            ray_dir.x = engine->camera.direction.x + engine->camera.plane.x * camera_x;
            ray_dir.y = engine->camera.direction.y + engine->camera.plane.y * camera_x;
            
            Vec3 world_pos;
            world_pos.x = engine->camera.position.x + ray_dir.x * depth;
            world_pos.y = engine->camera.position.y + ray_dir.y * depth;
            world_pos.z = engine->camera.z_position;
            
            // Calculate lighting from all lights
            ColorF total_light = {0.2f, 0.2f, 0.25f, 1.0f}; // Ambient
            
            for (int l = 0; l < engine->light_count; l++) {
                Light* light = &engine->lights[l];
                
                Vec3 to_light = vec3_sub(light->position, world_pos);
                float distance = vec3_length(to_light);
                
                if (distance < light->radius) {
                    float attenuation = light->intensity / (1.0f + distance * distance * 0.01f);
                    
                    total_light.r += light->color.r * attenuation;
                    total_light.g += light->color.g * attenuation;
                    total_light.b += light->color.b * attenuation;
                }
            }
            
            // Clamp
            if (total_light.r > 2.0f) total_light.r = 2.0f;
            if (total_light.g > 2.0f) total_light.g = 2.0f;
            if (total_light.b > 2.0f) total_light.b = 2.0f;
            
            // Store in light buffer
            engine->buffers.light_buffer[idx * 4 + 0] = (uint8_t)(total_light.r * 255);
            engine->buffers.light_buffer[idx * 4 + 1] = (uint8_t)(total_light.g * 255);
            engine->buffers.light_buffer[idx * 4 + 2] = (uint8_t)(total_light.b * 255);
            engine->buffers.light_buffer[idx * 4 + 3] = 255;
        }
    }
}

// SIMD-accelerated version (if SSE/AVX available)
#ifdef __SSE2__
#include <emmintrin.h>

void compute_dispatch_post_process_simd(ComputeContext* ctx, uint32_t* input,
                                       uint32_t* output, int width, int height) {
    // Process 4 pixels at once with SSE2
    int total_pixels = width * height;
    int simd_count = total_pixels / 4;
    
    __m128i* input_simd = (__m128i*)input;
    __m128i* output_simd = (__m128i*)output;
    
    for (int i = 0; i < simd_count; i++) {
        __m128i pixels = _mm_load_si128(&input_simd[i]);
        
        // Process 4 pixels in parallel
        // (Simplified - real implementation would do full blur)
        
        _mm_store_si128(&output_simd[i], pixels);
    }
    
    // Handle remaining pixels
    for (int i = simd_count * 4; i < total_pixels; i++) {
        output[i] = input[i];
    }
}
#endif
