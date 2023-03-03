# <https://github.com/Bill-Gray/PDCursesMod/blob/master/cmake/build_options.cmake>

option(PDC_BUILD_SHARED "" OFF)
option(PDC_WIDE "" ON)

function(penguins_select_pdcurses_target)
  set(_target hi)

  if(UNIX)
    option(PDC_VT_BUILD "" ON)
    set(_target vt)
  elseif(WIN32)
    option(PDC_WINCON_BUILD "" ON)
    set(_target wincon)
  else()
    message(FATAL_ERROR "Couldn't select an appropriate PDCurses implementation for your platform!")
  endif()
  message(STATUS "Selected PDCurses implementation: ${_target}")

  set(_target "${_target}_pdcurses")
  if(NOT PDC_BUILD_SHARED)
    set(_target "${_target}static")
  endif()
  set(PENGUINS_SELECTED_PDCURSES_TARGET "${_target}" PARENT_SCOPE)
endfunction()
penguins_select_pdcurses_target()

# Disable all implementations we don't use.
option(PDC_OS2_BUILD     "" OFF)
option(PDC_DOS_BUILD     "" OFF)
option(PDC_DOSVGA_BUILD  "" OFF)
option(PDC_DOSVT_BUILD   "" OFF)
option(PDC_SDL2_BUILD    "" OFF)
option(PDC_NCURSES_BUILD "" OFF)
option(PDC_DEMOS_BUILD   "" OFF)
option(PDC_WINCON_BUILD  "" OFF)
option(PDC_WINGUI_BUILD  "" OFF)
option(PDC_VT_BUILD      "" OFF)

option(PDC_SDL2_DEPS_BUILD "" OFF)
option(PDC_INSTALL_TARGETS "" OFF)
