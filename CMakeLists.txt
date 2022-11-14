cmake_minimum_required(VERSION 3.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(penguins C CXX)

add_library(penguins-lib
  src/io.c
  src/board.c
  src/utils.c
)
target_include_directories(penguins-lib PUBLIC include)

add_executable(penguins
  src/main.c
)
target_link_libraries(penguins PUBLIC penguins-lib)

find_package(wxWidgets COMPONENTS core base)
if(wxWidgets_FOUND)
  # include(${wxWidgets_USE_FILE})
  add_executable(penguins-gui
    src/gui/main.cc
  )
  target_link_libraries(penguins-gui PUBLIC penguins-lib)

  target_include_directories(penguins-gui PUBLIC ${wxWidgets_INCLUDE_DIRS} SYSTEM)
  target_link_directories(penguins-gui PRIVATE ${wxWidgets_LIBRARY_DIRS})
  target_link_libraries(penguins-gui PRIVATE ${wxWidgets_LIBRARIES})
  target_compile_definitions(penguins-gui PRIVATE ${wxWidgets_DEFINITIONS} ${wxWidgets_DEFINITIONS_DEBUG})
  target_compile_options(penguins-gui PRIVATE ${wxWidgets_CXX_FLAGS})
endif()
