#!/bin/bash
# -*- coding: latin-1 -*-
# Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
#
# Author: Ken Zangelin

# NOTE: this script is designed to be launched from makefile targets. Thus,
# the call to style_check.sh may break if you attempt to use it from a
# different place. 

continue=No
if [ "$1" = "--continue" ]
then
  continue=Yes
fi


# -----------------------------------------------------------------------------
#
# cpp_check
#
function cpp_check
{
  dir=$1
  echo "cppcheck $dir"
  cppcheck --error-exitcode=42 -j 8 --enable=all -I src/lib -I test/ -I test/unittests $dir > /dev/null 2> /tmp/cppcheck.log
  if [ "$?" != 0 ]
  then
    echo cppcheck errors in $dir:
    echo "================================================================="
    cat /tmp/cppcheck.log
    echo "================================================================="

    if [ "$continue" == "No" ]
    then
      exit 1
    fi

    echo
    echo
  fi
}

cpp_check src/lib/logSummary
cpp_check src/lib/jsonParseV2
cpp_check src/lib/apiTypesV2
cpp_check src/lib/mongoBackend
cpp_check src/lib/serviceRoutinesV2
cpp_check src/lib/logMsg
cpp_check src/lib/parseArgs
cpp_check src/lib/cache
cpp_check src/lib/alarmMgr
cpp_check src/lib/metricsMgr
cpp_check test/unittests
cpp_check test/unittests/orionTypes
cpp_check test/unittests/jsonParse
cpp_check test/unittests/apiTypesV2
cpp_check test/unittests/cache
cpp_check test/unittests/mongoBackend
cpp_check test/unittests/rest
cpp_check test/unittests/serviceRoutines

# FIXME: Just keep adding directories here until all of them are included:

# cpp_check src/app/contextBroker
# cpp_check src/lib/ngsiNotify
# cpp_check src/lib/parse
# cpp_check src/lib/rest
# cpp_check src/lib/common
# cpp_check src/lib/orionTypes
# cpp_check src/lib/convenience
# cpp_check src/lib/jsonParse
# cpp_check src/lib/ngsi10
# cpp_check src/lib/ngsi9
# cpp_check src/lib/serviceRoutines
# cpp_check src/lib/ngsi
#
# cpp_check test/unittests
# cpp_check test/unittests/common
# cpp_check test/unittests/parse
# cpp_check test/unittests/convenience
# cpp_check test/unittests/ngsi10
# cpp_check test/unittests/ngsi9
# cpp_check test/unittests/ngsi
#
