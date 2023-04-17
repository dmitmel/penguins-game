cmake_minimum_required(VERSION 3.11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "The minimum supported macOS version")
endif()

if(POLICY CMP0092)
  # Don't add warning flags for MSVC (/W3) by default. Must be turned on prior
  # to calling project().
  # <https://cmake.org/cmake/help/latest/policy/CMP0092.html>
  cmake_policy(SET CMP0092 NEW)
endif()

project(penguins
  LANGUAGES C CXX
  VERSION 0.0.1
)

if(POLICY CMP0135)
  # Necessary for using ExternalProject and FetchContent.
  # <https://cmake.org/cmake/help/latest/policy/CMP0135.html>
  cmake_policy(SET CMP0135 NEW)
endif()

option(USE_SANITIZERS "Enable sanitizers" OFF)
option(CHECK_INCLUDES "Check #include directives with include-what-you-use" ON)

option(INTERACTIVE_MODE "Build the interactive mode" ON)
option(AUTONOMOUS_MODE "Build the autonomous mode" ON)
option(GRAPHICAL_MODE "Build the graphical interface" ON)
option(BUILD_TESTS "Build tests" ON)
option(GENERATE_DOCUMENTATION "Generate developer documentation" ON)
option(GENERATE_WXWIDGETS_DOC_TAGS "" OFF)

# These must be set before including FetchContent.
option(FETCHCONTENT_QUIET "" OFF)
option(FETCHCONTENT_UPDATES_DISCONNECTED "" ON)

option(BUILD_WXWIDGETS_FROM_SOURCE "Whether to download and build wxWidgets from source" OFF)
option(BUILD_WXWIDGETS_MINIMAL "Whether to produce a minimal or a full build of wxWidgets" ON)
option(BUILD_SHARED_LIBS "Whether to build shared or static libraries" OFF)

include(FetchContent)
FetchContent_Declare(munit
  URL "https://github.com/nemequ/munit/archive/fbbdf1467eb0d04a6ee465def2e529e4c87f2118.tar.gz"
  URL_HASH SHA1=bd1f917487c42bcee3f32c41f07bdafdc4765b13
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/cmake/munit.cmake <SOURCE_DIR>/CMakeLists.txt
)
set(deps_wx_ver "3.2.2.1")
FetchContent_Declare(wxWidgets
  URL "https://github.com/wxWidgets/wxWidgets/releases/download/v${deps_wx_ver}/wxWidgets-${deps_wx_ver}.tar.bz2"
  URL_HASH SHA1=d645cacbf2678cfc402ae163f2b0e0b05be5d239
)
FetchContent_Declare(doxygen-awesome-css
  URL "https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.1.0.tar.gz"
  URL_HASH SHA1=4986c8b81cdbdcc63d0788e2c8cdca4745ceb564
)

find_program(IWYU NAMES include-what-you-use iwyu)

function(setup_penguins_target target)
  target_include_directories(${target} PUBLIC include)
  target_compile_definitions(${target} PUBLIC PENGUINS_VERSION_STRING="${PROJECT_VERSION}")

  set_target_properties(${target} PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
  )

  if(MSVC)
    target_compile_definitions(${target} PRIVATE
      # <https://learn.microsoft.com/en-us/cpp/c-runtime-library/security-features-in-the-crt?view=msvc-170>
      _CRT_SECURE_NO_WARNINGS
      # <https://learn.microsoft.com/en-us/cpp/c-runtime-library/compatibility?view=msvc-170>
      # <https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=msvc-170#posix-function-names>
      _CRT_NONSTDC_NO_WARNINGS
    )
    target_compile_options(${target} PRIVATE
      /W4
      /we4013  # 'function' undefined; assuming extern returning int
      /we4716  # 'function' must return a value
      /wd4706  # assignment within conditional expression
      /wd4456  # declaration of 'identifier' hides previous local declaration
      /wd4457  # declaration of 'identifier' hides function parameter
      /wd4458  # declaration of 'identifier' hides class member
    )
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Werror=return-type -Wdouble-promotion -Wmissing-declarations
      $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration -Werror=strict-prototypes>
    )
  endif()

  if(USE_SANITIZERS)
    if(MSVC)
      target_compile_options(${target} PRIVATE /fsanitize=address)
    else()
      target_compile_options(${target} PRIVATE
        -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined -fno-sanitize-recover=all
      )
      target_link_options(${target} PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()
  endif()

  if(IWYU AND CHECK_INCLUDES)
    set(iwyu_command ${IWYU} -Xiwyu --mapping_file=${CMAKE_CURRENT_SOURCE_DIR}/cmake/iwyu_mapping.yml -Xiwyu --no_fwd_decls)
    set_target_properties(${target} PROPERTIES
      C_INCLUDE_WHAT_YOU_USE "${iwyu_command}"
      CXX_INCLUDE_WHAT_YOU_USE "${iwyu_command}"
    )
  endif()
endfunction()

add_executable(embed_resource cmake/embed_resource.cc)
setup_penguins_target(embed_resource)

function(embed_resource res_path out_var_name)
  string(MAKE_C_IDENTIFIER "${res_path}" var_name)
  set("${out_var_name}" "${var_name}" PARENT_SCOPE)
  get_filename_component(res_path_abs "${res_path}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  add_custom_command(
    OUTPUT "${var_name}.h"
    COMMAND embed_resource header "${var_name}.h" "${var_name}"
    DEPENDS embed_resource
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${var_name}.c"
    COMMAND embed_resource source "${res_path_abs}" "${var_name}.c" "${var_name}.h" "${var_name}"
    DEPENDS embed_resource "${res_path_abs}" "${var_name}.h"
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
setup_penguins_target(penguins-lib)

# add executables (implementations of the files defined in the header files)
add_executable(penguins
  src/arguments.c
  src/main.c
)
setup_penguins_target(penguins)
add_custom_target(run COMMAND penguins USES_TERMINAL)
if(INTERACTIVE_MODE)
  target_compile_definitions(penguins PUBLIC INTERACTIVE_MODE)
  target_sources(penguins PRIVATE
    src/interactive.c
  )
endif()
if(AUTONOMOUS_MODE)
  target_compile_definitions(penguins PUBLIC AUTONOMOUS_MODE)
  target_sources(penguins PRIVATE
    src/autonomous.c
  )
endif()
# link all the
target_link_libraries(penguins PUBLIC penguins-lib)

if(BUILD_TESTS)
  FetchContent_MakeAvailable(munit)
  add_executable(penguins-tests src/tests.c)
  setup_penguins_target(penguins-tests)
  add_custom_target(run-tests COMMAND penguins-tests USES_TERMINAL)
  # add all of the libraries defined in penguins-lib
  target_link_libraries(penguins-tests PUBLIC munit penguins-lib)
endif()

if(GRAPHICAL_MODE)
  if(NOT BUILD_WXWIDGETS_FROM_SOURCE)
    find_package(wxWidgets COMPONENTS core base)
    if(NOT wxWidgets_FOUND)
      set(GRAPHICAL_MODE OFF)
    endif()
  else()
    if(BUILD_WXWIDGETS_MINIMAL)
      include(cmake/wxwidgets_config.cmake)
    endif()
    FetchContent_MakeAvailable(wxWidgets)
    set_property(DIRECTORY ${wxWidgets_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL ON)
  endif()
endif()

if(GRAPHICAL_MODE)
  set(mac_resources)
  set(embedded_resources)

  if(APPLE)
    set(iconset_dir "appicon.iconset")
    set(icns_icon "appicon.icns")

    set(icns_icon_commands)
    set(icns_icon_depends)
    set(icns_icon_byproducts)
    foreach(icon_size IN ITEMS 16 64 256)
      set(icon_src "resources/appicon_${icon_size}.png")
      set(icon_dst "${iconset_dir}/icon_${icon_size}x${icon_size}.png")
      list(APPEND icns_icon_commands
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${icon_src}" "${icon_dst}"
      )
      list(APPEND icns_icon_depends "${icon_src}")
      list(APPEND icns_icon_byproducts "${icon_dst}")
    endforeach()

    add_custom_command(
      OUTPUT "${iconset_dir}"
      COMMAND "${CMAKE_COMMAND}" -E remove_directory "${iconset_dir}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${iconset_dir}"
      ${icns_icon_commands}
      DEPENDS ${icns_icon_depends}
      BYPRODUCTS ${icns_icon_byproducts}
      VERBATIM
    )

    add_custom_command(
      OUTPUT "${icns_icon}"
      COMMAND iconutil -c icns -o "${icns_icon}" "${iconset_dir}"
      DEPENDS "${iconset_dir}"
      VERBATIM
    )
    list(APPEND mac_resources ${icns_icon})
  endif()

  foreach(resource_file IN ITEMS appicon_16.png appicon_64.png appicon_256.png tileset.png)
    embed_resource("resources/${resource_file}" resource_name)
    list(APPEND embedded_resources "${resource_name}.c")
  endforeach()

  # Useful information about enabling High-DPI support on Windows:
  # <https://doc.magnum.graphics/magnum/platforms-windows.html#platforms-windows-hidpi>.
  # Note that when compiling on an OS other than Windows, the Windows-specific
  # options and source files (`*.rc`, `*.manifest`) will be ignored (same goes
  # for macOS-specific options).
  add_executable(penguins-gui WIN32 MACOSX_BUNDLE
    ${mac_resources}
    ${embedded_resources}
    resources/penguins.rc
    resources_appicon_16_png.c
    resources_appicon_256_png.c
    resources_appicon_64_png.c
    resources_tileset_png.c
    src/gui/better_random.cc
    src/gui/bot_thread.cc
    src/gui/canvas.cc
    src/gui/controllers.cc
    src/gui/game.cc
    src/gui/game_end_dialog.cc
    src/gui/main.cc
    src/gui/new_game_dialog.cc
    src/gui/player_info_box.cc
    src/gui/simple_static_box.cc
    src/gui/tileset.cc
  )
  setup_penguins_target(penguins-gui)
  add_custom_target(run-gui COMMAND penguins-gui USES_TERMINAL)
  target_link_libraries(penguins-gui PUBLIC penguins-lib)
  target_include_directories(penguins-gui PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  if(MSVC)
    # We include our own manifest via the `.rc` file, so the default one has to be disabled.
    target_link_options(penguins-gui PRIVATE /MANIFEST:NO)
  endif()

  set_target_properties(penguins-gui PROPERTIES
    RESOURCE "${mac_resources}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/penguins.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "Penguins"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.github.dmitmel.penguins"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} v${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "appicon"
  )

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

find_package(Doxygen)
if(Doxygen_FOUND AND GENERATE_DOCUMENTATION)
  FetchContent_MakeAvailable(doxygen-awesome-css)

  set(doxygen_inputs include src)
  # Options for better integration with C.
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
  set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
  # Put undocumented and private functions into the generated documentation.
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_PRIVATE YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  # Generate pages with the code of source files with clickable links (by
  # default only the code of headers is included).
  set(DOXYGEN_SOURCE_BROWSER YES)
  set(DOXYGEN_INTERNAL_DOCS YES)
  # Don't put either a list of includes or an includes graph at the beginning
  # of pages for individual files.
  set(DOXYGEN_SHOW_INCLUDE_FILES NO)
  set(DOXYGEN_INCLUDE_GRAPH NO)
  set(DOXYGEN_INCLUDED_BY_GRAPH NO)
  # Also don't generate graphs on the directory pages (why is this even done?).
  set(DOXYGEN_DIRECTORY_GRAPH NO)
  # And don't generate some other graphs as well.
  set(DOXYGEN_COLLABORATION_GRAPH NO)
  # Arrange members on the file and class pages in the order of declaration.
  set(DOXYGEN_SORT_MEMBER_DOCS NO)
  # Add links to other members which reference a given one.
  set(DOXYGEN_REFERENCED_BY_RELATION YES)
  set(DOXYGEN_REFERENCES_LINK_SOURCE NO)
  # Apply the doxygen-awesome theme (in the sidebar-only mode), see
  # <https://github.com/jothepro/doxygen-awesome-css#sidebar-only-theme-2> and
  # apply my own layout changes.
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_DISABLE_INDEX NO)
  set(DOXYGEN_FULL_SIDEBAR NO)
  set(DOXYGEN_HTML_EXTRA_STYLESHEET
    ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome.css
    ${doxygen-awesome-css_SOURCE_DIR}/doxygen-awesome-sidebar-only.css)
  set(DOXYGEN_HTML_COLORSTYLE LIGHT)
  set(DOXYGEN_LAYOUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/docs/DoxygenLayout.xml)
  set(DOXYGEN_DOT_IMAGE_FORMAT svg)
  set(DOXYGEN_ALPHABETICAL_INDEX NO)

  set(doxygen_target_deps)
  set(DOXYGEN_TAGFILES)
  if(GENERATE_WXWIDGETS_DOC_TAGS AND BUILD_WXWIDGETS_FROM_SOURCE)
    add_custom_command(
      OUTPUT "wxWidgets.tag"
      BYPRODUCTS "${wxWidgets_SOURCE_DIR}/docs/doxygen/out"
      WORKING_DIRECTORY "${wxWidgets_SOURCE_DIR}"
      COMMAND "${CMAKE_COMMAND}" -E env
        "WXWIDGETS=${wxWidgets_SOURCE_DIR}"
        "DOXYGEN=${DOXYGEN_EXECUTABLE}"
        "WX_SKIP_DOXYGEN_VERSION_CHECK=1"
        "GENERATE_TAGFILE=${CMAKE_CURRENT_BINARY_DIR}/wxWidgets.tag"
        "${wxWidgets_SOURCE_DIR}/docs/doxygen/regen.sh" xml
      VERBATIM
    )
    add_custom_target(generate_wxwidgets_doc_tags DEPENDS "wxWidgets.tag")
    list(APPEND DOXYGEN_TAGFILES "${CMAKE_CURRENT_BINARY_DIR}/wxWidgets.tag=https://docs.wxwidgets.org/${deps_wx_ver}/")
    list(APPEND doxygen_target_deps "generate_wxwidgets_doc_tags")
  endif()

  doxygen_add_docs(doxygen ${doxygen_inputs}
    COMMENT "Generate API documentation for ${PROJECT_NAME}"
  )
  if(doxygen_target_deps)
    add_dependencies(doxygen ${doxygen_target_deps})
  endif()
endif()

set(installed_targets penguins)
set(installed_targets_pdb $<TARGET_PDB_FILE:penguins>)
if(GRAPHICAL_MODE)
  list(APPEND installed_targets penguins-gui)
  list(APPEND installed_targets_pdb $<TARGET_PDB_FILE:penguins-gui>)
endif()
install(TARGETS ${installed_targets}
  ARCHIVE DESTINATION . COMPONENT Runtime
  LIBRARY DESTINATION . COMPONENT Runtime
  RUNTIME DESTINATION . COMPONENT Runtime
  BUNDLE  DESTINATION . COMPONENT Runtime
)
if(MSVC)
  install(FILES ${installed_targets_pdb} DESTINATION . COMPONENT Runtime OPTIONAL)
endif()

if(WIN32)
  set(CPACK_GENERATOR ZIP)
else()
  set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_SOURCE_GENERATOR TGZ)

# <https://github.com/Kitware/CMake/blob/v3.25.3/Modules/CPack.cmake#L646-L654>
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CPACK_SYSTEM_NAME win64)
  else()
    set(CPACK_SYSTEM_NAME win32)
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CPACK_SYSTEM_NAME macos)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CPACK_SYSTEM_NAME linux)
else()
  set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
endif()

include(CPack)
