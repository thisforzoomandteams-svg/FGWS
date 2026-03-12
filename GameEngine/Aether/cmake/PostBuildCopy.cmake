# Post-build copy helper.
#
# Usage:
#   cmake -DSRC="..." -DDEST_DIR="..." -DDEST_NAME="..." -P PostBuildCopy.cmake
#
# This script intentionally never fails the build if the copy can't be performed
# (e.g., destination exe is currently running on Windows). It prints a warning
# and attempts a fallback copy to a secondary filename.

if(NOT DEFINED SRC OR SRC STREQUAL "")
  message(FATAL_ERROR "PostBuildCopy.cmake: SRC not set")
endif()
if(NOT DEFINED DEST_DIR OR DEST_DIR STREQUAL "")
  message(FATAL_ERROR "PostBuildCopy.cmake: DEST_DIR not set")
endif()
if(NOT DEFINED DEST_NAME OR DEST_NAME STREQUAL "")
  message(FATAL_ERROR "PostBuildCopy.cmake: DEST_NAME not set")
endif()

file(MAKE_DIRECTORY "${DEST_DIR}")

set(dest "${DEST_DIR}/${DEST_NAME}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SRC}" "${dest}"
  RESULT_VARIABLE copy_rv
)

if(copy_rv EQUAL 0)
  message(STATUS "Copied Sandbox to ${dest}")
else()
  message(WARNING "Post-build copy failed (file may be in use): ${dest}")

  # Fallback: copy to a secondary name so a new build is still accessible.
  get_filename_component(dest_we "${dest}" NAME_WE)
  get_filename_component(dest_ext "${dest}" EXT)
  set(fallback "${DEST_DIR}/${dest_we}.new${dest_ext}")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SRC}" "${fallback}"
    RESULT_VARIABLE fallback_rv
  )

  if(fallback_rv EQUAL 0)
    message(STATUS "Copied Sandbox fallback to ${fallback}")
  else()
    message(WARNING "Fallback copy also failed: ${fallback}")
  endif()
endif()
