find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
set(NODE_SRCS
node.cc
node.h
function.h
function.cc
proj.h
proj.cc
cproj.h
cproj.cc
csproj.h
csproj.cc
import.h
import.cc
solution.h
solution.cc)
source_group(ast FILES ${NODE_SRCS})
set(VM_SRCS
error_msg.cc
error_msg.h
file.cc
file.h
scope.cc
scope.h
token.cc
token.h
tokens.h
value.cc
value.h
xb.lpp
xb.ypp
vm.h
vm.cc
intrin_fns.h
intrin_fns.cc
)
set(BisonOutput ${CMAKE_SOURCE_DIR}/src/vm/xbparser.cpp)
SET(FlexOutput ${CMAKE_SOURCE_DIR}/src/vm/xbscanner.cpp)
list(APPEND VM_SRCS tokens.h ${FlexOutput} ${BisonOutput})
set_source_files_properties(${FlexOutput} ${BisonOutput} tokens.h PROPERTIES GENERATED TRUE)
source_group(vm FILES ${VM_SRCS})

add_library(vm STATIC ${VM_SRCS} ${NODE_SRCS})

target_link_libraries(vm common gen)
if(BISON_FOUND)
    add_custom_command(TARGET vm PRE_BUILD
      COMMAND ${BISON_EXECUTABLE}
              --defines=${CMAKE_SOURCE_DIR}/src/vm/tokens.h
              --output=${BisonOutput}
              ${CMAKE_CURRENT_SOURCE_DIR}/xb.ypp
      COMMENT "Generating parser.cpp"
    )
endif()
if(FLEX_FOUND)
    add_custom_command(TARGET vm PRE_BUILD
      COMMAND ${FLEX_EXECUTABLE}
              --outfile=${FlexOutput}
              ${CMAKE_CURRENT_SOURCE_DIR}/xb.lpp
      COMMENT "Generating scanner.cpp"
    )
endif()
if(WIN32)
    target_compile_definitions(vm PRIVATE YY_NO_UNISTD_H=1)
endif()
set_target_properties(vm PROPERTIES FOLDER "ibuild")