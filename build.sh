#!/bin/bash

# Advanced Raycasting Engine Build Script
# Supports multiple platforms and configurations

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}Advanced Raycasting Engine${NC}"
echo -e "${GREEN}Build Script${NC}"
echo -e "${GREEN}================================${NC}"

# Detect platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "mingw"* ]]; then
    PLATFORM="Windows"
else
    PLATFORM="Unknown"
fi

echo -e "${YELLOW}Platform: $PLATFORM${NC}"

# Parse arguments
BUILD_TYPE="release"
CLEAN=false
RUN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="debug"
            shift
            ;;
        --release)
            BUILD_TYPE="release"
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --run)
            RUN=true
            shift
            ;;
        --help)
            echo "Usage: ./build.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug      Build in debug mode"
            echo "  --release    Build in release mode (default)"
            echo "  --clean      Clean before building"
            echo "  --run        Run after building"
            echo "  --help       Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Check for dependencies
check_dependency() {
    if command -v $1 &> /dev/null; then
        echo -e "${GREEN}✓${NC} $1 found"
        return 0
    else
        echo -e "${RED}✗${NC} $1 not found"
        return 1
    fi
}

echo ""
echo "Checking dependencies..."
DEPS_OK=true

if ! check_dependency "gcc"; then
    DEPS_OK=false
fi

if ! check_dependency "make"; then
    DEPS_OK=false
fi

# Check for SDL2
if [[ "$PLATFORM" == "Linux" ]]; then
    if pkg-config --exists sdl2; then
        echo -e "${GREEN}✓${NC} SDL2 found"
    else
        echo -e "${RED}✗${NC} SDL2 not found"
        echo -e "${YELLOW}Install with: sudo apt-get install libsdl2-dev${NC}"
        DEPS_OK=false
    fi
elif [[ "$PLATFORM" == "macOS" ]]; then
    if brew list sdl2 &>/dev/null; then
        echo -e "${GREEN}✓${NC} SDL2 found"
    else
        echo -e "${RED}✗${NC} SDL2 not found"
        echo -e "${YELLOW}Install with: brew install sdl2${NC}"
        DEPS_OK=false
    fi
fi

if [ "$DEPS_OK" = false ]; then
    echo -e "${RED}Missing dependencies. Please install them first.${NC}"
    exit 1
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo ""
    echo -e "${YELLOW}Cleaning build artifacts...${NC}"
    make clean
fi

# Build
echo ""
echo -e "${YELLOW}Building ($BUILD_TYPE mode)...${NC}"

if [ "$BUILD_TYPE" = "debug" ]; then
    make DEBUG=1
else
    make release
fi

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

# Run if requested
if [ "$RUN" = true ]; then
    echo ""
    echo -e "${YELLOW}Running engine...${NC}"
    echo ""
    make run
fi

echo ""
echo -e "${GREEN}Done!${NC}"
