get_filename_component(install_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

add_library(ext::ibvm SHARED IMPORTED)

if(WIN32)
    set_target_properties(ext::ibvm PROPERTIES 
        IMPORTED_LOCATION "${install_prefix}/bin/ibvm.dll"
        IMPORTED_IMPLIB "${install_prefix}/lib/ibvm.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
    )
    set(ibvm_DLL "${install_prefix}/bin/ibvm.dll")
    set(cldump_DLL "${install_prefix}/bin/cl_inc_dump.dll")
endif()
