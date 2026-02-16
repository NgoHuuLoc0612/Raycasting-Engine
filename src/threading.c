#include "../include/engine.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void threading_init(ThreadPool* pool) {
    memset(pool, 0, sizeof(ThreadPool));
    pool->use_threading = true;
    pool->job_count = MAX_THREADS;
    
    // Initialize jobs
    for (int i = 0; i < MAX_THREADS; i++) {
        pool->jobs[i].start_column = 0;
        pool->jobs[i].end_column = 0;
        pool->jobs[i].engine = NULL;
        pool->jobs[i].completed = false;
        pool->thread_handles[i] = NULL;
    }
}

void threading_cleanup(ThreadPool* pool) {
    // Wait for all threads to complete
    for (int i = 0; i < pool->job_count; i++) {
        if (pool->thread_handles[i]) {
            pthread_join(*(pthread_t*)pool->thread_handles[i], NULL);
            free(pool->thread_handles[i]);
            pool->thread_handles[i] = NULL;
        }
    }
}

void* threading_render_job(void* arg) {
    RenderJob* job = (RenderJob*)arg;
    Engine* engine = job->engine;
    
    // Render assigned columns
    for (int x = job->start_column; x < job->end_column; x++) {
        Ray ray = {0};
        raycast_dda(engine, x, &ray);
        
        if (ray.distance < MAX_RENDER_DISTANCE) {
            render_textured_wall(engine, x, &ray);
        }
    }
    
    job->completed = true;
    return NULL;
}

void threading_render_parallel(Engine* engine) {
    if (!engine->use_multithreading || !engine->thread_pool.use_threading) {
        // Fallback to single-threaded
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            Ray ray = {0};
            raycast_dda(engine, x, &ray);
            if (ray.distance < MAX_RENDER_DISTANCE) {
                render_textured_wall(engine, x, &ray);
            }
        }
        return;
    }
    
    ThreadPool* pool = &engine->thread_pool;
    int columns_per_thread = SCREEN_WIDTH / pool->job_count;
    
    // Create jobs
    for (int i = 0; i < pool->job_count; i++) {
        pool->jobs[i].start_column = i * columns_per_thread;
        pool->jobs[i].end_column = (i == pool->job_count - 1) ? 
                                    SCREEN_WIDTH : 
                                    (i + 1) * columns_per_thread;
        pool->jobs[i].engine = engine;
        pool->jobs[i].completed = false;
        
        // Create thread
        pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));
        pthread_create(thread, NULL, threading_render_job, &pool->jobs[i]);
        pool->thread_handles[i] = thread;
    }
    
    // Wait for all threads
    for (int i = 0; i < pool->job_count; i++) {
        pthread_join(*(pthread_t*)pool->thread_handles[i], NULL);
        free(pool->thread_handles[i]);
        pool->thread_handles[i] = NULL;
    }
}
