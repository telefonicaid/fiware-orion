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
    orionld.cpp
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



# Executable declaration
# ------------------------------------------------------------

ADD_EXECUTABLE(orionld ${SOURCES} ${HEADERS})

MESSAGE("orionld distro: '${DISTRO}'")

# The following ones are distros that we have checked are built correctly

IF(${DISTRO} MATCHES "CentOS_6.*")
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} MATCHES "CentOS_7.*")
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} MATCHES "Debian_.*")
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})

# The following ones are distros that haven't been checked since the refactoring in
# PR #2062. They should be "promoted" as soon as we have positive feedback about
# them or fixed (maybe based in the version of this file previous to
# the aforementioned PR)

ELSEIF ((${DISTRO} STREQUAL "Ubuntu_13.04") OR (${DISTRO} STREQUAL "Ubuntu_12.04.1_LTS") OR (${DISTRO} STREQUAL "Ubuntu_12.04.2_LTS"))
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})
ELSEIF((${DISTRO} STREQUAL "Ubuntu_13.10") OR (${DISTRO} STREQUAL "Ubuntu_14.04.1_LTS") OR (${DISTRO} STREQUAL "Ubuntu_14.10"))    
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ELSEIF((${DISTRO} STREQUAL "Ubuntu_14.04.2_LTS") OR (${DISTRO} STREQUAL "Ubuntu_14.04.3_LTS") OR (${DISTRO} STREQUAL "Ubuntu_14.04.4_LTS") OR (${DISTRO} MATCHES "Linux_Mint_.*"))    
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} STREQUAL "openSUSE_13.1")  
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST_MT} ${DYNAMIC_LIBS})
ELSEIF(${DISTRO} STREQUAL "Fedora_20")  
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} ${BOOST} ${DYNAMIC_LIBS})

# Finally this is a "best effort" option to try to build on distros that don't have a
# specify switch

ELSE()  
  TARGET_LINK_LIBRARIES(orionld ${STATIC_LIBS} -lmongoclient ${BOOST} ${DYNAMIC_LIBS})
ENDIF()

IF (${DISTRO} MATCHES "Ubuntu.*")
  MESSAGE("orionld: INSTALL Ubuntu DISTRO: '${DISTRO}'")
  INSTALL(PROGRAMS ${CMAKE_SOURCE_DIR}/etc/init.d/orionld.ubuntu DESTINATION /etc/init.d RENAME orionld)
  INSTALL(FILES ${CMAKE_SOURCE_DIR}/etc/config/orionld DESTINATION /etc/default RENAME orionld)
ENDIF()

INSTALL(TARGETS               orionld
        RUNTIME DESTINATION   bin)
