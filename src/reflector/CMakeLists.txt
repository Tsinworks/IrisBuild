set(REFL_SRCS
reflector.h
reflector.cc
builder.cc)
source_group(ref FILES ${REFL_SRCS})
add_library(refl ${REFL_SRCS})
target_include_directories(refl PRIVATE "C:/Program Files/LLVM/include")
link_directories("C:/Program Files/LLVM/lib")
target_link_libraries(refl libclang)
set_target_properties(refl PROPERTIES FOLDER "rpc")

add_executable(rpcc main.cc)
target_link_libraries(rpcc refl common)
set_target_properties(rpcc PROPERTIES FOLDER "rpc")
