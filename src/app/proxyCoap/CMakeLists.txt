# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
# fermin at tid dot es

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET (SOURCES
    proxyCoap.cpp
    HttpProxy.cpp
    HttpMessage.cpp
    CoapController.cpp
)


SET (HEADERS
    HttpProxy.h
    HttpMessage.h
    CoapController.h
)

SET (STATIC_LIBS
    common
    mongoBackend
    ngsiNotify
    serviceRoutines
    convenienceMap
    mongoBackend
    rest
    jsonParse
    xmlParse
    convenience
    ngsi9
    ngsi10
    ngsi
    parse
    common
    lm
    pa
)

# Include directories
# -----------------------------------------------------------------
include_directories("${PROJECT_SOURCE_DIR}/src/app")
include_directories("${PROJECT_SOURCE_DIR}/src/lib")
include_directories("${RAPIDXML_INCLUDE}")


# Lib directories
# ------------------------------------------------------------
link_directories("/usr/local/lib/")
link_directories("/usr/lib64/")


# Executable declaration
# ------------------------------------------------------------
ADD_EXECUTABLE(proxyCoap ${SOURCES} ${HEADERS})

IF ((${DISTRO} STREQUAL "Ubuntu_13.04") OR (${DISTRO} STREQUAL "Ubuntu_12.04.1_LTS"))
  MESSAGE("Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lpthread -lssl -lcrypto -lgnutls -lgcrypt)
ELSEIF(${DISTRO} STREQUAL "openSUSE_13.1")
  MESSAGE("openSUSE DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt -lpthread -lssl -lcrypto)
ELSEIF(${DISTRO} STREQUAL "CentOS_6.3")
  MESSAGE("CentOS DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt)
ELSEIF(${DISTRO} STREQUAL "Fedora_20")
  MESSAGE("Fedora DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread -lboost_filesystem -lboost_system -lpthread)
ELSE()
  MESSAGE("Other DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread-mt -lboost_filesystem-mt -lboost_system-mt)
ENDIF()

# target_link_libraries (proxyCoap cantcoap curl)

INSTALL(TARGETS               proxyCoap
        RUNTIME DESTINATION   bin)
