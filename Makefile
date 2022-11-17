# A wrapper around CMake, based on <https://github.com/neovim/neovim/blob/v0.8.1/Makefile>

-include local.mk

CMAKE ?= cmake
CMAKE_BUILD_TYPE ?= Debug
CMAKE_FLAGS := -D CMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -G 'Unix Makefiles'
CMAKE_EXTRA_FLAGS ?=
BUILD_DIR ?= build

all: $(BUILD_DIR)/.ran-cmake
	$(MAKE) -C $(BUILD_DIR)

build: $(BUILD_DIR)/.ran-cmake
	$(MAKE) -C $(BUILD_DIR) penguins

build-gui: $(BUILD_DIR)/.ran-cmake
	$(MAKE) -C $(BUILD_DIR) penguins-gui

run: build
	$(BUILD_DIR)/penguins

run-gui: build-gui
	$(BUILD_DIR)/penguins-gui

clean:
	$(MAKE) -C $(BUILD_DIR) clean

distclean:
	rm -rf $(BUILD_DIR)

cmake:
	rm -f $(BUILD_DIR)/.ran-cmake
	$(MAKE) $(BUILD_DIR)/.ran-cmake

$(BUILD_DIR)/.ran-cmake:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) $(CMAKE_EXTRA_FLAGS) $$OLDPWD
	touch $@

.PHONY: all build build-gui run run-gui clean distclean cmake
