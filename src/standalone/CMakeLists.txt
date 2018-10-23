add_executable(${NM_STANDALONE} 
main.cc 
file_type_register.h
file_type_register.cc 
xbgn.rc)
target_link_libraries(${NM_STANDALONE} common vm gen build)
set_target_properties(${NM_STANDALONE} PROPERTIES FOLDER "ibuild")