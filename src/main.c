#include "../include/engine.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

#define TARGET_FPS 60
#define FRAME_TIME (1000.0f / TARGET_FPS)

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* screen_texture;
    bool running;
    bool keys[SDL_NUM_SCANCODES];
    int mouse_dx;
    int mouse_dy;
} Application;

void application_init(Application* app) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(1);
    }
    
    app->window = SDL_CreateWindow(
        "Advanced Raycasting Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!app->window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    
    app->renderer = SDL_CreateRenderer(
        app->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!app->renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        exit(1);
    }
    
    app->screen_texture = SDL_CreateTexture(
        app->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    
    if (!app->screen_texture) {
        fprintf(stderr, "Texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        exit(1);
    }
    
    SDL_SetRelativeMouseMode(SDL_TRUE);
    
    app->running = true;
    app->mouse_dx = 0;
    app->mouse_dy = 0;
    
    for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
        app->keys[i] = false;
    }
}

void application_cleanup(Application* app) {
    SDL_DestroyTexture(app->screen_texture);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}

void application_handle_events(Application* app, Engine* engine) {
    SDL_Event event;
    app->mouse_dx = 0;
    app->mouse_dy = 0;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                app->running = false;
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.scancode < SDL_NUM_SCANCODES) {
                    app->keys[event.key.keysym.scancode] = true;
                }
                
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    app->running = false;
                }
                
                // Toggle features
                if (event.key.keysym.sym == SDLK_b) {
                    engine->post_fx.bloom_enabled = !engine->post_fx.bloom_enabled;
                }
                if (event.key.keysym.sym == SDLK_m) {
                    engine->post_fx.motion_blur_enabled = !engine->post_fx.motion_blur_enabled;
                }
                if (event.key.keysym.sym == SDLK_v) {
                    engine->post_fx.vignette = !engine->post_fx.vignette;
                }
                if (event.key.keysym.sym == SDLK_f) {
                    engine->post_fx.fxaa_enabled = !engine->post_fx.fxaa_enabled;
                }
                
                // Spawn particles
                if (event.key.keysym.sym == SDLK_SPACE) {
                    for (int i = 0; i < 100; i++) {
                        Vec3 pos = {
                            engine->camera.position.x,
                            engine->camera.position.y,
                            engine->camera.z_position
                        };
                        Vec3 vel = {
                            (rand() / (float)RAND_MAX - 0.5f) * 5.0f,
                            (rand() / (float)RAND_MAX - 0.5f) * 5.0f,
                            (rand() / (float)RAND_MAX) * 8.0f
                        };
                        ColorF color = {
                            rand() / (float)RAND_MAX,
                            rand() / (float)RAND_MAX,
                            rand() / (float)RAND_MAX,
                            1.0f
                        };
                        particle_emit(engine, pos, vel, color, 2.0f);
                    }
                }
                break;
                
            case SDL_KEYUP:
                if (event.key.keysym.scancode < SDL_NUM_SCANCODES) {
                    app->keys[event.key.keysym.scancode] = false;
                }
                break;
                
            case SDL_MOUSEMOTION:
                app->mouse_dx = event.motion.xrel;
                app->mouse_dy = event.motion.yrel;
                break;
        }
    }
}

void application_update(Application* app, Engine* engine, float delta_time) {
    const float MOVE_SPEED = 5.0f;
    const float MOUSE_SENSITIVITY = 0.002f;
    
    // Mouse look
    if (app->mouse_dx != 0) {
        camera_rotate(&engine->camera, app->mouse_dx * MOUSE_SENSITIVITY);
    }
    if (app->mouse_dy != 0) {
        float pitch_change = app->mouse_dy * MOUSE_SENSITIVITY;
        if (pitch_change < 0) {
            camera_look_up(&engine->camera, -pitch_change);
        } else {
            camera_look_down(&engine->camera, pitch_change);
        }
    }
    
    // Movement
    if (app->keys[SDL_SCANCODE_W]) {
        camera_move_forward(&engine->camera, MOVE_SPEED * delta_time);
    }
    if (app->keys[SDL_SCANCODE_S]) {
        camera_move_backward(&engine->camera, MOVE_SPEED * delta_time);
    }
    if (app->keys[SDL_SCANCODE_A]) {
        camera_strafe_left(&engine->camera, MOVE_SPEED * delta_time);
    }
    if (app->keys[SDL_SCANCODE_D]) {
        camera_strafe_right(&engine->camera, MOVE_SPEED * delta_time);
    }
    
    // Crouch
    camera_crouch(&engine->camera, app->keys[SDL_SCANCODE_LCTRL]);
    
    // Open/close nearby doors
    if (app->keys[SDL_SCANCODE_E]) {
        int px = (int)engine->camera.position.x;
        int py = (int)engine->camera.position.y;
        
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                for (int i = 0; i < engine->world.door_count; i++) {
                    Door* door = &engine->world.doors[i];
                    if (door->x == px + dx && door->y == py + dy) {
                        if (door->open_amount < 0.5f) {
                            door_open(door);
                        } else {
                            door_close(door);
                        }
                    }
                }
            }
        }
    }
    
    // Update engine
    engine_update(engine, delta_time);
}

void application_render(Application* app, Engine* engine) {
    // Render engine to buffer
    engine_render(engine);
    
    // Update SDL texture
    void* pixels;
    int pitch;
    SDL_LockTexture(app->screen_texture, NULL, &pixels, &pitch);
    memcpy(pixels, engine->buffers.color_buffer, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    SDL_UnlockTexture(app->screen_texture);
    
    // Present to screen
    SDL_RenderClear(app->renderer);
    SDL_RenderCopy(app->renderer, app->screen_texture, NULL, NULL);
    
    // Draw simple FPS counter
    char fps_text[64];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.1f | Frame: %llu", 
             1.0f / engine->delta_time, engine->frame_count);
    
    SDL_SetWindowTitle(app->window, fps_text);
    
    SDL_RenderPresent(app->renderer);
}

int main(int argc, char* argv[]) {
    printf("Advanced Raycasting Engine\n");
    printf("===========================\n");
    printf("Controls:\n");
    printf("  WASD - Move\n");
    printf("  Mouse - Look around\n");
    printf("  E - Open/close doors\n");
    printf("  SPACE - Emit particles\n");
    printf("  CTRL - Crouch\n");
    printf("  B - Toggle bloom\n");
    printf("  M - Toggle motion blur\n");
    printf("  V - Toggle vignette\n");
    printf("  F - Toggle FXAA\n");
    printf("  ESC - Quit\n");
    printf("===========================\n\n");
    
    Application app;
    Engine engine;
    
    application_init(&app);
    engine_init(&engine);
    
    // Generate some procedural textures
    for (int i = 0; i < 4 && engine.texture_count < MAX_TEXTURES; i++) {
        Texture* tex = &engine.textures[engine.texture_count];
        tex->width = TEXTURE_SIZE;
        tex->height = TEXTURE_SIZE;
        tex->pixels = (uint32_t*)malloc(TEXTURE_SIZE * TEXTURE_SIZE * sizeof(uint32_t));
        
        // Generate different patterns
        for (int y = 0; y < TEXTURE_SIZE; y++) {
            for (int x = 0; x < TEXTURE_SIZE; x++) {
                uint8_t r, g, b;
                
                switch (i) {
                    case 0: // Brick pattern
                        r = 150 + (x % 8 < 1 || y % 8 < 1 ? 50 : 0);
                        g = 80 + (x % 8 < 1 || y % 8 < 1 ? 30 : 0);
                        b = 70 + (x % 8 < 1 || y % 8 < 1 ? 20 : 0);
                        break;
                    case 1: // Stone pattern
                        r = g = b = 100 + ((x * y) % 50);
                        break;
                    case 2: // Wood grain
                        r = 139 + (int)(20 * perlin_noise_2d(x * 0.1f, y * 0.5f));
                        g = 90 + (int)(15 * perlin_noise_2d(x * 0.1f, y * 0.5f));
                        b = 60 + (int)(10 * perlin_noise_2d(x * 0.1f, y * 0.5f));
                        break;
                    case 3: // Metal
                        r = g = b = 180 + (int)(30 * perlin_noise_2d(x * 0.2f, y * 0.2f));
                        break;
                }
                
                Color c = {r, g, b, 255};
                tex->pixels[y * TEXTURE_SIZE + x] = color_to_uint32(c);
            }
        }
        
        engine.texture_count++;
    }
    
    // Add some dynamic lights
    if (engine.light_count < MAX_LIGHTS) {
        engine.lights[engine.light_count++] = (Light){
            {10.0f, 10.0f, 2.0f},
            {1.0f, 0.3f, 0.1f, 1.0f},
            8.0f,
            12.0f,
            true,
            0.2f
        };
    }
    
    // Main loop
    Uint32 last_time = SDL_GetTicks();
    
    while (app.running) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        // Cap delta time to prevent huge jumps
        if (delta_time > 0.1f) delta_time = 0.1f;
        
        application_handle_events(&app, &engine);
        application_update(&app, &engine, delta_time);
        application_render(&app, &engine);
        
        // Simple frame rate limiting
        Uint32 frame_time = SDL_GetTicks() - current_time;
        if (frame_time < FRAME_TIME) {
            SDL_Delay((Uint32)(FRAME_TIME - frame_time));
        }
    }
    
    engine_cleanup(&engine);
    application_cleanup(&app);
    
    return 0;
}
