cmake_minimum_required(VERSION 3.11)
project(penguins C CXX)

if (POLICY CMP0135)
  # Necessary for using ExternalProject and FetchContent.
  # <https://cmake.org/cmake/help/latest/policy/CMP0135.html>
  cmake_policy(SET CMP0135 NEW)
endif()

option(USE_SANITIZERS "Enable sanitizers" OFF)
option(INTERACTIVE_MODE "Build the interactive mode" ON)
option(AUTONOMOUS_MODE "Build the autonomous mode" ON)
option(GRAPHICAL_MODE "Build the graphical interface" ON)
option(BUILD_TESTS "Build tests" ON)

# These must be set before including FetchContent.
option(FETCHCONTENT_QUIET "" OFF)
option(FETCHCONTENT_UPDATES_DISCONNECTED "" ON)

option(BUILD_WXWIDGETS_FROM_SOURCE "Whether to download and build wxWidgets from source" OFF)
option(BUILD_SHARED_LIBS "Whether to build shared or static libraries" OFF)

include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(penguins-common INTERFACE)
target_include_directories(penguins-common INTERFACE include)
if(MSVC)
  target_compile_options(penguins-common INTERFACE /W4)
else()
  target_compile_options(penguins-common INTERFACE
    -Wall -Wextra -Wpedantic -Werror=return-type
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>
  )
endif()

if(USE_SANITIZERS AND NOT MSVC)
  target_compile_options(penguins-common INTERFACE -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_options(penguins-common INTERFACE -fsanitize=address -fsanitize=undefined)
endif()

add_executable(embed_resource src/tools/embed_resource.cc)
target_link_libraries(embed_resource PRIVATE penguins-common)

function(embed_resource res_path)
  set(var_name ${res_path})
  # <https://github.com/Kitware/CMake/blob/v3.25.1/Source/kwsys/SystemTools.cxx#L4304-L4321>
  if(var_name MATCHES "^[0-9]")
    set(var_name "_${var_name}")
  endif()
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" var_name ${var_name})
  get_filename_component(res_path_abs ${res_path} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  add_custom_command(
    OUTPUT ${var_name}.c ${var_name}.h
    COMMAND embed_resource ${res_path_abs} ${var_name}.c ${var_name}.h ${var_name}
    DEPENDS embed_resource ${res_path_abs}
    VERBATIM
  )
endfunction()

add_library(penguins-lib STATIC
  src/board.c
  src/bot.c
  src/game.c
  src/movement.c
  src/placement.c
  src/utils.c
)
target_link_libraries(penguins-lib PRIVATE penguins-common)

# add executables (implementations of the files defined in the header files)
add_executable(penguins
  src/arguments.c
  src/main.c
  src/random.c
)
if(INTERACTIVE_MODE)
  target_compile_definitions(penguins PUBLIC INTERACTIVE_MODE)
  target_sources(penguins PRIVATE
    src/color.c
    src/interactive.c
    src/io.c
  )
endif()
if(AUTONOMOUS_MODE)
  target_compile_definitions(penguins PUBLIC AUTONOMOUS_MODE)
  target_sources(penguins PRIVATE
    src/autonomous.c
  )
endif()
# link all the
target_link_libraries(penguins PRIVATE penguins-common PUBLIC penguins-lib)

FetchContent_Declare(munit
  URL "https://github.com/nemequ/munit/tarball/fbbdf1467eb0d04a6ee465def2e529e4c87f2118"
  URL_HASH SHA1=53028ad0abb0667dd1f8d75940326b73ed759a71
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/munit.cmake <SOURCE_DIR>/CMakeLists.txt
)
FetchContent_MakeAvailable(munit)

if(BUILD_TESTS)
  add_executable(penguins-tests src/tests.c)
  # add all of the libraries defined in penguins-lib
  target_link_libraries(penguins-tests PRIVATE penguins-common PUBLIC munit penguins-lib)
endif()

if(GRAPHICAL_MODE)
  if(NOT BUILD_WXWIDGETS_FROM_SOURCE)
    find_package(wxWidgets COMPONENTS core base)
    if(NOT wxWidgets_FOUND)
      set(GRAPHICAL_MODE OFF)
    endif()
  else()
    FetchContent_Declare(wxWidgets
      URL "https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.1/wxWidgets-3.2.1.tar.bz2"
      URL_HASH SHA1=b5299275abddc7cb5fa92f75c17475ade3bc0532
    )
    FetchContent_MakeAvailable(wxWidgets)
    set_property(DIRECTORY ${wxWidgets_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL ON)
  endif()
endif()

if(GRAPHICAL_MODE)
  embed_resource(resources/appicon_256.png)
  embed_resource(resources/tileset.png)
  # Useful information about enabling High-DPI support on Windows:
  # <https://doc.magnum.graphics/magnum/platforms-windows.html#platforms-windows-hidpi>.
  # Note that when compiling on an OS other than Windows, the Windows-specific options and source
  # files (`*.rc`, `*.manifest`) will be ignored.
  add_executable(penguins-gui WIN32
    resources/penguins.rc
    resources_appicon_256_png.c
    resources_tileset_png.c
    src/gui/better_random.cc
    src/gui/game.cc
    src/gui/game_end_dialog.cc
    src/gui/main.cc
    src/gui/new_game_dialog.cc
    src/gui/player_info_box.cc
    src/gui/simple_static_box.cc
    src/gui/tileset.cc
  )
  target_link_libraries(penguins-gui PRIVATE penguins-common PUBLIC penguins-lib)
  target_include_directories(penguins-gui PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  if(MSVC)
    # We include our own manifest via the `.rc` file, so the default one has to be disabled.
    target_link_options(penguins-gui PRIVATE /MANIFEST:NO)
  endif()

  if(BUILD_WXWIDGETS_FROM_SOURCE)
    target_link_libraries(penguins-gui PUBLIC wx::core wx::base)
  else()
    target_include_directories(penguins-gui SYSTEM PUBLIC ${wxWidgets_INCLUDE_DIRS})
    target_link_directories(penguins-gui PRIVATE ${wxWidgets_LIBRARY_DIRS})
    target_link_libraries(penguins-gui PRIVATE ${wxWidgets_LIBRARIES})
    target_compile_definitions(penguins-gui PRIVATE ${wxWidgets_DEFINITIONS})
    target_compile_definitions(penguins-gui PRIVATE $<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)
    target_compile_options(penguins-gui PRIVATE ${wxWidgets_CXX_FLAGS})
  endif()
endif()
