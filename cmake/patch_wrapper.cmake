# The `patch` command exits with the code 1 when it detects that a patch has
# already been applied, so this wrapper script is necessary to treat that
# situation as a successful invocation of `patch`.

math(EXPR cmake_argc_max "${CMAKE_ARGC} - 1")
set(command_start_idx 0)
foreach(idx RANGE 1 "${cmake_argc_max}")
  if("${CMAKE_ARGV${idx}}" STREQUAL "-P")
    # The next argument is the path to our script, skip it as well.
    math(EXPR command_start_idx "${idx} + 2")
    # Anyway, the start of the script arguments has been found.
    break()
  else()
    math(EXPR command_start_idx "${idx} + 1")
  endif()
endforeach()

set(command)
foreach(idx RANGE "${command_start_idx}" "${cmake_argc_max}")
  list(APPEND command "${CMAKE_ARGV${idx}}")
endforeach()

execute_process(COMMAND ${command} RESULT_VARIABLE exit_code)
if(NOT (exit_code EQUAL 0 OR exit_code EQUAL 1))
  message(FATAL_ERROR "patch command failed with exit code: ${exit_code}")
endif()
