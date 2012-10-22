cmake_minimum_required(VERSION 2.6)

PROJECT(ubox C)
ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -g3 -Wmissing-declarations)

OPTION(BUILD_LUA "build Lua plugin" ON)

IF(APPLE)
  INCLUDE_DIRECTORIES(/opt/local/include)
  LINK_DIRECTORIES(/opt/local/lib)
ENDIF()

SET(SOURCES avl.c avl-cmp.c blob.c blobmsg.c uloop.c usock.c ustream.c ustream-fd.c vlist.c)

ADD_LIBRARY(ubox SHARED ${SOURCES})

SET(CMAKE_INSTALL_PREFIX /usr)

FILE(GLOB headers *.h)
INSTALL(FILES ${headers}
	DESTINATION include/libubox
)
INSTALL(TARGETS ubox
	LIBRARY DESTINATION lib
)

ADD_SUBDIRECTORY(lua)

find_library(json json)
IF(EXISTS ${json})
	ADD_LIBRARY(blobmsg_json SHARED blobmsg_json.c)
	TARGET_LINK_LIBRARIES(blobmsg_json ubox json)

	ADD_EXECUTABLE(jshn jshn.c)
	TARGET_LINK_LIBRARIES(jshn json)

	INSTALL(TARGETS blobmsg_json jshn
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
	)

	FILE(GLOB scripts sh/*.sh)
	INSTALL(FILES ${scripts}
		DESTINATION share/libubox
	)

ENDIF()
