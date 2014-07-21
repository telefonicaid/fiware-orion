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
# fermin at tid dot es

#####
#
# Configuration file for contextBroker
#
#####

export CONTEXTBROKER_TESTENV_SOURCED=YES

# BROKER_PORT - the port/socket where contextBroker will listen for connections
if [ -z "${BROKER_PORT}" ]; then
    BROKER_PORT=9999
fi
if [ -z "${BROKER2_PORT}" ]; then
    BROKER2_PORT=9998
fi

if [ -z "${CM_PORT}" ]; then
    CM_PORT=9998
fi
if [ -z "${COAP_PORT}" ]; then
    COAP_PORT=5683
fi

# LISTENER_PORT - the port/socket where listening application for some test cases listens for connections
if [ -z "${LISTENER_PORT}" ]; then
    LISTENER_PORT=9997
fi

# LISTENER2_PORT - the port/socket where listening application for some test cases involving more than one listerner instance
if [ -z "${LISTENER2_PORT}" ]; then
    LISTENER2_PORT=9977
fi


# MAXIMUM_WAIT - maximum time to wait in some processes during startup
if [ -z "${MAXIMUM_WAIT}" ]; then
    MAXIMUM_WAIT=30
fi

# BROKER_LOG_DIR - Where to log to
if [ -z "${BROKER_LOG_DIR}" ]; then
    BROKER_LOG_DIR=/var/log/contextBroker
fi

# BROKER_PID_FILE - Where to store the pid for contextBroker
if [ -z "${BROKER_PID_FILE}" ]; then
    BROKER_PID_FILE=/tmp/orion_${BROKER_PORT}.pid
fi
if [ -z "${BROKER_PID2_FILE}" ]; then
    BROKER_PID2_FILE=/tmp/orion_${BROKER2_PORT}.pid
fi

## Database configuration for orion-broker
if [ -z "${BROKER_DATABASE_HOST}" ]; then
    BROKER_DATABASE_HOST=localhost
fi
if [ -z "${BROKER_DATABASE_NAME}" ]; then
    BROKER_DATABASE_NAME=testharness
fi
if [ -z "${BROKER_DATABASE2_NAME}" ]; then
    BROKER_DATABASE2_NAME=testharness2
fi
if [ -z "${BROKER_DATABASE_USER}" ]; then
    BROKER_DATABASE_USER=orion
fi
if [ -z "${BROKER_DATABASE_PASSWORD}" ]; then
    BROKER_DATABASE_PASSWORD=orion
fi

export BROKER_USER BROKER_PORT BROKER2_PORT LISTENER_PORT LISTENER2_PORT BROKER_LOG_DIR BROKER_PID_FILE BROKER_PID2_FILE BROKER_DATABASE_HOST BROKER_DATABASE_NAME BROKER_DATABASE2_NAME BROKER_DATABASE_USER BROKER_DATABASE_PASSWORD CM_PORT MAXIMUM_WAIT COAP_PORT

#
# The following two lines are commented because they destroy "git diff" in some cases
#
# PATH=${HOME}/bin:${PATH}:$(pwd)/script:$(pwd)/BUILD_DEBUG/src
# export PATH
