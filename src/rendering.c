#include "../include/engine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Gaussian blur kernel for bloom
static const float GAUSSIAN_KERNEL[5] = {0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f};

void apply_lighting(Engine* engine) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            Color pixel = uint32_to_color(engine->buffers.color_buffer[idx]);
            
            ColorF final_light = {0.2f, 0.2f, 0.25f, 1.0f}; // Ambient
            
            // Calculate world position for this pixel
            float depth = engine->buffers.z_buffer[x % SCREEN_WIDTH];
            if (depth >= MAX_RENDER_DISTANCE) continue;
            
            // Apply each point light
            for (int i = 0; i < engine->light_count; i++) {
                Light* light = &engine->lights[i];
                
                // Simple distance-based attenuation
                float distance = depth;
                float attenuation = light->intensity / (1.0f + distance * distance * 0.01f);
                
                if (attenuation > 0.01f) {
                    final_light.r += light->color.r * attenuation;
                    final_light.g += light->color.g * attenuation;
                    final_light.b += light->color.b * attenuation;
                }
            }
            
            // Clamp lighting
            if (final_light.r > 2.0f) final_light.r = 2.0f;
            if (final_light.g > 2.0f) final_light.g = 2.0f;
            if (final_light.b > 2.0f) final_light.b = 2.0f;
            
            // Apply lighting to pixel
            pixel.r = (uint8_t)(pixel.r * final_light.r);
            pixel.g = (uint8_t)(pixel.g * final_light.g);
            pixel.b = (uint8_t)(pixel.b * final_light.b);
            
            if (pixel.r > 255) pixel.r = 255;
            if (pixel.g > 255) pixel.g = 255;
            if (pixel.b > 255) pixel.b = 255;
            
            engine->buffers.color_buffer[idx] = color_to_uint32(pixel);
        }
    }
}

void apply_shadows(Engine* engine) {
    for (int i = 0; i < engine->light_count; i++) {
        if (!engine->lights[i].cast_shadows) continue;
        
        Light* light = &engine->lights[i];
        
        // Simple shadow mapping using ray marching
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                int idx = y * SCREEN_WIDTH + x;
                float depth = engine->buffers.z_buffer[x % SCREEN_WIDTH];
                
                if (depth >= MAX_RENDER_DISTANCE) continue;
                
                // Calculate world position
                float camera_x = 2.0f * (x % SCREEN_WIDTH) / (float)SCREEN_WIDTH - 1.0f;
                Vec2 ray_dir;
                ray_dir.x = engine->camera.direction.x + engine->camera.plane.x * camera_x;
                ray_dir.y = engine->camera.direction.y + engine->camera.plane.y * camera_x;
                
                Vec2 world_pos;
                world_pos.x = engine->camera.position.x + ray_dir.x * depth;
                world_pos.y = engine->camera.position.y + ray_dir.y * depth;
                
                // Ray from surface to light
                Vec2 to_light = {
                    light->position.x - world_pos.x,
                    light->position.y - world_pos.y
                };
                float light_dist = vec2_length(to_light);
                to_light = vec2_normalize(to_light);
                
                // March ray towards light
                bool in_shadow = false;
                float march_step = 0.1f;
                Vec2 march_pos = world_pos;
                
                for (float d = march_step; d < light_dist; d += march_step) {
                    march_pos.x = world_pos.x + to_light.x * d;
                    march_pos.y = world_pos.y + to_light.y * d;
                    
                    int mx = (int)march_pos.x;
                    int my = (int)march_pos.y;
                    
                    if (mx >= 0 && mx < MAP_WIDTH && my >= 0 && my < MAP_HEIGHT) {
                        if (map_get_tile(&engine->world, mx, my) > 0) {
                            in_shadow = true;
                            break;
                        }
                    }
                }
                
                if (in_shadow) {
                    Color pixel = uint32_to_color(engine->buffers.color_buffer[idx]);
                    pixel.r = (uint8_t)(pixel.r * 0.3f);
                    pixel.g = (uint8_t)(pixel.g * 0.3f);
                    pixel.b = (uint8_t)(pixel.b * 0.3f);
                    engine->buffers.color_buffer[idx] = color_to_uint32(pixel);
                }
            }
        }
    }
}

void apply_fog(Engine* engine) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            float depth = engine->buffers.z_buffer[x % SCREEN_WIDTH];
            
            if (depth < engine->fog.start_distance) continue;
            
            float fog_factor = (depth - engine->fog.start_distance) / 
                              (engine->fog.end_distance - engine->fog.start_distance);
            fog_factor = fog_factor < 0.0f ? 0.0f : fog_factor > 1.0f ? 1.0f : fog_factor;
            fog_factor = 1.0f - expf(-engine->fog.density * depth);
            
            Color pixel = uint32_to_color(engine->buffers.color_buffer[idx]);
            
            pixel.r = (uint8_t)(pixel.r * (1.0f - fog_factor) + 
                               engine->fog.color.r * 255.0f * fog_factor);
            pixel.g = (uint8_t)(pixel.g * (1.0f - fog_factor) + 
                               engine->fog.color.g * 255.0f * fog_factor);
            pixel.b = (uint8_t)(pixel.b * (1.0f - fog_factor) + 
                               engine->fog.color.b * 255.0f * fog_factor);
            
            engine->buffers.color_buffer[idx] = color_to_uint32(pixel);
        }
    }
}

void post_process_bloom(Engine* engine) {
    // Extract bright pixels
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            Color pixel = uint32_to_color(engine->buffers.color_buffer[idx]);
            
            float brightness = (pixel.r + pixel.g + pixel.b) / (3.0f * 255.0f);
            
            if (brightness > engine->post_fx.bloom_threshold) {
                engine->buffers.post_process_buffer[idx] = engine->buffers.color_buffer[idx];
            } else {
                engine->buffers.post_process_buffer[idx] = 0;
            }
        }
    }
    
    // Horizontal blur
    uint32_t* temp = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            ColorF sum = {0, 0, 0, 0};
            
            for (int i = -4; i <= 4; i++) {
                int sx = x + i;
                if (sx < 0 || sx >= SCREEN_WIDTH) continue;
                
                Color c = uint32_to_color(engine->buffers.post_process_buffer[y * SCREEN_WIDTH + sx]);
                float weight = GAUSSIAN_KERNEL[abs(i)];
                
                sum.r += c.r * weight;
                sum.g += c.g * weight;
                sum.b += c.b * weight;
            }
            
            Color result = {(uint8_t)sum.r, (uint8_t)sum.g, (uint8_t)sum.b, 255};
            temp[y * SCREEN_WIDTH + x] = color_to_uint32(result);
        }
    }
    
    // Vertical blur and blend
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            ColorF sum = {0, 0, 0, 0};
            
            for (int i = -4; i <= 4; i++) {
                int sy = y + i;
                if (sy < 0 || sy >= SCREEN_HEIGHT) continue;
                
                Color c = uint32_to_color(temp[sy * SCREEN_WIDTH + x]);
                float weight = GAUSSIAN_KERNEL[abs(i)];
                
                sum.r += c.r * weight;
                sum.g += c.g * weight;
                sum.b += c.b * weight;
            }
            
            int idx = y * SCREEN_WIDTH + x;
            Color original = uint32_to_color(engine->buffers.color_buffer[idx]);
            
            float intensity = engine->post_fx.bloom_intensity;
            original.r = (uint8_t)(original.r + sum.r * intensity);
            original.g = (uint8_t)(original.g + sum.g * intensity);
            original.b = (uint8_t)(original.b + sum.b * intensity);
            
            if (original.r > 255) original.r = 255;
            if (original.g > 255) original.g = 255;
            if (original.b > 255) original.b = 255;
            
            engine->buffers.color_buffer[idx] = color_to_uint32(original);
        }
    }
    
    free(temp);
}

void post_process_chromatic_aberration(Engine* engine) {
    memcpy(engine->buffers.post_process_buffer, engine->buffers.color_buffer,
           SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    int offset = (int)(engine->post_fx.aberration_strength * 3.0f);
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int idx = y * SCREEN_WIDTH + x;
            
            // Sample red channel with offset
            int rx = x - offset;
            uint8_t r = 0;
            if (rx >= 0 && rx < SCREEN_WIDTH) {
                r = uint32_to_color(engine->buffers.post_process_buffer[y * SCREEN_WIDTH + rx]).r;
            }
            
            // Green channel - no offset
            uint8_t g = uint32_to_color(engine->buffers.post_process_buffer[idx]).g;
            
            // Sample blue channel with offset
            int bx = x + offset;
            uint8_t b = 0;
            if (bx >= 0 && bx < SCREEN_WIDTH) {
                b = uint32_to_color(engine->buffers.post_process_buffer[y * SCREEN_WIDTH + bx]).b;
            }
            
            Color result = {r, g, b, 255};
            engine->buffers.color_buffer[idx] = color_to_uint32(result);
        }
    }
}

void post_process_tone_mapping(Engine* engine) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        Color pixel = uint32_to_color(engine->buffers.color_buffer[i]);
        
        // Apply exposure
        float r = pixel.r / 255.0f * engine->post_fx.exposure;
        float g = pixel.g / 255.0f * engine->post_fx.exposure;
        float b = pixel.b / 255.0f * engine->post_fx.exposure;
        
        // Reinhard tone mapping
        r = r / (1.0f + r);
        g = g / (1.0f + g);
        b = b / (1.0f + b);
        
        // Gamma correction
        r = powf(r, 1.0f / engine->post_fx.gamma);
        g = powf(g, 1.0f / engine->post_fx.gamma);
        b = powf(b, 1.0f / engine->post_fx.gamma);
        
        pixel.r = (uint8_t)(r * 255.0f);
        pixel.g = (uint8_t)(g * 255.0f);
        pixel.b = (uint8_t)(b * 255.0f);
        
        engine->buffers.color_buffer[i] = color_to_uint32(pixel);
    }
}

void post_process_vignette(Engine* engine) {
    float center_x = SCREEN_WIDTH * 0.5f;
    float center_y = SCREEN_HEIGHT * 0.5f;
    float max_dist = sqrtf(center_x * center_x + center_y * center_y);
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            float dx = x - center_x;
            float dy = y - center_y;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float vignette = 1.0f - (dist / max_dist) * engine->post_fx.vignette_intensity;
            if (vignette < 0.0f) vignette = 0.0f;
            
            int idx = y * SCREEN_WIDTH + x;
            Color pixel = uint32_to_color(engine->buffers.color_buffer[idx]);
            
            pixel.r = (uint8_t)(pixel.r * vignette);
            pixel.g = (uint8_t)(pixel.g * vignette);
            pixel.b = (uint8_t)(pixel.b * vignette);
            
            engine->buffers.color_buffer[idx] = color_to_uint32(pixel);
        }
    }
}

void post_process_fxaa(Engine* engine) {
    memcpy(engine->buffers.post_process_buffer, engine->buffers.color_buffer,
           SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    const float EDGE_THRESHOLD = 0.125f;
    const float SUBPIX_QUALITY = 0.75f;
    
    for (int y = 1; y < SCREEN_HEIGHT - 1; y++) {
        for (int x = 1; x < SCREEN_WIDTH - 1; x++) {
            int idx = y * SCREEN_WIDTH + x;
            
            Color center = uint32_to_color(engine->buffers.post_process_buffer[idx]);
            Color top = uint32_to_color(engine->buffers.post_process_buffer[(y-1) * SCREEN_WIDTH + x]);
            Color bottom = uint32_to_color(engine->buffers.post_process_buffer[(y+1) * SCREEN_WIDTH + x]);
            Color left = uint32_to_color(engine->buffers.post_process_buffer[y * SCREEN_WIDTH + (x-1)]);
            Color right = uint32_to_color(engine->buffers.post_process_buffer[y * SCREEN_WIDTH + (x+1)]);
            
            float luma_center = (center.r + center.g + center.b) / (3.0f * 255.0f);
            float luma_top = (top.r + top.g + top.b) / (3.0f * 255.0f);
            float luma_bottom = (bottom.r + bottom.g + bottom.b) / (3.0f * 255.0f);
            float luma_left = (left.r + left.g + left.b) / (3.0f * 255.0f);
            float luma_right = (right.r + right.g + right.b) / (3.0f * 255.0f);
            
            float edge = fabsf(luma_center - luma_top) + fabsf(luma_center - luma_bottom) +
                        fabsf(luma_center - luma_left) + fabsf(luma_center - luma_right);
            
            if (edge > EDGE_THRESHOLD) {
                Color blend = {
                    (uint8_t)((center.r + top.r + bottom.r + left.r + right.r) / 5),
                    (uint8_t)((center.g + top.g + bottom.g + left.g + right.g) / 5),
                    (uint8_t)((center.b + top.b + bottom.b + left.b + right.b) / 5),
                    255
                };
                
                engine->buffers.color_buffer[idx] = color_to_uint32(blend);
            }
        }
    }
}

void post_process_motion_blur(Engine* engine) {
    // Motion blur based on camera velocity
    float velocity = vec2_length(engine->camera.physics.velocity);
    if (velocity < 0.01f) return;
    
    Vec2 blur_dir = vec2_normalize(engine->camera.physics.velocity);
    int samples = (int)(velocity * engine->post_fx.motion_blur_strength * 10.0f);
    if (samples < 2) return;
    if (samples > 16) samples = 16;
    
    memcpy(engine->buffers.post_process_buffer, engine->buffers.color_buffer,
           SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            ColorF sum = {0, 0, 0, 0};
            
            for (int s = 0; s < samples; s++) {
                float offset = (s - samples / 2) * 2.0f;
                int sx = x + (int)(blur_dir.x * offset);
                int sy = y + (int)(blur_dir.y * offset);
                
                if (sx >= 0 && sx < SCREEN_WIDTH && sy >= 0 && sy < SCREEN_HEIGHT) {
                    Color c = uint32_to_color(engine->buffers.post_process_buffer[sy * SCREEN_WIDTH + sx]);
                    sum.r += c.r;
                    sum.g += c.g;
                    sum.b += c.b;
                }
            }
            
            Color result = {
                (uint8_t)(sum.r / samples),
                (uint8_t)(sum.g / samples),
                (uint8_t)(sum.b / samples),
                255
            };
            
            engine->buffers.color_buffer[y * SCREEN_WIDTH + x] = color_to_uint32(result);
        }
    }
}
