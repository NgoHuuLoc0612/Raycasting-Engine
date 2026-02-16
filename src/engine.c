#include "../include/engine.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PI 3.14159265359f
#define HALF_PI 1.57079632679f
#define TWO_PI 6.28318530718f
#define DEG_TO_RAD (PI / 180.0f)
#define RAD_TO_DEG (180.0f / PI)

void engine_init(Engine* engine) {
    memset(engine, 0, sizeof(Engine));
    
    // Initialize camera
    engine->camera.position = (Vec2){MAP_WIDTH / 2.0f, MAP_HEIGHT / 2.0f};
    engine->camera.direction = (Vec2){-1.0f, 0.0f};
    engine->camera.plane = (Vec2){0.0f, 0.66f};
    engine->camera.pitch = 0.0f;
    engine->camera.z_position = 0.5f;
    engine->camera.velocity_z = 0.0f;
    engine->camera.crouching = false;
    
    // Initialize physics
    engine->camera.physics.position = engine->camera.position;
    engine->camera.physics.radius = 0.25f;
    engine->camera.physics.friction = 0.85f;
    engine->camera.physics.bounce = 0.0f;
    
    // Allocate render buffers
    engine->buffers.z_buffer = (float*)malloc(SCREEN_WIDTH * sizeof(float));
    engine->buffers.color_buffer = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    engine->buffers.shadow_buffer = (float*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(float));
    engine->buffers.light_buffer = (uint8_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    engine->buffers.post_process_buffer = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    // Initialize fog
    engine->fog.color = (ColorF){0.5f, 0.5f, 0.6f, 1.0f};
    engine->fog.density = 0.02f;
    engine->fog.start_distance = 5.0f;
    engine->fog.end_distance = MAX_RENDER_DISTANCE;
    
    // Initialize post-processing
    engine->post_fx.bloom_enabled = true;
    engine->post_fx.bloom_threshold = 0.8f;
    engine->post_fx.bloom_intensity = 0.3f;
    engine->post_fx.fxaa_enabled = true;
    engine->post_fx.gamma = 2.2f;
    engine->post_fx.exposure = 1.0f;
    engine->post_fx.vignette = true;
    engine->post_fx.vignette_intensity = 0.4f;
    
    // Generate procedural map
    map_generate_procedural(&engine->world, (uint32_t)time(NULL));
    
    // Add default lights
    engine->lights[0].position = (Vec3){MAP_WIDTH / 2.0f, MAP_HEIGHT / 2.0f, 2.0f};
    engine->lights[0].color = (ColorF){1.0f, 0.9f, 0.7f, 1.0f};
    engine->lights[0].intensity = 5.0f;
    engine->lights[0].radius = 15.0f;
    engine->lights[0].cast_shadows = true;
    engine->light_count = 1;
    
    engine->texture_count = 0;
    engine->sprite_count = 0;
    engine->particle_count = 0;
}

void engine_cleanup(Engine* engine) {
    free(engine->buffers.z_buffer);
    free(engine->buffers.color_buffer);
    free(engine->buffers.shadow_buffer);
    free(engine->buffers.light_buffer);
    free(engine->buffers.post_process_buffer);
    
    for (int i = 0; i < engine->texture_count; i++) {
        if (engine->textures[i].pixels) free(engine->textures[i].pixels);
        if (engine->textures[i].normal_map) free(engine->textures[i].normal_map);
        if (engine->textures[i].specular_map) free(engine->textures[i].specular_map);
        if (engine->textures[i].emission_map) free(engine->textures[i].emission_map);
    }
}

void engine_update(Engine* engine, float delta_time) {
    engine->delta_time = delta_time;
    engine->frame_count++;
    engine->time_accumulator += delta_time;
    
    // Update physics with substeps for stability
    float substep_dt = delta_time / PHYSICS_SUBSTEPS;
    for (int i = 0; i < PHYSICS_SUBSTEPS; i++) {
        physics_update(engine, &engine->camera.physics, substep_dt);
    }
    
    // Sync camera position with physics
    engine->camera.position = engine->camera.physics.position;
    
    // Update camera headbob
    float speed = vec2_length(engine->camera.physics.velocity);
    camera_update_headbob(&engine->camera, delta_time, speed > 0.01f);
    
    // Update doors
    for (int i = 0; i < engine->world.door_count; i++) {
        door_update(&engine->world.doors[i], delta_time);
    }
    
    // Update sprites
    for (int i = 0; i < engine->sprite_count; i++) {
        sprite_animate(&engine->sprites[i], delta_time);
    }
    
    // Update particles
    particle_update(engine, delta_time);
    
    // Update light flickering
    for (int i = 0; i < engine->light_count; i++) {
        if (engine->lights[i].flickering > 0.0f) {
            float flicker = fast_sin(engine->time_accumulator * 10.0f + i * 2.0f);
            engine->lights[i].intensity *= 1.0f + flicker * engine->lights[i].flickering;
        }
    }
}

void raycast_dda(Engine* engine, int x, Ray* ray) {
    float camera_x = 2.0f * x / (float)SCREEN_WIDTH - 1.0f;
    
    ray->origin = engine->camera.position;
    ray->direction.x = engine->camera.direction.x + engine->camera.plane.x * camera_x;
    ray->direction.y = engine->camera.direction.y + engine->camera.plane.y * camera_x;
    
    ray->map_x = (int)ray->origin.x;
    ray->map_y = (int)ray->origin.y;
    
    float delta_dist_x = fabsf(1.0f / ray->direction.x);
    float delta_dist_y = fabsf(1.0f / ray->direction.y);
    
    int step_x, step_y;
    float side_dist_x, side_dist_y;
    
    if (ray->direction.x < 0) {
        step_x = -1;
        side_dist_x = (ray->origin.x - ray->map_x) * delta_dist_x;
    } else {
        step_x = 1;
        side_dist_x = (ray->map_x + 1.0f - ray->origin.x) * delta_dist_x;
    }
    
    if (ray->direction.y < 0) {
        step_y = -1;
        side_dist_y = (ray->origin.y - ray->map_y) * delta_dist_y;
    } else {
        step_y = 1;
        side_dist_y = (ray->map_y + 1.0f - ray->origin.y) * delta_dist_y;
    }
    
    bool hit = false;
    ray->hit_door = false;
    
    // DDA algorithm
    while (!hit && ray->distance < MAX_RENDER_DISTANCE) {
        if (side_dist_x < side_dist_y) {
            side_dist_x += delta_dist_x;
            ray->map_x += step_x;
            ray->side = 0;
        } else {
            side_dist_y += delta_dist_y;
            ray->map_y += step_y;
            ray->side = 1;
        }
        
        if (ray->map_x < 0 || ray->map_x >= MAP_WIDTH || 
            ray->map_y < 0 || ray->map_y >= MAP_HEIGHT) {
            break;
        }
        
        int tile = map_get_tile(&engine->world, ray->map_x, ray->map_y);
        
        // Check for doors
        for (int i = 0; i < engine->world.door_count; i++) {
            Door* door = &engine->world.doors[i];
            if (door->x == ray->map_x && door->y == ray->map_y) {
                float door_pos = door->horizontal ? 
                    (ray->origin.y + ray->direction.y * side_dist_x) : 
                    (ray->origin.x + ray->direction.x * side_dist_y);
                door_pos = door_pos - (int)door_pos;
                
                if (door_pos < door->open_amount) {
                    ray->hit_door = true;
                    ray->door_state = (int)(door->open_amount * 100);
                    ray->texture_id = door->texture_id;
                    hit = true;
                }
                break;
            }
        }
        
        if (tile > 0) {
            hit = true;
            ray->texture_id = engine->world.wall_textures[ray->map_y][ray->map_x];
        }
    }
    
    // Calculate perpendicular distance to avoid fisheye effect
    if (ray->side == 0) {
        ray->perpendicular_distance = (ray->map_x - ray->origin.x + 
                                       (1 - step_x) / 2) / ray->direction.x;
    } else {
        ray->perpendicular_distance = (ray->map_y - ray->origin.y + 
                                       (1 - step_y) / 2) / ray->direction.y;
    }
    
    ray->distance = ray->perpendicular_distance;
    
    // Calculate hit point for texture mapping
    if (ray->side == 0) {
        ray->hit_point.x = ray->origin.x + ray->perpendicular_distance * ray->direction.x;
        ray->hit_point.y = ray->origin.y + ray->perpendicular_distance * ray->direction.y;
        ray->texture_x = ray->hit_point.y;
    } else {
        ray->hit_point.x = ray->origin.x + ray->perpendicular_distance * ray->direction.x;
        ray->hit_point.y = ray->origin.y + ray->perpendicular_distance * ray->direction.y;
        ray->texture_x = ray->hit_point.x;
    }
    
    ray->texture_x -= floorf(ray->texture_x);
    ray->z_height = map_get_floor_height(&engine->world, ray->map_x, ray->map_y);
}

void raycast_floor_ceiling(Engine* engine, int y, int x) {
    float ray_dir_x0 = engine->camera.direction.x - engine->camera.plane.x;
    float ray_dir_y0 = engine->camera.direction.y - engine->camera.plane.y;
    float ray_dir_x1 = engine->camera.direction.x + engine->camera.plane.x;
    float ray_dir_y1 = engine->camera.direction.y + engine->camera.plane.y;
    
    int p = y - SCREEN_HEIGHT / 2;
    float pos_z = 0.5f * SCREEN_HEIGHT + engine->camera.z_position * SCREEN_HEIGHT;
    float row_distance = pos_z / (float)p;
    
    float floor_step_x = row_distance * (ray_dir_x1 - ray_dir_x0) / SCREEN_WIDTH;
    float floor_step_y = row_distance * (ray_dir_y1 - ray_dir_y0) / SCREEN_WIDTH;
    
    float floor_x = engine->camera.position.x + row_distance * ray_dir_x0;
    float floor_y = engine->camera.position.y + row_distance * ray_dir_y0;
    
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int cell_x = (int)floor_x;
        int cell_y = (int)floor_y;
        
        if (cell_x >= 0 && cell_x < MAP_WIDTH && cell_y >= 0 && cell_y < MAP_HEIGHT) {
            int floor_tex = engine->world.floor_textures[cell_y][cell_x];
            int ceiling_tex = engine->world.ceiling_textures[cell_y][cell_x];
            
            float tx = floor_x - cell_x;
            float ty = floor_y - cell_y;
            
            // Render floor
            if (floor_tex >= 0 && floor_tex < engine->texture_count) {
                Color color = texture_sample_bilinear(&engine->textures[floor_tex], tx, ty);
                int idx = y * SCREEN_WIDTH + x;
                engine->buffers.color_buffer[idx] = color_to_uint32(color);
                engine->buffers.z_buffer[x] = row_distance;
            }
            
            // Render ceiling
            int ceiling_y = SCREEN_HEIGHT - y - 1;
            if (ceiling_tex >= 0 && ceiling_tex < engine->texture_count) {
                Color color = texture_sample_bilinear(&engine->textures[ceiling_tex], tx, ty);
                int idx = ceiling_y * SCREEN_WIDTH + x;
                engine->buffers.color_buffer[idx] = color_to_uint32(color);
            }
        }
        
        floor_x += floor_step_x;
        floor_y += floor_step_y;
    }
}

void render_textured_wall(Engine* engine, int x, Ray* ray) {
    int line_height = (int)(SCREEN_HEIGHT / ray->perpendicular_distance);
    
    int draw_start = -line_height / 2 + SCREEN_HEIGHT / 2 + 
                     (int)(engine->camera.pitch * SCREEN_HEIGHT) +
                     (int)(engine->camera.bob_offset);
    
    int draw_end = line_height / 2 + SCREEN_HEIGHT / 2 + 
                   (int)(engine->camera.pitch * SCREEN_HEIGHT) +
                   (int)(engine->camera.bob_offset);
    
    if (draw_start < 0) draw_start = 0;
    if (draw_end >= SCREEN_HEIGHT) draw_end = SCREEN_HEIGHT - 1;
    
    if (ray->texture_id < 0 || ray->texture_id >= engine->texture_count) {
        return;
    }
    
    Texture* tex = &engine->textures[ray->texture_id];
    int tex_x = (int)(ray->texture_x * tex->width);
    
    if (ray->side == 0 && ray->direction.x > 0) tex_x = tex->width - tex_x - 1;
    if (ray->side == 1 && ray->direction.y < 0) tex_x = tex->width - tex_x - 1;
    
    float step = 1.0f * tex->height / line_height;
    float tex_pos = (draw_start - (int)(engine->camera.pitch * SCREEN_HEIGHT) - 
                    (int)(engine->camera.bob_offset) - SCREEN_HEIGHT / 2 + 
                    line_height / 2) * step;
    
    for (int y = draw_start; y < draw_end; y++) {
        int tex_y = (int)tex_pos & (tex->height - 1);
        tex_pos += step;
        
        Color color = texture_sample(tex, tex_x / (float)tex->width, 
                                     tex_y / (float)tex->height);
        
        // Apply distance-based shading
        float shade = 1.0f / (1.0f + ray->perpendicular_distance * 0.1f);
        color.r = (uint8_t)(color.r * shade);
        color.g = (uint8_t)(color.g * shade);
        color.b = (uint8_t)(color.b * shade);
        
        // Side darkening
        if (ray->side == 1) {
            color.r = (uint8_t)(color.r * 0.7f);
            color.g = (uint8_t)(color.g * 0.7f);
            color.b = (uint8_t)(color.b * 0.7f);
        }
        
        engine->buffers.color_buffer[y * SCREEN_WIDTH + x] = color_to_uint32(color);
    }
    
    engine->buffers.z_buffer[x] = ray->perpendicular_distance;
}

void engine_render(Engine* engine) {
    // Clear buffers
    memset(engine->buffers.color_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        engine->buffers.z_buffer[i] = MAX_RENDER_DISTANCE;
    }
    
    // Render floor and ceiling
    for (int y = SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT; y++) {
        raycast_floor_ceiling(engine, y, 0);
    }
    
    // Render walls using DDA raycasting
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        Ray ray = {0};
        raycast_dda(engine, x, &ray);
        
        if (ray.distance < MAX_RENDER_DISTANCE) {
            render_textured_wall(engine, x, &ray);
        }
    }
    
    // Render sprites (sorted by distance)
    sprite_sort_by_distance(engine->sprites, engine->sprite_count, engine->camera.position);
    render_sprites(engine);
    
    // Render particles
    particle_render(engine);
    
    // Apply lighting
    apply_lighting(engine);
    
    // Apply shadows
    apply_shadows(engine);
    
    // Apply fog
    apply_fog(engine);
    
    // Post-processing pipeline
    if (engine->post_fx.bloom_enabled) {
        post_process_bloom(engine);
    }
    
    if (engine->post_fx.motion_blur_enabled) {
        post_process_motion_blur(engine);
    }
    
    if (engine->post_fx.chromatic_aberration) {
        post_process_chromatic_aberration(engine);
    }
    
    post_process_tone_mapping(engine);
    
    if (engine->post_fx.vignette) {
        post_process_vignette(engine);
    }
    
    if (engine->post_fx.fxaa_enabled) {
        post_process_fxaa(engine);
    }
}
