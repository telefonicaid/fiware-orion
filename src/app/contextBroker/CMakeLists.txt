# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es

SET (SOURCES
    contextBroker.cpp
    orionRestServices.cpp
)

SET (HEADERS
    orionRestServices.h
)


SET (STATIC_LIBS
    ${ORION_LIBS}
    ${COMMON_STATIC_LIBS}
)

# Include directories
# ------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/app")
include_directories("${PROJECT_SOURCE_DIR}/src/lib")

# Lib directories
# ------------------------------------------------------------
link_directories("/usr/local/lib/")
link_directories("/usr/lib64/")
link_directories("/usr/lib/x86_64-linux-gnu")
link_directories("${Boost_LIBRARY_DIRS}")
link_directories("${LIBMICROHTTPD_LIBRARY_DIRS}")


# Executable declaration
# ------------------------------------------------------------

ADD_EXECUTABLE(contextBroker ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(contextBroker
  ${STATIC_LIBS}
  ${DYNAMIC_LIBS}
  -lmongoclient
  ${Boost_LIBRARIES}
  ${LIBMICROHTTPD_LIBRARIES}
)

IF (${DISTRO} MATCHES "Ubuntu.*")
  MESSAGE("contextBroker: INSTALL Ubuntu DISTRO: '${DISTRO}'")
  INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/etc/init.d/contextBroker.ubuntu DESTINATION /etc/init.d RENAME contextBroker)
  INSTALL(FILES ${CMAKE_SOURCE_DIR}/etc/config/contextBroker DESTINATION /etc/default RENAME contextBroker)
ENDIF()

INSTALL(TARGETS               contextBroker
        RUNTIME DESTINATION   bin)
