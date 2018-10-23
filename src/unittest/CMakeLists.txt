include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../parser)

add_executable(scope_test scope_test.cc)
target_link_libraries(scope_test common vm)
set_target_properties(scope_test PROPERTIES FOLDER "test")

add_executable(path_test path_test.cc)
target_link_libraries(path_test common build vm)
set_target_properties(path_test PROPERTIES FOLDER "test")

add_executable(msvc_compile_test msvc_compile_test.cc)
target_link_libraries(msvc_compile_test common vm build)
set_target_properties(msvc_compile_test PROPERTIES FOLDER "test")