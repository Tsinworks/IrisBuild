
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(iGenBison ${CMAKE_BINARY_DIR}/src/parser.cpp)
set(iGenFlex ${CMAKE_BINARY_DIR}/src/scanner.cpp)
list(APPEND MAIN_SRCS ${CMAKE_BINARY_DIR}/src/tokens.h ${iGenFlex} ${iGenBison})
set_source_files_properties(${iGenFlex} ${iGenBison} ${CMAKE_BINARY_DIR}/src/tokens.h PROPERTIES GENERATED TRUE)

list(APPEND MAIN_SRCS main.cpp)
list(APPEND MAIN_SRCS parse/types.h parse/types.cpp)
list(APPEND MAIN_SRCS parse/context.h parse/context.cpp)

source_group(igen FILES ${MAIN_SRCS})
add_executable(igen ${MAIN_SRCS} parse/inter.ypp parse/inter.lpp)
target_link_libraries(igen common)
if(BISON_FOUND)
    add_custom_command(TARGET igen PRE_BUILD
      COMMAND ${BISON_EXECUTABLE} --defines=${CMAKE_BINARY_DIR}/src/tokens.h --output=${iGenBison} ${CMAKE_CURRENT_SOURCE_DIR}/parse/inter.ypp
      COMMENT "Generating parser.cpp")
endif()
if(FLEX_FOUND)
    add_custom_command(TARGET igen PRE_BUILD
      COMMAND ${FLEX_EXECUTABLE} --outfile=${iGenFlex} ${CMAKE_CURRENT_SOURCE_DIR}/parse/inter.lpp
      COMMENT "Generating scanner.cpp")
endif()

target_include_directories(igen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})