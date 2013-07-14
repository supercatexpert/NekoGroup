INCLUDE_DIRECTORIES(${DEPS_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
LINK_DIRECTORIES(${DEPS_LIBRARY_DIRS})

SET(NekoGroupPasswordHelper_SOURCES
	ng-password-helper.c)

SET(NekoGroupPasswordHelper_HEADERS
	ng-password-helper.h)

ADD_EXECUTABLE(NekoGroupPasswordHelper ${NekoGroupPasswordHelper_SOURCES} ${NekoGroupPasswordHelper_HEADERS})
INSTALL(TARGETS NekoGroupPasswordHelper RUNTIME DESTINATION bin)
TARGET_LINK_LIBRARIES(NekoGroupPasswordHelper ${DEPS_LIBRARIES})

