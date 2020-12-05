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



# -----------------------------------------------------------------------------
#
# CONFIGURATION FILE for contextBroker
#
export CONTEXTBROKER_TESTENV_SOURCED=YES


# -----------------------------------------------------------------------------
#
# MAXIMUM_WAIT - maximum time to wait in some processes during startup
#
export MAXIMUM_WAIT=${MAXIMUM_WAIT:-30}



# -----------------------------------------------------------------------------
#
# Ports
#
# o CB_PORT          - port where the main contextBroker listens for connections
#
# o CP1_PORT         - port where the first contextProvider listens for connections
# o CP2_PORT         - port where the second contextProvider listens for connections
# o CP3_PORT         - port where the third contextProvider listens for connections
# o CP4_PORT         - port where the fourth contextProvider listens for connections
# o CP5_PORT         - port where the fifth contextProvider listens for connections
#
# o LISTENER_PORT    - port where listening (test) applications listen for connections
# o LISTENER2_PORT   - port where a second listening (test) application listens for connections
# o LISTENER3_PORT   - port where a third listening (test) application listens for connections
#
# o MQTT_BROKER_PORT - port of the MQTT Broker
# o MQTT_BROKER_HOST - host of the MQTT Broker

export CB_PORT=${CB_PORT:-9999}
export CP1_PORT=${CP1_PORT:-9801}
export CP2_PORT=${CP2_PORT:-9802}
export CP3_PORT=${CP3_PORT:-9803}
export CP4_PORT=${CP4_PORT:-9804}
export CP5_PORT=${CP5_PORT:-9805}
export LISTENER_PORT=${LISTENER_PORT:-9997}
export LISTENER2_PORT=${LISTENER2_PORT:-9977}
export LISTENER3_PORT=${LISTENER3_PORT:-9957}
export MQTT_BROKER_PORT=${MQTT_BROKER_PORT:-1883}
export MQTT_BROKER_HOST=${MQTT_BROKER_HOST:-localhost}



# -----------------------------------------------------------------------------
#
# Log directories
#
# o CB_LOG_DIR        - directory where the 'main' broker keeps its log file
# o CP1_LOG_DIR       - directory where contextProvider1 keeps its log file
# o CP2_LOG_DIR       - directory where contextProvider2 keeps its log file
# o CP3_LOG_DIR       - directory where contextProvider3 keeps its log file
# o CP4_LOG_DIR       - directory where contextProvider4 keeps its log file
# o CP5_LOG_DIR       - directory where contextProvider5 keeps its log file
#
export CB_LOG_DIR=${CB_LOG_DIR:-/var/log/contextBroker}
export CP1_LOG_DIR=${CP1_LOG_DIR:-/tmp/orion/logs/contextProvider1}
export CP2_LOG_DIR=${CP2_LOG_DIR:-/tmp/orion/logs/contextProvider2}
export CP3_LOG_DIR=${CP3_LOG_DIR:-/tmp/orion/logs/contextProvider3}
export CP4_LOG_DIR=${CP4_LOG_DIR:-/tmp/orion/logs/contextProvider4}
export CP5_LOG_DIR=${CP5_LOG_DIR:-/tmp/orion/logs/contextProvider5}



# -----------------------------------------------------------------------------
#
# PID files
#
# o CB_PID_FILE       - path to pid file for the main broker
#
# o CP1_PID_FILE      - path to pid file for the first context provider
# o CP2_PID_FILE      - path to pid file for the second context provider
# o CP3_PID_FILE      - path to pid file for the third context provider
# o CP4_PID_FILE      - path to pid file for the fourth context provider
# o CP5_PID_FILE      - path to pid file for the fifth context provider
#
export CB_PID_FILE=${CB_PID_FILE:-/tmp/orion_${CB_PORT}.pid}
export CP1_PID_FILE=${CP1_PID_FILE:-/tmp/orion_${CP1_PORT}.pid}
export CP2_PID_FILE=${CP2_PID_FILE:-/tmp/orion_${CP2_PORT}.pid}
export CP3_PID_FILE=${CP3_PID_FILE:-/tmp/orion_${CP3_PORT}.pid}
export CP4_PID_FILE=${CP4_PID_FILE:-/tmp/orion_${CP4_PORT}.pid}
export CP5_PID_FILE=${CP5_PID_FILE:-/tmp/orion_${CP5_PORT}.pid}



# -----------------------------------------------------------------------------
#
# Database configuration for all instances of contextBroker
#
export CB_DATABASE_HOST=${CB_DATABASE_HOST:-localhost}
export CB_DATABASE_USER=${CB_DATABASE_USER:-orion}
export CB_DATABASE_PASSWORD=${CB_DATABASE_PASSWORD:-orion}



# -----------------------------------------------------------------------------
#
# Name of the database for the instances of the broker doing harness tests
#
# o CB_DB_NAME       - database for main broker
#
# o CP1_DB_NAME      - database for first context provider
# o CP2_DB_NAME      - database for second context provider
# o CP3_DB_NAME      - database for third context provider
# o CP4_DB_NAME      - database for fourth context provider
# o CP5_DB_NAME      - database for fifth context provider
#


export CB_DB_NAME=${CB_DB_NAME:-ftest}
export CP1_DB_NAME=${CP1_DB_NAME:-ftest_cp1}
export CP2_DB_NAME=${CP2_DB_NAME:-ftest_cp2}
export CP3_DB_NAME=${CP3_DB_NAME:-ftest_cp3}
export CP4_DB_NAME=${CP4_DB_NAME:-ftest_cp4}
export CP5_DB_NAME=${CP5_DB_NAME:-ftest_cp5}
 
