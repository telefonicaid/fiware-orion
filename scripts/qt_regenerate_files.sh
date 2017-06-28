#!/bin/bash
# -*- coding: latin-1 -*-
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
#

# This small script is useful for Qt Creator users, in order to regenerate the files directory
# The argument is the file in which your Orion Context Broker repository is, e.g:
#
# ./qt_regenerate_files.sh ~/src/fiware-orion

CURRENT=$(pwd)
cd $1
find | egrep '(\.cpp|\.h|\.test|\.txt|\.sh|\.py|\.jmx|\.md|\.apib)$' | grep -v BUILD | grep -v '.git' | grep -v 'archive/' | cut -c 3-
cd $CURRENT
