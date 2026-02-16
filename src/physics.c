#include "../include/engine.h"
#include <math.h>

#define GRAVITY -9.81f
#define TERMINAL_VELOCITY -20.0f

bool physics_check_collision(Engine* engine, Vec2 position, float radius) {
    // Check against map tiles
    int min_x = (int)(position.x - radius);
    int max_x = (int)(position.x + radius);
    int min_y = (int)(position.y - radius);
    int max_y = (int)(position.y + radius);
    
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
                return true; // Collision with map boundaries
            }
            
            int tile = map_get_tile(&engine->world, x, y);
            if (tile > 0) {
                // Check circle-square collision
                float closest_x = position.x;
                float closest_y = position.y;
                
                if (position.x < x) closest_x = (float)x;
                else if (position.x > x + 1) closest_x = (float)(x + 1);
                
                if (position.y < y) closest_y = (float)y;
                else if (position.y > y + 1) closest_y = (float)(y + 1);
                
                float dx = position.x - closest_x;
                float dy = position.y - closest_y;
                float dist_sq = dx * dx + dy * dy;
                
                if (dist_sq < radius * radius) {
                    return true;
                }
            }
        }
    }
    
    // Check against doors
    for (int i = 0; i < engine->world.door_count; i++) {
        if (door_check_collision(&engine->world.doors[i], position)) {
            return true;
        }
    }
    
    return false;
}

void physics_resolve_collision(PhysicsBody* body, Vec2 normal, float penetration) {
    // Move body out of collision
    body->position.x += normal.x * penetration;
    body->position.y += normal.y * penetration;
    
    // Reflect velocity along normal
    float dot = vec2_dot(body->velocity, normal);
    if (dot < 0.0f) {
        body->velocity.x -= normal.x * dot * (1.0f + body->bounce);
        body->velocity.y -= normal.y * dot * (1.0f + body->bounce);
    }
}

void physics_apply_gravity(PhysicsBody* body, float delta_time) {
    if (!body->affected_by_gravity) return;
    
    body->velocity.y += GRAVITY * delta_time;
    
    if (body->velocity.y < TERMINAL_VELOCITY) {
        body->velocity.y = TERMINAL_VELOCITY;
    }
}

void physics_update(Engine* engine, PhysicsBody* body, float delta_time) {
    // Store previous position for collision detection
    Vec2 old_position = body->position;
    
    // Apply friction
    body->velocity.x *= body->friction;
    body->velocity.y *= body->friction;
    
    // Update position
    body->position.x += body->velocity.x * delta_time;
    body->position.y += body->velocity.y * delta_time;
    
    // Check and resolve collisions
    if (physics_check_collision(engine, body->position, body->radius)) {
        // Try X axis only
        body->position.y = old_position.y;
        if (physics_check_collision(engine, body->position, body->radius)) {
            // Try Y axis only
            body->position.x = old_position.x;
            body->position.y = old_position.y + body->velocity.y * delta_time;
            
            if (physics_check_collision(engine, body->position, body->radius)) {
                // Can't move anywhere, restore position
                body->position = old_position;
                body->velocity.x = 0.0f;
                body->velocity.y = 0.0f;
            } else {
                // Y movement successful, stop X velocity
                body->velocity.x = 0.0f;
            }
        } else {
            // X movement successful, stop Y velocity
            body->velocity.y = 0.0f;
        }
    }
    
    // Additional circle-based collision resolution
    int tile_x = (int)body->position.x;
    int tile_y = (int)body->position.y;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int check_x = tile_x + dx;
            int check_y = tile_y + dy;
            
            if (check_x < 0 || check_x >= MAP_WIDTH || 
                check_y < 0 || check_y >= MAP_HEIGHT) continue;
            
            int tile = map_get_tile(&engine->world, check_x, check_y);
            if (tile > 0) {
                // Find closest point on tile to circle center
                float closest_x = body->position.x;
                float closest_y = body->position.y;
                
                if (body->position.x < check_x) closest_x = (float)check_x;
                else if (body->position.x > check_x + 1) closest_x = (float)(check_x + 1);
                
                if (body->position.y < check_y) closest_y = (float)check_y;
                else if (body->position.y > check_y + 1) closest_y = (float)(check_y + 1);
                
                Vec2 to_circle = vec2_sub(body->position, (Vec2){closest_x, closest_y});
                float dist = vec2_length(to_circle);
                
                if (dist < body->radius && dist > 0.001f) {
                    Vec2 normal = vec2_normalize(to_circle);
                    float penetration = body->radius - dist;
                    physics_resolve_collision(body, normal, penetration);
                }
            }
        }
    }
}

// Camera-specific movement functions
void camera_move_forward(Camera* cam, float distance) {
    cam->physics.velocity.x += cam->direction.x * distance;
    cam->physics.velocity.y += cam->direction.y * distance;
}

void camera_move_backward(Camera* cam, float distance) {
    cam->physics.velocity.x -= cam->direction.x * distance;
    cam->physics.velocity.y -= cam->direction.y * distance;
}

void camera_strafe_left(Camera* cam, float distance) {
    Vec2 left = {-cam->direction.y, cam->direction.x};
    cam->physics.velocity.x += left.x * distance;
    cam->physics.velocity.y += left.y * distance;
}

void camera_strafe_right(Camera* cam, float distance) {
    Vec2 right = {cam->direction.y, -cam->direction.x};
    cam->physics.velocity.x += right.x * distance;
    cam->physics.velocity.y += right.y * distance;
}

void camera_rotate(Camera* cam, float angle) {
    // Rotate direction vector
    float old_dir_x = cam->direction.x;
    cam->direction.x = cam->direction.x * cosf(angle) - cam->direction.y * sinf(angle);
    cam->direction.y = old_dir_x * sinf(angle) + cam->direction.y * cosf(angle);
    
    // Rotate plane vector
    float old_plane_x = cam->plane.x;
    cam->plane.x = cam->plane.x * cosf(angle) - cam->plane.y * sinf(angle);
    cam->plane.y = old_plane_x * sinf(angle) + cam->plane.y * cosf(angle);
}

void camera_look_up(Camera* cam, float angle) {
    cam->pitch += angle;
    if (cam->pitch > 1.0f) cam->pitch = 1.0f;
}

void camera_look_down(Camera* cam, float angle) {
    cam->pitch -= angle;
    if (cam->pitch < -1.0f) cam->pitch = -1.0f;
}

void camera_crouch(Camera* cam, bool state) {
    cam->crouching = state;
    if (state) {
        cam->z_position = 0.3f;
    } else {
        cam->z_position = 0.5f;
    }
}

void camera_update_headbob(Camera* cam, float delta_time, bool moving) {
    if (moving) {
        cam->bob_phase += delta_time * 8.0f;
        cam->bob_offset = sinf(cam->bob_phase) * 5.0f;
    } else {
        // Smoothly return to center
        cam->bob_offset *= 0.9f;
        if (fabsf(cam->bob_offset) < 0.1f) {
            cam->bob_offset = 0.0f;
        }
    }
}

// Door system implementation
void door_open(Door* door) {
    if (!door->is_opening && door->open_amount < 1.0f) {
        door->is_opening = true;
        door->is_closing = false;
    }
}

void door_close(Door* door) {
    if (!door->is_closing && door->open_amount > 0.0f) {
        door->is_closing = true;
        door->is_opening = false;
    }
}

void door_update(Door* door, float delta_time) {
    const float DOOR_SPEED = 2.0f;
    
    if (door->is_opening) {
        door->open_amount += DOOR_SPEED * delta_time;
        if (door->open_amount >= 1.0f) {
            door->open_amount = 1.0f;
            door->is_opening = false;
        }
    }
    
    if (door->is_closing) {
        door->open_amount -= DOOR_SPEED * delta_time;
        if (door->open_amount <= 0.0f) {
            door->open_amount = 0.0f;
            door->is_closing = false;
        }
    }
}

bool door_check_collision(Door* door, Vec2 position) {
    if (door->open_amount >= 0.9f) return false;
    
    float dx = fabsf(position.x - (door->x + 0.5f));
    float dy = fabsf(position.y - (door->y + 0.5f));
    
    if (door->horizontal) {
        return dx < 0.5f && dy < (1.0f - door->open_amount) * 0.5f;
    } else {
        return dy < 0.5f && dx < (1.0f - door->open_amount) * 0.5f;
    }
}

// Sprite sorting by distance (painter's algorithm)
static int sprite_compare(const void* a, const void* b) {
    const Sprite* sa = (const Sprite*)a;
    const Sprite* sb = (const Sprite*)b;
    
    // Note: camera position is passed via global for qsort
    // This is a simplified version - production code should use a better approach
    float dist_a = (sa->position.x - 0) * (sa->position.x - 0) + 
                   (sa->position.y - 0) * (sa->position.y - 0);
    float dist_b = (sb->position.x - 0) * (sb->position.x - 0) + 
                   (sb->position.y - 0) * (sb->position.y - 0);
    
    return (dist_b > dist_a) - (dist_b < dist_a);
}

void sprite_sort_by_distance(Sprite* sprites, int count, Vec2 camera_pos) {
    // Bubble sort for stability and simplicity
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            float dist_a = (sprites[j].position.x - camera_pos.x) * 
                          (sprites[j].position.x - camera_pos.x) +
                          (sprites[j].position.y - camera_pos.y) * 
                          (sprites[j].position.y - camera_pos.y);
            
            float dist_b = (sprites[j + 1].position.x - camera_pos.x) * 
                          (sprites[j + 1].position.x - camera_pos.x) +
                          (sprites[j + 1].position.y - camera_pos.y) * 
                          (sprites[j + 1].position.y - camera_pos.y);
            
            if (dist_a < dist_b) {
                Sprite temp = sprites[j];
                sprites[j] = sprites[j + 1];
                sprites[j + 1] = temp;
            }
        }
    }
}

void sprite_animate(Sprite* sprite, float delta_time) {
    if (sprite->animation_speed > 0.0f) {
        sprite->animation_frame += (int)(sprite->animation_speed * delta_time * 10.0f);
    }
}

void render_sprites(Engine* engine) {
    for (int i = 0; i < engine->sprite_count; i++) {
        Sprite* sprite = &engine->sprites[i];
        
        // Transform sprite position to camera space
        Vec2 sprite_pos = vec2_sub(sprite->position, engine->camera.position);
        
        float inv_det = 1.0f / (engine->camera.plane.x * engine->camera.direction.y - 
                                engine->camera.direction.x * engine->camera.plane.y);
        
        Vec2 transform;
        transform.x = inv_det * (engine->camera.direction.y * sprite_pos.x - 
                                 engine->camera.direction.x * sprite_pos.y);
        transform.y = inv_det * (-engine->camera.plane.y * sprite_pos.x + 
                                 engine->camera.plane.x * sprite_pos.y);
        
        // Skip if behind camera
        if (transform.y <= 0.0f) continue;
        
        int sprite_screen_x = (int)((SCREEN_WIDTH / 2) * (1 + transform.x / transform.y));
        
        int sprite_height = abs((int)(SCREEN_HEIGHT / transform.y)) * sprite->scale.y;
        int sprite_width = abs((int)(SCREEN_HEIGHT / transform.y)) * sprite->scale.x;
        
        int draw_start_y = -sprite_height / 2 + SCREEN_HEIGHT / 2;
        int draw_end_y = sprite_height / 2 + SCREEN_HEIGHT / 2;
        int draw_start_x = -sprite_width / 2 + sprite_screen_x;
        int draw_end_x = sprite_width / 2 + sprite_screen_x;
        
        if (draw_start_y < 0) draw_start_y = 0;
        if (draw_end_y >= SCREEN_HEIGHT) draw_end_y = SCREEN_HEIGHT - 1;
        if (draw_start_x < 0) draw_start_x = 0;
        if (draw_end_x >= SCREEN_WIDTH) draw_end_x = SCREEN_WIDTH - 1;
        
        // Render sprite columns
        for (int x = draw_start_x; x < draw_end_x; x++) {
            if (transform.y < engine->buffers.z_buffer[x]) {
                for (int y = draw_start_y; y < draw_end_y; y++) {
                    int idx = y * SCREEN_WIDTH + x;
                    
                    // Simple sprite rendering (would need texture sampling in full implementation)
                    Color color = {255, 255, 255, 255};
                    color.r = (uint8_t)(color.r * sprite->tint.r);
                    color.g = (uint8_t)(color.g * sprite->tint.g);
                    color.b = (uint8_t)(color.b * sprite->tint.b);
                    
                    engine->buffers.color_buffer[idx] = color_to_uint32(color);
                }
            }
        }
    }
}
