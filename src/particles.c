#include "../include/engine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Particle system implementation
void particle_emit(Engine* engine, Vec3 position, Vec3 velocity, ColorF color, float lifetime) {
    if (engine->particle_count >= MAX_PARTICLES) return;
    
    Particle* p = &engine->particles[engine->particle_count++];
    p->position = position;
    p->velocity = velocity;
    p->color = color;
    p->lifetime = lifetime;
    p->size = 0.1f;
    p->gravity_scale = 1.0f;
    p->texture_id = -1;
}

void particle_update(Engine* engine, float delta_time) {
    for (int i = 0; i < engine->particle_count; i++) {
        Particle* p = &engine->particles[i];
        
        // Update lifetime
        p->lifetime -= delta_time;
        
        if (p->lifetime <= 0.0f) {
            // Remove dead particle by swapping with last
            engine->particles[i] = engine->particles[engine->particle_count - 1];
            engine->particle_count--;
            i--;
            continue;
        }
        
        // Apply velocity
        p->position = vec3_add(p->position, vec3_mul(p->velocity, delta_time));
        
        // Apply gravity
        p->velocity.z += -9.81f * p->gravity_scale * delta_time;
        
        // Apply air resistance
        p->velocity = vec3_mul(p->velocity, 0.98f);
        
        // Fade alpha based on lifetime
        if (p->lifetime < 1.0f) {
            p->color.a = p->lifetime;
        }
    }
}

void particle_render(Engine* engine) {
    for (int i = 0; i < engine->particle_count; i++) {
        Particle* p = &engine->particles[i];
        
        // Transform to camera space
        Vec2 sprite_pos = {p->position.x - engine->camera.position.x,
                          p->position.y - engine->camera.position.y};
        
        float inv_det = 1.0f / (engine->camera.plane.x * engine->camera.direction.y - 
                                engine->camera.direction.x * engine->camera.plane.y);
        
        Vec2 transform;
        transform.x = inv_det * (engine->camera.direction.y * sprite_pos.x - 
                                 engine->camera.direction.x * sprite_pos.y);
        transform.y = inv_det * (-engine->camera.plane.y * sprite_pos.x + 
                                 engine->camera.plane.x * sprite_pos.y);
        
        if (transform.y <= 0.0f) continue;
        
        int screen_x = (int)((SCREEN_WIDTH / 2) * (1 + transform.x / transform.y));
        int screen_y = (int)(SCREEN_HEIGHT / 2 - (SCREEN_HEIGHT / transform.y) * 
                            (p->position.z - engine->camera.z_position));
        
        int size = (int)(p->size * SCREEN_HEIGHT / transform.y);
        
        for (int dy = -size; dy <= size; dy++) {
            for (int dx = -size; dx <= size; dx++) {
                if (dx * dx + dy * dy > size * size) continue;
                
                int px = screen_x + dx;
                int py = screen_y + dy;
                
                if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;
                
                int idx = py * SCREEN_WIDTH + px;
                
                if (transform.y < engine->buffers.z_buffer[px]) {
                    Color existing = uint32_to_color(engine->buffers.color_buffer[idx]);
                    
                    float alpha = p->color.a;
                    Color particle_color = {
                        (uint8_t)(p->color.r * 255),
                        (uint8_t)(p->color.g * 255),
                        (uint8_t)(p->color.b * 255),
                        (uint8_t)(alpha * 255)
                    };
                    
                    // Alpha blend
                    existing.r = (uint8_t)(existing.r * (1.0f - alpha) + particle_color.r * alpha);
                    existing.g = (uint8_t)(existing.g * (1.0f - alpha) + particle_color.g * alpha);
                    existing.b = (uint8_t)(existing.b * (1.0f - alpha) + particle_color.b * alpha);
                    
                    engine->buffers.color_buffer[idx] = color_to_uint32(existing);
                }
            }
        }
    }
}

// Texture operations
Color texture_sample(Texture* texture, float u, float v) {
    if (!texture->pixels) {
        return (Color){255, 0, 255, 255}; // Magenta for missing texture
    }
    
    int x = (int)(u * texture->width) % texture->width;
    int y = (int)(v * texture->height) % texture->height;
    
    if (x < 0) x += texture->width;
    if (y < 0) y += texture->height;
    
    uint32_t pixel = texture->pixels[y * texture->width + x];
    return uint32_to_color(pixel);
}

Color texture_sample_bilinear(Texture* texture, float u, float v) {
    if (!texture->pixels) {
        return (Color){255, 0, 255, 255};
    }
    
    float x = u * texture->width - 0.5f;
    float y = v * texture->height - 0.5f;
    
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    float fx = x - x0;
    float fy = y - y0;
    
    x0 = (x0 % texture->width + texture->width) % texture->width;
    x1 = (x1 % texture->width + texture->width) % texture->width;
    y0 = (y0 % texture->height + texture->height) % texture->height;
    y1 = (y1 % texture->height + texture->height) % texture->height;
    
    Color c00 = uint32_to_color(texture->pixels[y0 * texture->width + x0]);
    Color c10 = uint32_to_color(texture->pixels[y0 * texture->width + x1]);
    Color c01 = uint32_to_color(texture->pixels[y1 * texture->width + x0]);
    Color c11 = uint32_to_color(texture->pixels[y1 * texture->width + x1]);
    
    Color result;
    result.r = (uint8_t)(
        c00.r * (1 - fx) * (1 - fy) +
        c10.r * fx * (1 - fy) +
        c01.r * (1 - fx) * fy +
        c11.r * fx * fy
    );
    result.g = (uint8_t)(
        c00.g * (1 - fx) * (1 - fy) +
        c10.g * fx * (1 - fy) +
        c01.g * (1 - fx) * fy +
        c11.g * fx * fy
    );
    result.b = (uint8_t)(
        c00.b * (1 - fx) * (1 - fy) +
        c10.b * fx * (1 - fy) +
        c01.b * (1 - fx) * fy +
        c11.b * fx * fy
    );
    result.a = 255;
    
    return result;
}

Color texture_sample_trilinear(Texture* texture, float u, float v, float mip_level) {
    // Simplified - full implementation would use actual mipmaps
    return texture_sample_bilinear(texture, u, v);
}

Vec3 texture_sample_normal(Texture* texture, float u, float v) {
    if (!texture->normal_map) {
        return (Vec3){0.0f, 0.0f, 1.0f};
    }
    
    int x = (int)(u * texture->width) % texture->width;
    int y = (int)(v * texture->height) % texture->height;
    
    if (x < 0) x += texture->width;
    if (y < 0) y += texture->height;
    
    int idx = (y * texture->width + x) * 3;
    
    Vec3 normal;
    normal.x = (texture->normal_map[idx] / 255.0f) * 2.0f - 1.0f;
    normal.y = (texture->normal_map[idx + 1] / 255.0f) * 2.0f - 1.0f;
    normal.z = (texture->normal_map[idx + 2] / 255.0f) * 2.0f - 1.0f;
    
    return vec3_normalize(normal);
}

bool texture_load(Texture* texture, const char* filename) {
    // This is a stub - real implementation would use stb_image or similar
    // For now, generate procedural texture
    texture->width = TEXTURE_SIZE;
    texture->height = TEXTURE_SIZE;
    texture->has_alpha = false;
    
    texture->pixels = (uint32_t*)malloc(texture->width * texture->height * sizeof(uint32_t));
    
    // Generate checkerboard pattern
    for (int y = 0; y < texture->height; y++) {
        for (int x = 0; x < texture->width; x++) {
            bool checker = ((x / 8) + (y / 8)) % 2;
            uint8_t value = checker ? 200 : 100;
            
            Color c = {value, value, value, 255};
            texture->pixels[y * texture->width + x] = color_to_uint32(c);
        }
    }
    
    return true;
}

void texture_generate_mipmaps(Texture* texture) {
    // Simplified mipmap generation
    // Full implementation would generate multiple levels
}

// Lighting calculations
ColorF lighting_calculate_diffuse(Vec3 normal, Vec3 light_dir, ColorF light_color) {
    float diff = fmaxf(vec3_dot(normal, light_dir), 0.0f);
    
    return (ColorF){
        light_color.r * diff,
        light_color.g * diff,
        light_color.b * diff,
        1.0f
    };
}

ColorF lighting_calculate_specular(Vec3 normal, Vec3 light_dir, Vec3 view_dir, 
                                    ColorF light_color, float shininess) {
    Vec3 reflect_dir = vec3_sub(vec3_mul(normal, 2.0f * vec3_dot(light_dir, normal)), 
                                light_dir);
    float spec = powf(fmaxf(vec3_dot(view_dir, reflect_dir), 0.0f), shininess);
    
    return (ColorF){
        light_color.r * spec,
        light_color.g * spec,
        light_color.b * spec,
        1.0f
    };
}

float lighting_calculate_shadow(Engine* engine, Vec3 position, Vec3 light_pos) {
    Vec3 to_light = vec3_sub(light_pos, position);
    float distance = vec3_length(to_light);
    to_light = vec3_normalize(to_light);
    
    // Ray march towards light
    float shadow = 1.0f;
    float t = 0.01f;
    
    while (t < distance) {
        Vec3 sample_pos = vec3_add(position, vec3_mul(to_light, t));
        
        int mx = (int)sample_pos.x;
        int my = (int)sample_pos.y;
        
        if (mx >= 0 && mx < MAP_WIDTH && my >= 0 && my < MAP_HEIGHT) {
            if (map_get_tile(&engine->world, mx, my) > 0) {
                shadow = 0.0f;
                break;
            }
        }
        
        t += 0.1f;
    }
    
    return shadow;
}

void lighting_apply_point_light(Engine* engine, Light* light) {
    // This would be called for each light during rendering
    // Implementation integrated into apply_lighting in rendering.c
}

// Color utilities
Color color_blend_alpha(Color src, Color dst) {
    float alpha = src.a / 255.0f;
    
    return (Color){
        (uint8_t)(src.r * alpha + dst.r * (1.0f - alpha)),
        (uint8_t)(src.g * alpha + dst.g * (1.0f - alpha)),
        (uint8_t)(src.b * alpha + dst.b * (1.0f - alpha)),
        255
    };
}

Color color_multiply(Color c, float factor) {
    return (Color){
        (uint8_t)(c.r * factor),
        (uint8_t)(c.g * factor),
        (uint8_t)(c.b * factor),
        c.a
    };
}

ColorF colorf_add(ColorF a, ColorF b) {
    return (ColorF){a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a};
}

ColorF colorf_multiply(ColorF c, float factor) {
    return (ColorF){c.r * factor, c.g * factor, c.b * factor, c.a * factor};
}

uint32_t color_to_uint32(Color c) {
    return (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
}

Color uint32_to_color(uint32_t c) {
    return (Color){
        (c >> 16) & 0xFF,
        (c >> 8) & 0xFF,
        c & 0xFF,
        (c >> 24) & 0xFF
    };
}
