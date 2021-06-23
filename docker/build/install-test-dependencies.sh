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

TEST_TOOLS=(
 'bc' \
 'nano' \
 'netcat' \
 'python3-pip' \
 'gridsite-clients' \
 'valgrind' \
)

echo -e "\e[1;32m Builder: installing gmock \e[0m"
apt-get -y install libgtest-dev google-mock
# install gtest lib
cd /usr/src/googletest/googletest
cmake CMakeLists.txt
make

# install gmock lib
cd /usr/src/googletest/googlemock
cmake CMakeLists.txt
make

echo
echo -e "\e[1;32m Builder: installing  tools and dependencies \e[0m"
apt-get -y install --no-install-recommends \
    ${TEST_TOOLS[@]}

echo
echo -e "\e[1;32m Builder: installing python dependencies \e[0m"
pip install --upgrade setuptools wheel
pip install Flask==1.0.2 pyOpenSSL==20.0.0 # paho-mqtt
pip install paho-mqtt
yes | pip uninstall setuptools wheel
