# Snow-Pi Dashboard Makefile
# Author: /x64/dumped

CC = gcc
SDL_DIR = SDL-release-3.2.26/SDL-release-3.2.26
BUILD_DIR = SDL-release-3.2.26/build
SDL_TTF_DIR = SDL_ttf
CFLAGS = -Wall -Wextra -O2 -std=c11 -I$(SDL_DIR)/include -I$(SDL_TTF_DIR)/include
SRC = main.c map_viewer.c

# Detect OS
ifeq ($(OS),Windows_NT)
    # Windows build
    LIBS = -L$(BUILD_DIR)/Release -lSDL3 -lSDL3_ttf -lsqlite3 -lm
    TARGET_EXT = .exe
    RM = del /Q
    MKDIR = mkdir
    PATHSEP = \\
else
    # Linux/Unix build
    LIBS = -L$(BUILD_DIR) -Wl,-rpath,$(BUILD_DIR) -lSDL3 -lSDL3_ttf -lsqlite3 -lm -lpthread -ldl
    TARGET_EXT =
    RM = rm -f
    MKDIR = mkdir -p
    PATHSEP = /
    
    # Detect architecture for ARM optimization
    ARCH := $(shell uname -m)
    ifeq ($(ARCH),aarch64)
        CFLAGS += -march=armv8-a -mtune=cortex-a53
    else ifeq ($(ARCH),armv7l)
        CFLAGS += -march=armv7-a -mfpu=neon-vfpv4 -mtune=cortex-a7
    endif
endif

TARGET = snow-pi-dash$(TARGET_EXT)

all: sdl3 $(TARGET)

sdl3:
	@if [ ! -f "$(BUILD_DIR)/libSDL3.so" ] && [ ! -f "$(BUILD_DIR)/libSDL3.a" ]; then \
		echo "Building SDL3 from source..."; \
		mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && \
		cmake ../SDL-release-3.2.26 \
			-DCMAKE_BUILD_TYPE=Release \
			-DSDL_STATIC=OFF \
			-DSDL_SHARED=ON \
			-DSDL_TEST_LIBRARY=OFF && \
		cmake --build . --parallel; \
	else \
		echo "SDL3 already built."; \
	fi

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)
	@echo "Build complete! Run with: ./$(TARGET)"

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

clean:
ifeq ($(OS),Windows_NT)
	if exist $(TARGET) del /Q $(TARGET)
else
	rm -f $(TARGET)
endif

clean-all: clean
ifeq ($(OS),Windows_NT)
	if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
else
	rm -rf $(BUILD_DIR)
endif

install:
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/$(TARGET)"

install-deps-debian:
	sudo apt update
	sudo apt install -y build-essential cmake git pkg-config \
		libx11-dev libxext-dev libwayland-dev libxkbcommon-dev \
		libegl1-mesa-dev libgles2-mesa-dev libdbus-1-dev libibus-1.0-dev \
		libudev-dev libfreetype6-dev libharfbuzz-dev fonts-dejavu-core \
		libsqlite3-dev

install-deps-arch:
	sudo pacman -S --needed base-devel cmake wayland libxkbcommon mesa libx11 \
		libxext dbus ibus freetype2 harfbuzz ttf-dejavu

run: $(TARGET)
	./$(TARGET)

.PHONY: all debug clean install install-deps-debian install-deps-arch run

