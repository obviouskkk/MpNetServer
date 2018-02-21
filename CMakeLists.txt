CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE  true )

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
SET(CMAKE_USE_RELATIVE_PATHS  true )
FIND_PACKAGE(glib REQUIRED)
#INCLUDE_DIRECTORIES( /usr/include/liblog/)

#-fhosted 宿主环境编译 -D_REENTRANT 多线程可重入 -z defs在创建.so文件时禁止未定义的符号，以便在编译时而不是运行时捕获此问题
ADD_DEFINITIONS("-Wall -Wno-unused -g    -fhosted -D_REENTRANT -D_GNU_SOURCE  -z defs")

INCLUDE_DIRECTORIES( ${GLIB_INCLUDE_DIR}  )
INCLUDE_DIRECTORIES(${GLIB_PKG_INCLUDE_DIRS})

SET (EXTRA_LIBS ${EXTRA_LIBS} glib-2.0  dl  pthread )

AUX_SOURCE_DIRECTORY(./src/ SRC_LIST )
ADD_EXECUTABLE(server ${SRC_LIST} ) 
TARGET_LINK_LIBRARIES (server ${EXTRA_LIBS})
#INSTALL (FILES  atomic.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  config.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  List.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  tcp.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  net_if.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  service.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  net.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  bindconf.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  daemon.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  shmq.h DESTINATION  include/quiet_serv    )
#INSTALL (FILES  dll.h DESTINATION  include/quiet_serv    )
