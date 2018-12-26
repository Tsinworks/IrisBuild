
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

set(iGenBison ${CMAKE_BINARY_DIR}/src/parser.cpp)
set(iGenFlex ${CMAKE_BINARY_DIR}/src/scanner.cpp)
list(APPEND IGEN_SRCS ${CMAKE_BINARY_DIR}/src/tokens.h ${iGenFlex} ${iGenBison})
set_source_files_properties(${iGenFlex} ${iGenBison} ${CMAKE_BINARY_DIR}/src/tokens.h PROPERTIES GENERATED TRUE)

list(APPEND IGEN_SRCS parse/types.h parse/types.cpp)
list(APPEND IGEN_SRCS parse/context.h parse/context.cpp)

set(GEN_SRCS gen/cxx_gen.cpp gen/cxx_gen.h)
source_group(gen FILES ${GEN_SRCS})

source_group(igen FILES ${IGEN_SRCS})

set(PY_SRCS
python/xige.py
python/cpp_gen.py)
source_group(python FILES ${PY_SRCS})

add_library(xige SHARED ${IGEN_SRCS} ${PY_SRCS} parse/xige.h parse/inter.ypp parse/inter.lpp)
target_link_libraries(xige common)
target_compile_definitions(xige PRIVATE BUILD_SHARED=1 BUILD_LIB=1)

add_executable(igen main.cpp ${GEN_SRCS})
target_link_libraries(igen xige)

if(BISON_FOUND)
    add_custom_command(TARGET xige PRE_BUILD
      COMMAND ${BISON_EXECUTABLE} --defines=${CMAKE_BINARY_DIR}/src/tokens.h --output=${iGenBison} ${CMAKE_CURRENT_SOURCE_DIR}/parse/inter.ypp
      COMMENT "Generating parser.cpp")
endif()
if(FLEX_FOUND)
    add_custom_command(TARGET xige PRE_BUILD
      COMMAND ${FLEX_EXECUTABLE} --outfile=${iGenFlex} ${CMAKE_CURRENT_SOURCE_DIR}/parse/inter.lpp
      COMMENT "Generating scanner.cpp")
endif()

add_custom_command(TARGET xige POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/python/xige.py $<TARGET_FILE_DIR:xige>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/python/cpp_gen.py $<TARGET_FILE_DIR:xige>
    COMMENT "Copy python scripts")

target_include_directories(igen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(xige PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(igen PROPERTIES FOLDER "idl")
set_target_properties(xige PROPERTIES FOLDER "idl")