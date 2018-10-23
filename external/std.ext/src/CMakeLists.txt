set(STDEXT_SRC
    socket.cpp
    win32.cpp
    ../include/ext/socket.hpp
    ../include/ext/win32.hpp
)
source_group(src FILES ${STDEXT_SRC})
add_library(stdext STATIC ${STDEXT_SRC})

