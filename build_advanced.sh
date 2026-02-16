#!/bin/bash
# Optimized build script for Advanced Raycasting Engine with all features

echo "================================"
echo "Advanced Raycasting Engine v2.0"
echo "Building with all features..."
echo "================================"

# Create directories
mkdir -p build bin

# Compile all source files with optimization
echo "Compiling source files..."

gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/engine.c -o build/engine.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/math.c -o build/math.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/rendering.c -o build/rendering.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/physics.c -o build/physics.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/particles.c -o build/particles.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/map.c -o build/map.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/threading.c -o build/threading.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/pbr.c -o build/pbr.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/gi.c -o build/gi.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/audio.c -o build/audio.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/scripting.c -o build/scripting.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/compute.c -o build/compute.o
gcc -Iinclude -O3 -march=native -ffast-math -pthread -c src/main.c -o build/main.o

echo "Linking executable..."

# Link with all necessary libraries
gcc build/*.o -o bin/raycasting_engine.exe \
    -lm -lmingw32 -lSDL2main -lSDL2 -lpthread

if [ $? -eq 0 ]; then
    echo "================================"
    echo "Build successful!"
    echo "Executable: bin/raycasting_engine.exe"
    echo ""
    echo "New Features:"
    echo "  ✓ Multi-threaded rendering"
    echo "  ✓ Physically-Based Rendering (PBR)"
    echo "  ✓ Global Illumination"
    echo "  ✓ Compute shader acceleration"
    echo "  ✓ Audio system (3D spatial audio)"
    echo "  ✓ Scripting system"
    echo "================================"
    echo ""
    echo "Run with: ./bin/raycasting_engine.exe"
else
    echo "Build failed!"
    exit 1
fi
