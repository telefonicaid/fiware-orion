#!/bin/bash
# -*- coding: latin-1 -*-
# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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



# -----------------------------------------------------------------------------
#
# style_check
#
function style_check
{
  style_check.sh -d "$1"
  if [ "$?" != 0 ]
  then
    echo Lint Errors:
    cat LINT_ERRORS
    echo "================================================================="
    cat LINT | grep -v "Done processing"
    echo 
    exit 1
  fi
}

style_check src/lib/logMsg

# style_check src/lib/parseArgs
# FIXME: Just keep adding directories here until all of them are included ...
