cmake_minimum_required (VERSION 3.6)
project (IrisBuild)

set(NM_STANDALONE "ibuild")

set_property(GLOBAL PROPERTY USE_FOLDERS on)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${xbgn_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_CXX_STANDARD 14)
include_directories(
	include external/std.ext/include 
	external/rapidjson 
	external/tinyxml2
	external/detours/include
)
add_subdirectory(src)
#add_subdirectory(tests)
