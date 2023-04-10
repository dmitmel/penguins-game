# A wrapper around CMake, based on <https://github.com/neovim/neovim/blob/v0.8.1/Makefile>

CMAKE_FLAGS = -G'$(CMAKE_GENERATOR)'
define cmake_var
$(1) ?= $(2)
CMAKE_FLAGS += $$(if $$($(1)),-D$(1)='$$($(1))',)
endef

-include local.mk

CMAKE ?= cmake
NINJA ?= ninja

CMAKE_GENERATOR ?= Unix Makefiles

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

$(eval $(call cmake_var,CMAKE_BUILD_TYPE,Debug))
$(eval $(call cmake_var,USE_SANITIZERS,))
$(eval $(call cmake_var,BUILD_WXWIDGETS_FROM_SOURCE,))
$(eval $(call cmake_var,BUILD_SHARED_LIBS,))

CMAKE_EXTRA_FLAGS ?=
BUILD_DIR ?= build
CMAKE_STAMP := $(BUILD_DIR)/.ran-cmake

all: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR)

build: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR) penguins

build-lib: $(CMAKE_STAMP)
	+$(BUILD_TOOL) -C $(BUILD_DIR) penguins-lib

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
