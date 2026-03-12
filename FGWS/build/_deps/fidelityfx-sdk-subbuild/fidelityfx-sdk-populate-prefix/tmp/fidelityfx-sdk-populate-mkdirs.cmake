# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-src")
  file(MAKE_DIRECTORY "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-build"
  "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-subbuild/fidelityfx-sdk-populate-prefix"
  "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-subbuild/fidelityfx-sdk-populate-prefix/tmp"
  "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-subbuild/fidelityfx-sdk-populate-prefix/src/fidelityfx-sdk-populate-stamp"
  "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-subbuild/fidelityfx-sdk-populate-prefix/src"
  "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-subbuild/fidelityfx-sdk-populate-prefix/src/fidelityfx-sdk-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-subbuild/fidelityfx-sdk-populate-prefix/src/fidelityfx-sdk-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/_deps/fidelityfx-sdk-subbuild/fidelityfx-sdk-populate-prefix/src/fidelityfx-sdk-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
