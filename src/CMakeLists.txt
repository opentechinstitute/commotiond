SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

SET(DAEMONSRC daemon.c)
SET(LIBSRC debug.h extern/wpa_ctrl.c extern/wpa_ctrl.h extern/halloc.c extern/halloc.h iface.c iface.h loop.c loop.h msg.c msg.h process.c process.h profile.c profile.h socket.c socket.h util.c util.h  extern/list.c extern/list.h  extern/tst.h extern/tst.c olsrd.c olsrd.h id.c id.h obj.c obj.h list.c list.h tree.c tree.h cmd.c cmd.h plugin.c plugin.h)
SET(CLIENTSRC client.c)

ADD_EXECUTABLE(daemon ${DAEMONSRC})
ADD_EXECUTABLE(client ${CLIENTSRC})
ADD_LIBRARY(commotion SHARED ${LIBSRC})

TARGET_LINK_LIBRARIES(commotion rt)
TARGET_LINK_LIBRARIES(daemon commotion)
TARGET_LINK_LIBRARIES(client commotion)

SET_TARGET_PROPERTIES(daemon PROPERTIES OUTPUT_NAME commotiond)
SET_TARGET_PROPERTIES(client PROPERTIES OUTPUT_NAME commotion)

INSTALL(TARGETS commotion DESTINATION lib)
INSTALL(TARGETS daemon RUNTIME DESTINATION sbin)
INSTALL(TARGETS client RUNTIME DESTINATION bin)
