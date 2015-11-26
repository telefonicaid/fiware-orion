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
# iot_support at tid dot es

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
link_directories("/usr/lib/x86_64-linux-gnu")

# FIXME P4: Issue #476: remove or keep?
link_directories("/opt/lib/")



# Executable declaration
# ------------------------------------------------------------
ADD_EXECUTABLE(proxyCoap ${SOURCES} ${HEADERS})

IF(${DISTRO} STREQUAL "Fedora_20")
  MESSAGE("proxyCoap: Fedora DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_13.10")
  MESSAGE("proxyCoap: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl boost_thread boost_system pthread)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.04.1_LTS")
  # FIXME P4: Issue #476: remove or keep?
  MESSAGE("proxyCoap: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl boost_thread boost_system pthread)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.04.3_LTS")
  # FIXME P4: Issue #476: remove or keep?
  MESSAGE("proxyCoap: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl boost_thread boost_system pthread)
ELSEIF(${DISTRO} STREQUAL "Ubuntu_14.10")
  # ADDED FOR UBUNTU 14.10
  MESSAGE("contextBroker: Ubuntu DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl boost_thread boost_system pthread)
ELSEIF(${DISTRO} STREQUAL "Debian_8.0")
  MESSAGE("proxyCoap: Debian DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl boost_thread boost_system pthread)
ELSEIF(${DISTRO} MATCHES "CentOS_6.*")
  MESSAGE("proxyCoap: CentOS DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread-mt)
ELSE()
  MESSAGE("proxyCoap: Other DISTRO: '${DISTRO}'")
  TARGET_LINK_LIBRARIES(proxyCoap ${STATIC_LIBS} cantcoap curl -lboost_thread -lboost_system -lpthread)
ENDIF()

# target_link_libraries (proxyCoap cantcoap curl)

INSTALL(TARGETS               proxyCoap
        RUNTIME DESTINATION   bin)
