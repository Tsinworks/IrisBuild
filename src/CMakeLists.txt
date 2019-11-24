set(COMMON_SRCS
common/os.hpp
common/os.cc
common/log.cc
common/args_parse.cc
common/win32.hpp
common/win32.cpp)
source_group(Common FILES ${COMMON_SRCS})

if(BUILD_IGEN)
    add_subdirectory(igen)
endif()

set(STANDALONE_SRCS
console_utils.h
console_utils.cc
file_type_register.h
file_type_register.cc
main_ui.h
main_ui.cc
main.cc
ibuild_WinRes.rc)

source_group(Std FILES ${STANDALONE_SRCS})

add_executable(ib ${STANDALONE_SRCS} ${COMMON_SRCS})
target_link_libraries(ib ext::libui ext::ibvm)

if(WIN32)
    add_custom_command(TARGET ib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${libui_DLL}
            $<TARGET_FILE_DIR:ib>
	    COMMENT "Copying libui.dll...")

    add_custom_command(TARGET ib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${ibvm_DLL}
            $<TARGET_FILE_DIR:ib>
	    COMMENT "Copying ibvm.dll...")
        
    add_custom_command(TARGET ib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${cldump_DLL}
            $<TARGET_FILE_DIR:ib>
	    COMMENT "Copying ibvm.dll...")
endif()