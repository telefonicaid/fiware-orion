#!/bin/bash

# Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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

set -e

docker run --rm -d --name context-server -p 7080:8080  -e MEMORY_ENABLED=true -e LOGGERS_LEVELS_ROOT=TRACE wistefan/context-server

timeout 30 bash -c 'while [[ "$(curl -s -o /dev/null -w ''%{http_code}'' http://localhost:7080/health)" != "200" ]]; do sleep 5; done' || false
