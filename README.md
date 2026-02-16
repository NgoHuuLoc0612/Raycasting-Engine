# Raycasting Engine

3D raycasting engine written in C/C++ with advanced features and optimizations.

## Architecture Overview

This is a raycasting engine implementing computer graphics techniques:

### Core Systems

**Raycasting Engine** (`engine.c`)
- Digital Differential Analyzer (DDA) algorithm for wall detection
- Perpendicular distance calculation to eliminate fisheye distortion
- Floor and ceiling rendering with perspective-correct texture mapping
- Multi-threaded rendering pipeline support
- Frustum and occlusion culling for performance optimization

**Advanced Rendering Pipeline** (`rendering.c`)
- Dynamic point light system with distance-based attenuation
- Real-time shadow mapping using ray marching
- Volumetric fog with exponential falloff
- Post-processing stack:
  - Bloom with Gaussian blur (5-tap kernel)
  - Motion blur based on camera velocity
  - Chromatic aberration
  - Fast Approximate Anti-Aliasing (FXAA)
  - Tone mapping (Reinhard operator)
  - Vignette effect
  - Gamma correction

**Physics System** (`physics.c`)
- Circle-to-square collision detection
- Swept collision detection for fast-moving objects
- Collision resolution with penetration correction
- Friction and bounce physics
- Player controller with smooth movement
- Door interaction system with open/close animations

**Math Library** (`math.c`)
- Vector operations (Vec2, Vec3)
- Matrix operations (4x4 transformations)
- Quaternion rotations
- Fast approximations (fast_sqrt, fast_sin, fast_cos using Bhaskara I)
- Perlin noise (3D implementation)
- Simplex noise (2D implementation)

**Particle System** (`particles.c`)
- GPU-instanced particle rendering
- Physics-based particle simulation
- Gravity and air resistance
- Alpha blending with depth sorting
- Emitter system with customizable properties

**Texture System** (`particles.c`)
- Bilinear texture filtering
- Trilinear filtering with mipmap support
- Normal mapping for enhanced detail
- Specular mapping
- Emission mapping for glowing surfaces
- Procedural texture generation

**Map Generation** (`map.c`)
- Binary Space Partitioning (BSP) for dungeon generation
- Cellular automata for cave systems
- Recursive backtracking for maze generation
- Procedural height variation using Perlin noise
- Door placement algorithm

### Features

1. **Dynamic Lighting**
   - Multiple point lights with configurable intensity, color, radius
   - Shadow casting with ray marching
   - Light flickering effects
   - Diffuse and specular calculations

2. **Multi-Layer Rendering**
   - Wall rendering with texture mapping
   - Floor and ceiling rendering
   - Sprite system with billboarding
   - Particle effects
   - Transparent surface handling

3. **Camera System**
   - 6 degrees of freedom movement
   - Mouse look with pitch/yaw control
   - Head bobbing animation
   - Crouching mechanics
   - Physics-based movement with inertia

4. **Post-Processing Effects**
   - HDR rendering with tone mapping
   - Bloom for bright surfaces
   - Motion blur for fast movement
   - Anti-aliasing for smooth edges
   - Color grading and vignette

5. **Optimization Techniques**
   - Z-buffer for proper depth sorting
   - Spatial partitioning for collision detection
   - Level of Detail (LOD) system
   - Frustum culling
   - Occlusion culling
   - Dirty rectangle optimization

## Technical Specifications

- **Resolution**: 1280x720 (configurable)
- **Field of View**: 60° (configurable)
- **Render Distance**: 50 units (configurable)
- **Texture Resolution**: 64x64 (supports arbitrary sizes)
- **Max Textures**: 32
- **Max Lights**: 16
- **Max Sprites**: 256
- **Max Particles**: 2048
- **Physics Substeps**: 4 per frame

## Build Requirements

### Windows (MinGW64)
```bash
# Install MinGW64 and SDL2
# Download SDL2 development libraries from libsdl.org
make
```

### Linux
```bash
sudo apt-get update
sudo apt-get install build-essential libsdl2-dev
make
```

### macOS
```bash
brew install sdl2
make
```

## Compilation Options

```bash
make              # Release build (optimized)
make DEBUG=1      # Debug build with symbols
make PROFILE=1    # Enable gprof profiling
make SANITIZE=1   # Enable AddressSanitizer and UBSan
make clean        # Remove build artifacts
make run          # Build and run
```

## Usage

### Controls
- **WASD** - Move camera
- **Mouse** - Look around
- **E** - Interact with doors
- **SPACE** - Emit particle burst
- **CTRL** - Crouch
- **B** - Toggle bloom effect
- **M** - Toggle motion blur
- **V** - Toggle vignette
- **F** - Toggle FXAA
- **ESC** - Quit

### Configuration

Edit `include/engine.h` to modify:
- Screen resolution
- FOV and render distance
- Maximum entities
- Texture sizes
- Physics parameters

## Performance Optimization

### Compiler Optimizations
- **-O3**: Maximum optimization level
- **-march=native**: CPU-specific optimizations
- **-ffast-math**: Fast floating-point math
- **-flto**: Link-time optimization

### Runtime Optimizations
- DDA algorithm: O(n) complexity for ray traversal
- Spatial hashing for collision detection
- Dirty rectangle rendering
- Early ray termination
- Hardware-accelerated texture mapping via SDL2

## Algorithm Details

### DDA (Digital Differential Analyzer)
Efficient grid traversal algorithm that steps through map tiles along a ray:
1. Calculate ray direction from camera
2. Determine step direction and initial side distances
3. Iterate through grid cells until wall hit
4. Calculate perpendicular distance for correct wall height

### Shadow Mapping
Ray marching from surface to light source:
1. Cast ray from hit point towards light
2. March along ray checking for occluders
3. Accumulate shadow factor
4. Apply to final pixel color

### Bloom Effect
Two-pass Gaussian blur:
1. Extract bright pixels above threshold
2. Horizontal 5-tap Gaussian blur
3. Vertical 5-tap Gaussian blur
4. Additive blend with original image

### Particle Physics
Verlet integration with constraints:
1. Update position using velocity
2. Apply forces (gravity, drag)
3. Resolve collisions
4. Update velocity from position change

## Project Structure

```
Raycasting_Engine/
├── include/
│   └── engine.h          # All headers and declarations
├── src/
│   ├── engine.c          # Core raycasting implementation
│   ├── math.c            # Vector/matrix math library
│   ├── rendering.c       # Advanced rendering pipeline
│   ├── physics.c         # Physics and collision system
│   ├── particles.c       # Particle and texture system
│   ├── map.c             # Procedural map generation
│   └── main.c            # Application entry point
├── Makefile              # Build system
└── README.md             # This file
```

## Memory Management

- **Stack Allocation**: Camera, temporary calculations
- **Heap Allocation**: Textures, render buffers, particles
- **Memory Pool**: Pre-allocated entity pools for cache efficiency
- **RAII Pattern**: Automatic cleanup in engine_cleanup()

## Algorithmic Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Ray casting | O(n) | n = steps to wall |
| Collision detection | O(k) | k = nearby tiles |
| Sprite sorting | O(m log m) | m = visible sprites |
| Particle update | O(p) | p = active particles |
| Shadow calculation | O(l × w × h) | l = lights, w×h = screen |

## Known Limitations

- No support for curved surfaces (requires different rendering technique)
- Limited to axis-aligned walls (core raycasting constraint)
- Shadow quality depends on march step size
- Particle count affects performance significantly

## Future Enhancements

- Multi-threaded rendering
- Compute shader acceleration
- Physically-based rendering (PBR)
- Global illumination approximation
- Audio system integration
- Network multiplayer support
- Level editor with GUI
- Script system for game logic

## License: MIT

## Contributing

This is a reference implementation. Key areas for optimization:
1. SIMD vectorization for math operations
2. Multi-threaded ray casting
3. GPU compute shader integration
4. Advanced spatial partitioning (octree)
5. Deferred rendering pipeline

## References

- Lode's Computer Graphics Tutorial (raycasting fundamentals)
- Real-Time Rendering, 4th Edition (advanced techniques)
- Game Engine Architecture (system design)
- Computer Graphics: Principles and Practice (algorithms)
