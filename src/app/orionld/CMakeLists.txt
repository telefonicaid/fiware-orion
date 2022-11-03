# Copyright 2018 FIWARE Foundation e.V.
#
# This file is part of Orion-LD Context Broker.
#
# Orion-LD Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion-LD Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# orionld at fiware dot org

SET (SOURCES
    orionld.cpp
    orionRestServices.cpp
    orionldRestServices.cpp
)

SET (HEADERS
    orionRestServices.h
)


SET (STATIC_LIBS
    ${ORION_LIBS}
    ${COMMON_STATIC_LIBS}
    kjson.a
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
ADD_EXECUTABLE(orionld ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(orionld
  ${STATIC_LIBS}
  ${DYNAMIC_LIBS}
  -lmongoclient
  ${Boost_LIBRARIES}
  ${LIBMICROHTTPD_LIBRARIES}
)

IF (${DISTRO} MATCHES "Ubuntu.*")
  MESSAGE("orionld: INSTALL Ubuntu DISTRO: '${DISTRO}'")
  INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/etc/init.d/orionld.ubuntu DESTINATION /etc/init.d RENAME orionld)
  INSTALL(FILES ${CMAKE_SOURCE_DIR}/etc/config/orionld DESTINATION /etc/default RENAME orionld)
ENDIF()

INSTALL(TARGETS               orionld
        RUNTIME DESTINATION   bin)
