set(COMMON_SRCS 
	hash_map.cc 
	source_file.cc 
	os.cc os.hpp 
	log.hpp log.cc 
	md5.hpp md5.cc
	base64.hpp base64.cc
    zip.hpp zip.cc
    download.hpp download.cc
    pe_walker.hpp pe_walker.cc
)
if(WIN32)
list(APPEND COMMON_SRCS 
${CMAKE_SOURCE_DIR}/external/std.ext/src/win32.cpp)
endif()
list(APPEND COMMON_SRCS 
${CMAKE_SOURCE_DIR}/external/std.ext/src/socket.cpp)
source_group(src FILES ${COMMON_SRCS})

set(_ZIP_SRCS
#CDirEntry.cpp
#tools.cpp
zipper.cpp
unzipper.cpp
minizip/zip.c
minizip/unzip.c
minizip/ioapi.c
minizip/mztools.c
)
if(WIN32)
list(APPEND _ZIP_SRCS "minizip/iowin32.c")
endif()
foreach(_ZIP_SRC ${_ZIP_SRCS})
    list(APPEND ZIP_SRCS "${CMAKE_SOURCE_DIR}/external/zipper/${_ZIP_SRC}")
endforeach()

source_group("external\\zipper" FILES ${ZIP_SRCS})

add_library(common ${COMMON_SRCS} ${ZIP_SRCS})
set_target_properties(common PROPERTIES FOLDER "ibuild")

if(NOT WIN32)
    find_package(OpenSSL)
    if(OpenSSL_FOUND)
        add_definitions(-DHAS_OPENSSL=1)
        target_link_libraries(common ${OPENSSL_LIBRARIES})
    endif()
endif()

find_package(ZLib)

if(WIN32)
    target_link_libraries(common crypt32 winhttp shlwapi userenv)
endif()

target_include_directories(common PRIVATE 
${OPENSSL_INCLUDE_DIR} 
${ZLIB_INCLUDE_DIR}
${CMAKE_SOURCE_DIR}/external/zipper
${CMAKE_SOURCE_DIR}/external/zipper/minizip)


target_link_libraries(common ${ZLIB_LIBRARY})

target_compile_definitions(common PRIVATE HAVE_CONFIG_H=1)

if(WIN32)
    target_link_libraries(common "${CMAKE_SOURCE_DIR}/external/detours/lib.vc141.x64/detours.lib")
endif()
