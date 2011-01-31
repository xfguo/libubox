cmake_minimum_required(VERSION 2.8)

PROJECT(ubox C)
ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -g3)

SET(SOURCES avl.c blob.c blobmsg.c hash.c uhtbl.c uloop.c)

ADD_LIBRARY(ubox SHARED ${SOURCES})

SET(CMAKE_INSTALL_PREFIX /usr)

FILE(GLOB headers *.h)
INSTALL(FILES ${headers}
	DESTINATION include/libubox
)
INSTALL(TARGETS ubox
	LIBRARY DESTINATION lib
)
