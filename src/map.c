#include "../include/engine.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Simple pseudo-random number generator for consistent map generation
static uint32_t map_seed = 0;

static uint32_t rand_lcg(void) {
    map_seed = (map_seed * 1103515245 + 12345) & 0x7fffffff;
    return map_seed;
}

static float rand_float(void) {
    return rand_lcg() / (float)0x7fffffff;
}

static int rand_range(int min, int max) {
    return min + (rand_lcg() % (max - min + 1));
}

int map_get_tile(WorldMap* map, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 1; // Out of bounds = wall
    }
    return map->tiles[y][x];
}

void map_set_tile(WorldMap* map, int x, int y, int value) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        map->tiles[y][x] = value;
    }
}

float map_get_floor_height(WorldMap* map, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 0.0f;
    }
    return map->floor_heights[y][x];
}

float map_get_ceiling_height(WorldMap* map, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 1.0f;
    }
    return map->ceiling_heights[y][x];
}

void map_load_from_file(WorldMap* map, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        // If file doesn't exist, generate procedural map
        map_generate_procedural(map, (uint32_t)time(NULL));
        return;
    }
    
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int tile;
            if (fscanf(file, "%d", &tile) == 1) {
                map->tiles[y][x] = tile;
            }
        }
    }
    
    fclose(file);
}

// Cellular automata for cave generation
static void cellular_automata_step(int tiles[MAP_HEIGHT][MAP_WIDTH]) {
    int temp[MAP_HEIGHT][MAP_WIDTH];
    memcpy(temp, tiles, sizeof(int) * MAP_HEIGHT * MAP_WIDTH);
    
    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 1; x < MAP_WIDTH - 1; x++) {
            int wall_count = 0;
            
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    if (tiles[y + dy][x + dx] > 0) wall_count++;
                }
            }
            
            // Birth/survival rules: B5678/S5678 (cave-like)
            if (tiles[y][x] > 0) {
                temp[y][x] = (wall_count >= 4) ? 1 : 0;
            } else {
                temp[y][x] = (wall_count >= 5) ? 1 : 0;
            }
        }
    }
    
    memcpy(tiles, temp, sizeof(int) * MAP_HEIGHT * MAP_WIDTH);
}

// BSP (Binary Space Partitioning) for dungeon generation
typedef struct {
    int x, y, w, h;
} BSPRoom;

static void bsp_split(BSPRoom rooms[], int* room_count, int max_rooms, 
                     int x, int y, int w, int h, int depth) {
    if (depth == 0 || *room_count >= max_rooms || w < 8 || h < 8) {
        if (*room_count < max_rooms) {
            rooms[*room_count].x = x + 1;
            rooms[*room_count].y = y + 1;
            rooms[*room_count].w = w - 2;
            rooms[*room_count].h = h - 2;
            (*room_count)++;
        }
        return;
    }
    
    bool vertical = (rand_float() > 0.5f) || (w < h);
    
    if (vertical && h >= 8) {
        int split = rand_range(4, h - 4);
        bsp_split(rooms, room_count, max_rooms, x, y, w, split, depth - 1);
        bsp_split(rooms, room_count, max_rooms, x, y + split, w, h - split, depth - 1);
    } else if (!vertical && w >= 8) {
        int split = rand_range(4, w - 4);
        bsp_split(rooms, room_count, max_rooms, x, y, split, h, depth - 1);
        bsp_split(rooms, room_count, max_rooms, x + split, y, w - split, h, depth - 1);
    }
}

static void carve_room(WorldMap* map, BSPRoom* room) {
    for (int y = room->y; y < room->y + room->h; y++) {
        for (int x = room->x; x < room->x + room->w; x++) {
            if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                map->tiles[y][x] = 0;
            }
        }
    }
}

static void carve_corridor(WorldMap* map, int x1, int y1, int x2, int y2) {
    // L-shaped corridor
    for (int x = (x1 < x2 ? x1 : x2); x <= (x1 > x2 ? x1 : x2); x++) {
        if (x >= 0 && x < MAP_WIDTH && y1 >= 0 && y1 < MAP_HEIGHT) {
            map->tiles[y1][x] = 0;
        }
    }
    
    for (int y = (y1 < y2 ? y1 : y2); y <= (y1 > y2 ? y1 : y2); y++) {
        if (x2 >= 0 && x2 < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
            map->tiles[y][x2] = 0;
        }
    }
}

void map_generate_procedural(WorldMap* map, uint32_t seed) {
    map_seed = seed;
    map->door_count = 0;
    
    // Initialize everything to walls
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map->tiles[y][x] = 1;
            map->floor_heights[y][x] = 0.0f;
            map->ceiling_heights[y][x] = 1.0f;
            map->floor_textures[y][x] = 0;
            map->ceiling_textures[y][x] = 0;
            map->wall_textures[y][x] = 0;
        }
    }
    
    // Choose generation algorithm
    int algo = rand_range(0, 2);
    
    if (algo == 0) {
        // BSP dungeon generation
        BSPRoom rooms[64];
        int room_count = 0;
        
        bsp_split(rooms, &room_count, 64, 0, 0, MAP_WIDTH, MAP_HEIGHT, 4);
        
        // Carve rooms
        for (int i = 0; i < room_count; i++) {
            carve_room(map, &rooms[i]);
        }
        
        // Connect rooms with corridors
        for (int i = 0; i < room_count - 1; i++) {
            int x1 = rooms[i].x + rooms[i].w / 2;
            int y1 = rooms[i].y + rooms[i].h / 2;
            int x2 = rooms[i + 1].x + rooms[i + 1].w / 2;
            int y2 = rooms[i + 1].y + rooms[i + 1].h / 2;
            
            carve_corridor(map, x1, y1, x2, y2);
        }
        
    } else if (algo == 1) {
        // Cellular automata caves
        // Random fill
        for (int y = 1; y < MAP_HEIGHT - 1; y++) {
            for (int x = 1; x < MAP_WIDTH - 1; x++) {
                map->tiles[y][x] = (rand_float() < 0.45f) ? 1 : 0;
            }
        }
        
        // Apply cellular automata
        for (int i = 0; i < 5; i++) {
            cellular_automata_step(map->tiles);
        }
        
    } else {
        // Maze generation using recursive backtracking
        // Start with all walls except border
        for (int y = 2; y < MAP_HEIGHT - 2; y += 2) {
            for (int x = 2; x < MAP_WIDTH - 2; x += 2) {
                map->tiles[y][x] = 0;
            }
        }
        
        // Randomly knock down walls to create passages
        for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT / 4; i++) {
            int x = rand_range(1, MAP_WIDTH - 2);
            int y = rand_range(1, MAP_HEIGHT - 2);
            map->tiles[y][x] = 0;
        }
    }
    
    // Add some doors
    for (int i = 0; i < 10; i++) {
        int x = rand_range(1, MAP_WIDTH - 2);
        int y = rand_range(1, MAP_HEIGHT - 2);
        
        // Check if this is a good door position (wall with open spaces)
        if (map->tiles[y][x] == 1) {
            int neighbors = 0;
            if (map->tiles[y-1][x] == 0) neighbors++;
            if (map->tiles[y+1][x] == 0) neighbors++;
            if (map->tiles[y][x-1] == 0) neighbors++;
            if (map->tiles[y][x+1] == 0) neighbors++;
            
            if (neighbors == 2 && map->door_count < 64) {
                Door* door = &map->doors[map->door_count++];
                door->x = x;
                door->y = y;
                door->open_amount = 0.0f;
                door->is_opening = false;
                door->is_closing = false;
                door->texture_id = 0;
                door->horizontal = (map->tiles[y][x-1] == 0 && map->tiles[y][x+1] == 0);
                
                map->tiles[y][x] = 0; // Make it passable
            }
        }
    }
    
    // Add height variation using Perlin noise
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (map->tiles[y][x] == 0) {
                float noise = perlin_noise_2d(x * 0.1f, y * 0.1f);
                map->floor_heights[y][x] = noise * 0.1f;
                map->ceiling_heights[y][x] = 1.0f + noise * 0.2f;
            }
        }
    }
    
    // Assign random textures
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            map->wall_textures[y][x] = rand_range(0, 3);
            map->floor_textures[y][x] = rand_range(0, 3);
            map->ceiling_textures[y][x] = rand_range(0, 3);
        }
    }
}

// Optimization utilities (stubs for enterprise features)
void optimize_frustum_culling(Engine* engine) {
    // Frustum culling would eliminate objects outside view
    // Implementation would use plane equations and dot products
}

void optimize_occlusion_culling(Engine* engine) {
    // Occlusion culling would eliminate objects behind walls
    // Could use portal rendering or hierarchical z-buffer
}

void optimize_lod_system(Engine* engine) {
    // Level of detail system would reduce complexity at distance
    // Switch between high/low poly models and texture mipmaps
}

void optimize_spatial_partitioning(Engine* engine) {
    // Spatial partitioning (quadtree, octree, BSP) for faster queries
    // Reduces collision checks and ray intersection tests
}

// Performance profiling
void profile_begin(ProfileSection* section) {
    // Would use high-resolution timer
    section->start_time = 0;
    section->call_count++;
}

void profile_end(ProfileSection* section) {
    // Would calculate elapsed time
    section->total_time += 0;
}

void profile_reset(ProfileSection* section) {
    section->total_time = 0;
    section->call_count = 0;
}

float profile_get_ms(ProfileSection* section) {
    if (section->call_count == 0) return 0.0f;
    return (float)section->total_time / section->call_count / 1000.0f;
}
