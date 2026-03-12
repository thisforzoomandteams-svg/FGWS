add_executable(Sandbox)

target_sources(Sandbox PRIVATE
  src/main.cpp
)

target_link_libraries(Sandbox PRIVATE
  Aether::Engine
)

# Copy the final binary to the repo-local /exe folder after build.
add_custom_command(TARGET Sandbox POST_BUILD
  COMMAND "${CMAKE_COMMAND}"
          -DSRC="$<TARGET_FILE:Sandbox>"
          -DDEST_DIR="${PROJECT_SOURCE_DIR}/exe"
          -DDEST_NAME="$<TARGET_FILE_NAME:Sandbox>"
          -P "${PROJECT_SOURCE_DIR}/cmake/PostBuildCopy.cmake"
)
