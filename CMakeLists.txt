cmake_minimum_required(VERSION 2.6)

PROJECT(ubox C)
ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -g3)

IF(APPLE)
  INCLUDE_DIRECTORIES(/opt/local/include)
  LINK_DIRECTORIES(/opt/local/lib)
ENDIF()

SET(SOURCES avl.c blob.c blobmsg.c uloop.c usock.c)

ADD_LIBRARY(ubox SHARED ${SOURCES})

SET(CMAKE_INSTALL_PREFIX /usr)

FILE(GLOB headers *.h)
INSTALL(FILES ${headers}
	DESTINATION include/libubox
)
INSTALL(TARGETS ubox
	LIBRARY DESTINATION lib
)

find_library(json json)
IF(EXISTS ${json})
	ADD_LIBRARY(blobmsg_json SHARED blobmsg_json.c)
	TARGET_LINK_LIBRARIES(blobmsg_json ubox json)
	INSTALL(TARGETS blobmsg_json
		LIBRARY DESTINATION lib
	)
ENDIF()
