#!/bin/bash

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


set -e

echo
echo -e "\e[1;32m Builder: installing libmicrohttpd \e[0m"
curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.72.tar.gz | tar xzC ${ROOT_FOLDER}
cd ${ROOT_FOLDER}/libmicrohttpd-0.9.72
./configure --disable-messages --disable-postprocessor --disable-dauth
make
make install
cd ${ROOT_FOLDER} && rm -Rf libmicrohttpd-0.9.72
