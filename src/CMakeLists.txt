INCLUDE_DIRECTORIES(${DEPS_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})
LINK_DIRECTORIES(${DEPS_LIBRARY_DIRS})

SET(NekoGroup_SOURCES
	main.c
	ng-bot.c
	ng-cmd.c
	ng-config.c
	ng-core.c
	ng-db.c
	ng-main.c
	ng-marshal.c
	ng-utils.c)

SET(NekoGroup_HEADERS
	ng-bot.h
	ng-cmd.h
	ng-common.h
	ng-config.h
	ng-core.h
	ng-db.h
	ng-main.h
	ng-marshal.h
	ng-utils.h)

GLIB_MARSHAL("ng_marshal" "ng-marshal" "${CMAKE_CURRENT_BINARY}")
ADD_SCHEMA(prj.NekoGroup.gschema.xml)
ADD_EXECUTABLE(NekoGroup ${NekoGroup_SOURCES} ${NekoGroup_HEADERS})
INSTALL(TARGETS NekoGroup RUNTIME DESTINATION bin)
TARGET_LINK_LIBRARIES(NekoGroup ${DEPS_LIBRARIES})

ADD_SUBDIRECTORY(password-helper)

