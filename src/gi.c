#include "../include/engine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void gi_init_probes(Engine* engine) {
    engine->probe_count = 0;
    
    // Place probes in a grid throughout the level
    int grid_size = 8;
    float spacing = (float)MAP_WIDTH / grid_size;
    
    for (int y = 0; y < grid_size && engine->probe_count < IRRADIANCE_PROBES; y++) {
        for (int x = 0; x < grid_size && engine->probe_count < IRRADIANCE_PROBES; x++) {
            IrradianceProbe* probe = &engine->gi_probes[engine->probe_count++];
            
            probe->position.x = x * spacing + spacing * 0.5f;
            probe->position.y = y * spacing + spacing * 0.5f;
            probe->position.z = 1.0f;
            probe->influence_radius = spacing * 1.5f;
            probe->needs_update = true;
            
            // Initialize irradiance to zero
            for (int i = 0; i < 6; i++) {
                probe->irradiance[i] = (ColorF){0.1f, 0.1f, 0.15f, 1.0f}; // Ambient
            }
        }
    }
}

// Update a single probe by sampling lighting in 6 directions
void gi_update_probe(Engine* engine, IrradianceProbe* probe) {
    if (!probe->needs_update) return;
    
    // Sample directions: +X, -X, +Y, -Y, +Z, -Z
    Vec3 directions[6] = {
        {1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, -1.0f}
    };
    
    for (int dir = 0; dir < 6; dir++) {
        ColorF accumulated = {0.0f, 0.0f, 0.0f, 1.0f};
        int sample_count = 0;
        
        // Sample multiple rays in this hemisphere
        for (int i = 0; i < 16; i++) {
            // Generate random direction in hemisphere
            float theta = (i / 16.0f) * 2.0f * 3.14159f;
            float phi = acosf(1.0f - 2.0f * ((i % 4) / 4.0f));
            
            Vec3 sample_dir;
            sample_dir.x = sinf(phi) * cosf(theta);
            sample_dir.y = sinf(phi) * sinf(theta);
            sample_dir.z = cosf(phi);
            
            // Orient to hemisphere
            if (vec3_dot(sample_dir, directions[dir]) < 0.0f) {
                sample_dir = vec3_mul(sample_dir, -1.0f);
            }
            
            // Cast ray from probe
            Vec2 ray_origin = {probe->position.x, probe->position.y};
            Vec2 ray_dir = {sample_dir.x, sample_dir.y};
            float ray_length = vec2_length(ray_dir);
            if (ray_length > 0.001f) {
                ray_dir = vec2_normalize(ray_dir);
            }
            
            // Simple ray march
            float distance = 0.0f;
            ColorF ray_color = {0.0f, 0.0f, 0.0f, 1.0f};
            
            while (distance < 10.0f) {
                Vec2 sample_pos = vec2_add(ray_origin, vec2_mul(ray_dir, distance));
                int mx = (int)sample_pos.x;
                int my = (int)sample_pos.y;
                
                if (mx >= 0 && mx < MAP_WIDTH && my >= 0 && my < MAP_HEIGHT) {
                    int tile = map_get_tile(&engine->world, mx, my);
                    if (tile > 0) {
                        // Hit wall - sample its color
                        int tex_id = engine->world.wall_textures[my][mx];
                        if (tex_id >= 0 && tex_id < engine->texture_count) {
                            Color wall_color = texture_sample(&engine->textures[tex_id], 0.5f, 0.5f);
                            ray_color.r = wall_color.r / 255.0f;
                            ray_color.g = wall_color.g / 255.0f;
                            ray_color.b = wall_color.b / 255.0f;
                        }
                        break;
                    }
                }
                
                distance += 0.5f;
            }
            
            // Accumulate light from this sample
            if (distance < 10.0f) {
                accumulated.r += ray_color.r;
                accumulated.g += ray_color.g;
                accumulated.b += ray_color.b;
                sample_count++;
            }
        }
        
        // Average samples
        if (sample_count > 0) {
            probe->irradiance[dir].r = accumulated.r / sample_count;
            probe->irradiance[dir].g = accumulated.g / sample_count;
            probe->irradiance[dir].b = accumulated.b / sample_count;
        }
        
        // Add contribution from lights
        for (int l = 0; l < engine->light_count; l++) {
            Light* light = &engine->lights[l];
            Vec3 to_light = vec3_sub(light->position, probe->position);
            float dist = vec3_length(to_light);
            
            if (dist < light->radius) {
                Vec3 light_dir = vec3_normalize(to_light);
                float alignment = vec3_dot(light_dir, directions[dir]);
                
                if (alignment > 0.0f) {
                    float attenuation = light->intensity / (1.0f + dist * dist * 0.1f);
                    attenuation *= alignment;
                    
                    probe->irradiance[dir].r += light->color.r * attenuation;
                    probe->irradiance[dir].g += light->color.g * attenuation;
                    probe->irradiance[dir].b += light->color.b * attenuation;
                }
            }
        }
    }
    
    probe->needs_update = false;
}

// Sample irradiance at a position by blending nearby probes
ColorF gi_sample_irradiance(Engine* engine, Vec3 position, Vec3 normal) {
    if (!engine->use_gi || engine->probe_count == 0) {
        return (ColorF){0.1f, 0.1f, 0.15f, 1.0f}; // Default ambient
    }
    
    ColorF result = {0.0f, 0.0f, 0.0f, 1.0f};
    float total_weight = 0.0f;
    
    // Find nearest probes and blend
    for (int i = 0; i < engine->probe_count; i++) {
        IrradianceProbe* probe = &engine->gi_probes[i];
        Vec3 to_probe = vec3_sub(probe->position, position);
        float distance = vec3_length(to_probe);
        
        if (distance < probe->influence_radius) {
            float weight = 1.0f - (distance / probe->influence_radius);
            weight = weight * weight; // Smooth falloff
            
            // Sample probe based on normal direction
            int dominant_dir = 0;
            float max_comp = fabsf(normal.x);
            
            if (fabsf(normal.y) > max_comp) {
                dominant_dir = normal.y > 0 ? 2 : 3;
                max_comp = fabsf(normal.y);
            }
            if (fabsf(normal.z) > max_comp) {
                dominant_dir = normal.z > 0 ? 4 : 5;
            }
            
            ColorF irradiance = probe->irradiance[dominant_dir];
            
            result.r += irradiance.r * weight;
            result.g += irradiance.g * weight;
            result.b += irradiance.b * weight;
            total_weight += weight;
        }
    }
    
    if (total_weight > 0.0f) {
        result.r /= total_weight;
        result.g /= total_weight;
        result.b /= total_weight;
    } else {
        result.r = 0.1f;
        result.g = 0.1f;
        result.b = 0.15f;
    }
    
    return result;
}

// Light propagation for dynamic GI
void gi_propagate_light(Engine* engine) {
    if (!engine->use_gi) return;
    
    // Update probes that need updating
    for (int i = 0; i < engine->probe_count; i++) {
        gi_update_probe(engine, &engine->gi_probes[i]);
    }
}
