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

yum install --nogpgcheck -y openssl-devel

wget https://cmake.org/files/v3.12/cmake-3.12.3.tar.gz
tar zxvf cmake-3.*
cd cmake-3.12.3
./bootstrap --prefix=/usr/local
make -j$(nproc)
make install

echo
echo -e "\e[1;32m Builder: installing mongo c driver \e[0m"
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.5/mongo-c-driver-1.17.5.tar.gz
tar xzf mongo-c-driver-1.17.5.tar.gz
cd mongo-c-driver-1.17.5
mkdir cmake-build
cd cmake-build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
make install