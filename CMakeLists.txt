cmake_minimum_required(VERSION 3.1)
project(penguins C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(penguins-lib
  src/io.c
  src/board.c
)
target_include_directories(penguins-lib PUBLIC include)

add_library(munit munit/munit.c)
target_include_directories(munit PUBLIC munit)

# add executables (implementations of the files defined in the header files)
add_executable(penguins 
  src/main.c
  src/random.c
)
# link all the 
target_link_libraries(penguins PUBLIC penguins-lib)

add_executable(penguins-tests src/tests.c)
# add all of the libraries defined in penguins-lib 
target_link_libraries(penguins-tests PUBLIC munit penguins-lib)

find_package(wxWidgets COMPONENTS core base)
if(wxWidgets_FOUND)
  # include(${wxWidgets_USE_FILE})
  add_executable(penguins-gui
    src/gui/main.cc
    src/gui/better_random.cc
  )
  target_link_libraries(penguins-gui PUBLIC penguins-lib)

  target_include_directories(penguins-gui SYSTEM PUBLIC ${wxWidgets_INCLUDE_DIRS})
  target_link_directories(penguins-gui PRIVATE ${wxWidgets_LIBRARY_DIRS})
  target_link_libraries(penguins-gui PRIVATE ${wxWidgets_LIBRARIES})
  target_compile_definitions(penguins-gui PRIVATE ${wxWidgets_DEFINITIONS} ${wxWidgets_DEFINITIONS_DEBUG})
  target_compile_options(penguins-gui PRIVATE ${wxWidgets_CXX_FLAGS})
endif()
