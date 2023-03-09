# A wrapper around CMake, based on <https://github.com/neovim/neovim/blob/v0.8.1/Makefile>

-include local.mk

CMAKE ?= $(shell command -v cmake 2>/dev/null)
NINJA ?= $(shell command -v ninja 2>/dev/null)

ifneq (,$(NINJA))
  CMAKE_GENERATOR ?= Ninja
else
  CMAKE_GENERATOR ?= Unix Makefiles
endif

# Based on <https://github.com/neovim/neovim/blob/v0.8.3/Makefile#L57-L66>
ifneq (,$(findstring Ninja,$(CMAKE_GENERATOR)))
  BUILD_TOOL = $(NINJA)
  ifneq ($(VERBOSE),)
    BUILD_TOOL += -v
  endif
  ifeq (n,$(findstring n,$(firstword -$(MAKEFLAGS))))
    BUILD_TOOL += -n
  endif
  ifeq (k,$(findstring k,$(firstword -$(MAKEFLAGS))))
    BUILD_TOOL += -k0
  endif
  closeparen := )
  BUILD_TOOL += $(shell \
    for arg in $(MAKEFLAGS); do case "$$arg" in \
      -[jl][0-9]*[!0-9]*$(closeparen) ;; \
      -[jl][0-9]*$(closeparen) printf '%s\n' "$$arg";; \
      --$(closeparen) break;; \
    esac; done)
else
  BUILD_TOOL = $(MAKE)
endif

CMAKE_BUILD_TYPE ?= Debug
USE_SANITIZERS ?= OFF
CMAKE_FLAGS := -G'$(CMAKE_GENERATOR)' -DCMAKE_BUILD_TYPE='$(CMAKE_BUILD_TYPE)' -DUSE_SANITIZERS='$(USE_SANITIZERS)'
CMAKE_EXTRA_FLAGS ?=
BUILD_DIR ?= build
CMAKE_STAMP := $(BUILD_DIR)/.ran-cmake

all: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR)

build: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR) penguins

build-gui: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR) penguins-gui

build-tests: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR) penguins-tests

run: build
	$(BUILD_DIR)/penguins

run-gui: build-gui
	$(BUILD_DIR)/penguins-gui

test: build-tests
	$(BUILD_DIR)/penguins-tests

docs: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR) doxygen

clean:
	+$(BUILD_TOOL) -C $(BUILD_DIR) clean

distclean:
	rm -rf $(BUILD_DIR)

cmake:
	rm -f $(CMAKE_STAMP)
	$(MAKE) $(CMAKE_STAMP)

$(CMAKE_STAMP):
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && $(CMAKE) $(CMAKE_FLAGS) $(CMAKE_EXTRA_FLAGS) $$OLDPWD
	touch $@

.PHONY: all build build-gui run run-gui test docs clean distclean cmake
