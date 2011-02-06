cmake_minimum_required(VERSION 2.8)

PROJECT(ubox C)
ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -g3)

IF(APPLE)
  INCLUDE_DIRECTORIES(/opt/local/include)
  LINK_DIRECTORIES(/opt/local/lib)
ENDIF()

SET(SOURCES avl.c blob.c blobmsg.c uloop.c usock.c)

ADD_LIBRARY(ubox SHARED ${SOURCES})
ADD_LIBRARY(blobmsg_json SHARED blobmsg_json.c)

TARGET_LINK_LIBRARIES(blobmsg_json ubox json)

SET(CMAKE_INSTALL_PREFIX /usr)

FILE(GLOB headers *.h)
INSTALL(FILES ${headers}
	DESTINATION include/libubox
)
INSTALL(TARGETS ubox blobmsg_json
	LIBRARY DESTINATION lib
)
