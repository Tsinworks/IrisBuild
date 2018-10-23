set(VIRTUAL_PROC_SRCS
virtual_process.h
virtual_process.cc
proto.h
proto.cc
entries.h
entries.cc
dllmain.cc
)
source_group("virtual_proc" FILES ${VIRTUAL_PROC_SRCS})
add_library(virtual_proc SHARED ${VIRTUAL_PROC_SRCS})
set_target_properties(virtual_proc PROPERTIES FOLDER "ibuild/build")
target_link_libraries(virtual_proc PRIVATE "${CMAKE_SOURCE_DIR}/external/detours/lib.vc141.x64/detours.lib")