# Thin wrapper around CMake. Real build logic lives in CMakeLists.txt.

BUILD_DIR  ?= build
BUILD_TYPE ?= Debug
JOBS       ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc)

NEEDLE_DIR      := needle3
NEEDLE_PLATFORM := macos-arm64
NEEDLE_URL      := https://huggingface.co/Cactus-Compute/needle3/resolve/main
NEEDLE_FILES    := $(NEEDLE_DIR)/needle.h $(NEEDLE_DIR)/libneedle.a $(NEEDLE_DIR)/needle3.cact

.PHONY: all build configure release test run clean distclean needle

all: build

configure: needle
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR) -j $(JOBS)

release:
	$(MAKE) build BUILD_DIR=build-release BUILD_TYPE=Release

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure -j $(JOBS)

run: build
	./$(BUILD_DIR)/src/cactus-board

clean:
	rm -rf $(BUILD_DIR) build-release

# Also removes the downloaded Needle 3 engine and weights.
distclean: clean
	rm -f $(NEEDLE_FILES)

# Download the Needle 3 macOS engine, C header and weights (skips files already present).
needle: $(NEEDLE_FILES)

$(NEEDLE_DIR)/needle.h $(NEEDLE_DIR)/libneedle.a:
	curl -fL --progress-bar -o $@ $(NEEDLE_URL)/$(NEEDLE_PLATFORM)/$(notdir $@)

$(NEEDLE_DIR)/needle3.cact:
	curl -fL --progress-bar -o $@ $(NEEDLE_URL)/needle3.cact
