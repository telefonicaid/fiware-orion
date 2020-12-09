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

OPS_DEPS_CORE=(
  'libcurl3' \
  'libssl1.1' \
  'ca-certificates' \
)

OPS_DEPS_BOOST=(
 'libboost-filesystem' \
 'libboost-regex' \
 'libboost-thread' \
)

apt-get -y update

apt-get -y install -f --no-install-recommends 'libboost-all-dev'

BOOST_VER=$(apt-cache policy libboost-all-dev | grep Installed | awk '{ print $2 }' | cut -c -6)
echo
echo -e "\e[1;32m Builder: installing boost ops deps \e[0m"
for i in ${OPS_DEPS_BOOST[@]}; do TO_INSTALL="${TO_INSTALL} ${i}${BOOST_VER}"; done
apt-get install -y ${TO_INSTALL[@]}

echo
echo -e "\e[1;32m Builder: installing core ops deps \e[0m"
apt-get -y install --no-install-recommends \
    ${OPS_DEPS_CORE[@]}