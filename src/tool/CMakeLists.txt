set(BLD_SRCS
build.h
build.cc
build_cproj.cc
toolchain.h
toolchain.cc
msvc.h
msvc.cc
clang.h
clang.cc
detector.h
detector.cc)
source_group(build FILES ${BLD_SRCS})
add_library(build STATIC ${BLD_SRCS})
set_target_properties(build PROPERTIES FOLDER "ibuild/build")

if(WIN32)
    set(CL_TRACKER_SRCS
    cl_inc_tracker.cc)
    add_library(cl_tracker SHARED ${CL_TRACKER_SRCS})
    target_link_libraries(cl_tracker PRIVATE "${CMAKE_SOURCE_DIR}/external/detours/lib.vc141.x64/detours.lib")
    set_target_properties(cl_tracker PROPERTIES FOLDER "ibuild/build")
endif()
