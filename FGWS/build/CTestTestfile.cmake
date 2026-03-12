# CMake generated Testfile for 
# Source directory: C:/Users/yomna/OneDrive/Desktop/Projects/FGWS
# Build directory: C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[FrameGenTests]=] "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/Debug/framegen_test.exe")
  set_tests_properties([=[FrameGenTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;121;add_test;C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[FrameGenTests]=] "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/Release/framegen_test.exe")
  set_tests_properties([=[FrameGenTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;121;add_test;C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[FrameGenTests]=] "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/MinSizeRel/framegen_test.exe")
  set_tests_properties([=[FrameGenTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;121;add_test;C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[FrameGenTests]=] "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/build/RelWithDebInfo/framegen_test.exe")
  set_tests_properties([=[FrameGenTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;121;add_test;C:/Users/yomna/OneDrive/Desktop/Projects/FGWS/CMakeLists.txt;0;")
else()
  add_test([=[FrameGenTests]=] NOT_AVAILABLE)
endif()
subdirs("_deps/glm-build")
subdirs("_deps/directx-headers-build")
