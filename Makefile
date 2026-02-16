# Advanced Raycasting Engine Makefile
# Supports MinGW64, GCC, and Clang

CC = gcc
CXX = g++
TARGET = raycasting_engine

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
CPP_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(C_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
          $(CPP_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Compiler flags
CFLAGS = -I$(INC_DIR) -Wall -Wextra -O3 -march=native -ffast-math -flto
CXXFLAGS = $(CFLAGS) -std=c++17
LDFLAGS = -lm -lSDL2 -lSDL2main

# Platform detection
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    LDFLAGS += -mwindows -mconsole
    RM = del /Q
    MKDIR = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
    RMDIR = if exist $(subst /,\,$(1)) rmdir /S /Q $(subst /,\,$(1))
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        LDFLAGS += -lGL -lGLU
    endif
    ifeq ($(UNAME_S),Darwin)
        LDFLAGS += -framework OpenGL -framework Cocoa
    endif
    RM = rm -f
    MKDIR = mkdir -p $(1)
    RMDIR = rm -rf $(1)
endif

# Build modes
DEBUG ?= 0
ifeq ($(DEBUG),1)
    CFLAGS += -g -O0 -DDEBUG
    CXXFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O3 -DNDEBUG
    CXXFLAGS += -O3 -DNDEBUG
endif

# Profiling
PROFILE ?= 0
ifeq ($(PROFILE),1)
    CFLAGS += -pg
    CXXFLAGS += -pg
    LDFLAGS += -pg
endif

# Sanitizers (for debugging)
SANITIZE ?= 0
ifeq ($(SANITIZE),1)
    CFLAGS += -fsanitize=address,undefined
    CXXFLAGS += -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
endif

# Main targets
.PHONY: all clean run release debug

all: $(BIN_DIR)/$(TARGET)

release: CFLAGS += -O3 -DNDEBUG
release: CXXFLAGS += -O3 -DNDEBUG
release: clean all

debug: CFLAGS += -g -O0 -DDEBUG
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: clean all

# Create directories
$(BUILD_DIR):
	$(call MKDIR,$@)

$(BIN_DIR):
	$(call MKDIR,$@)

# Link executable
$(BIN_DIR)/$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "Linking $@..."
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	$(call RMDIR,$(BUILD_DIR))
	$(call RMDIR,$(BIN_DIR))
	@echo "Clean complete"

# Run the application
run: all
	@echo "Running $(TARGET)..."
	@cd $(BIN_DIR) && ./$(TARGET)

# Install dependencies (Linux/MacOS)
.PHONY: deps
deps:
	@echo "Installing dependencies..."
ifeq ($(UNAME_S),Linux)
	sudo apt-get update
	sudo apt-get install -y libsdl2-dev build-essential
else ifeq ($(UNAME_S),Darwin)
	brew install sdl2
endif
	@echo "Dependencies installed"

# Help target
.PHONY: help
help:
	@echo "Advanced Raycasting Engine - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the engine (default)"
	@echo "  release  - Build optimized release version"
	@echo "  debug    - Build debug version with symbols"
	@echo "  clean    - Remove all build artifacts"
	@echo "  run      - Build and run the engine"
	@echo "  deps     - Install required dependencies (Linux/MacOS)"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1     - Enable debug mode"
	@echo "  PROFILE=1   - Enable profiling"
	@echo "  SANITIZE=1  - Enable address and undefined behavior sanitizers"
	@echo ""
	@echo "Examples:"
	@echo "  make                  # Build release version"
	@echo "  make DEBUG=1          # Build debug version"
	@echo "  make clean all        # Clean rebuild"
	@echo "  make run              # Build and run"
	@echo "  make SANITIZE=1 debug # Debug with sanitizers"

# Dependency tracking
-include $(OBJECTS:.o=.d)

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(BUILD_DIR)/$*.o $< -MF $@

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -MM -MT $(BUILD_DIR)/$*.o $< -MF $@
