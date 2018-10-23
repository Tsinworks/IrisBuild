set(GEN_SRCS
vs.h vs.cc
vsc.h vsc.cc
xcode.h xcode.cc
xcode_object.h xcode_object.cc
xcode_writer.h xcode_writer.cc
ninja.h ninja.cc
xml_writer.h xml_writer.cc
gen.h gen.cc
)
source_group(src FILES ${GEN_SRCS})
add_library(gen STATIC ${GEN_SRCS})
if(WIN32)
    target_link_libraries(gen Advapi32)
endif()
set_target_properties(gen PROPERTIES FOLDER "ibuild/generator")