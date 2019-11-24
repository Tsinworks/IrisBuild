get_filename_component(install_prefix "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

add_library(ext::libui SHARED IMPORTED)

if(WIN32)
    set_property(TARGET ext::libui APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
    set_property(TARGET ext::libui APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(ext::libui PROPERTIES 
        IMPORTED_LOCATION_DEBUG "${install_prefix}/bin/libui.dll"
        IMPORTED_IMPLIB_DEBUG "${install_prefix}/lib/libui.lib"
        IMPORTED_LOCATION_RELEASE "${install_prefix}/bin/libui.dll"
        IMPORTED_IMPLIB_RELEASE "${install_prefix}/lib/libui.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include"
    )
    set(libui_DLL "${install_prefix}/bin/libui.dll")
elseif(APPLE)
    set_target_properties(ext::libui PROPERTIES
        IMPORTED_LOCATION "${install_prefix}/bin/libui.A.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${install_prefix}/include")
endif()
