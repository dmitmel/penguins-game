cmake_minimum_required(VERSION 3.4)
project(penguins C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(penguins-common INTERFACE)
if(MSVC)
  target_compile_options(penguins-common INTERFACE /W4)
else()
  target_compile_options(penguins-common INTERFACE
    -Wall -Wextra -Wpedantic -Werror=return-type
    $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration>
  )
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

add_library(penguins-lib
  src/board.c
  src/game.c
  src/movement.c
  src/placement.c
)
target_include_directories(penguins-lib PUBLIC include)
target_link_libraries(penguins-lib PRIVATE penguins-common)

# add executables (implementations of the files defined in the header files)
add_executable(penguins
  src/arguments.c
  src/interactive.c
  src/io.c
  src/main.c
  src/random.c
)
# link all the
target_link_libraries(penguins PRIVATE penguins-common)
target_link_libraries(penguins PUBLIC penguins-lib)

if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/munit)
  add_library(munit munit/munit.c)
  target_include_directories(munit PUBLIC munit)

  add_executable(penguins-tests src/tests.c)
  # add all of the libraries defined in penguins-lib
  target_link_libraries(penguins-tests PUBLIC munit penguins-lib)
endif()

find_package(wxWidgets COMPONENTS core base)
if(wxWidgets_FOUND)
  # include(${wxWidgets_USE_FILE})
  embed_resource(resources/tileset.png)
  add_executable(penguins-gui WIN32
    src/gui/better_random.cc
    src/gui/game.cc
    src/gui/main.cc
    src/gui/new_game_dialog.cc
    src/gui/player_info_box.cc
    src/gui/simple_static_box.cc
    src/gui/tileset.cc
    resources_tileset_png.c
  )
  target_link_libraries(penguins-gui PRIVATE penguins-common)
  target_link_libraries(penguins-gui PUBLIC penguins-lib)
  target_include_directories(penguins-gui PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

  target_include_directories(penguins-gui SYSTEM PUBLIC ${wxWidgets_INCLUDE_DIRS})
  target_link_directories(penguins-gui PRIVATE ${wxWidgets_LIBRARY_DIRS})
  target_link_libraries(penguins-gui PRIVATE ${wxWidgets_LIBRARIES})
  target_compile_definitions(penguins-gui PRIVATE ${wxWidgets_DEFINITIONS} ${wxWidgets_DEFINITIONS_DEBUG})
  target_compile_options(penguins-gui PRIVATE ${wxWidgets_CXX_FLAGS})
endif()
