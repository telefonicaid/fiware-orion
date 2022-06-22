#!/bin/bash

# Copyright 2022 Telefonica Investigacion y Desarrollo, S.A.U
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
echo -e "\e[1;32m Builder: installing prometheus client library\e[0m"
git clone https://github.com/digitalocean/prometheus-client-c.git ${ROOT_FOLDER}/prometheus-client-c
cd ${ROOT_FOLDER}/prometheus-client-c
pwd
git checkout release-0.1.3
sed 's/\&promhttp_handler,/(MHD_AccessHandlerCallback) \&promhttp_handler,/' promhttp/src/promhttp.c > XXX
mv XXX promhttp/src/promhttp.c
./auto build && ./auto package

cp promhttp/build/libpromhttp.so prom/build/libprom.so /usr/local/lib/
ls -l /usr/local/lib/libprom*.so
