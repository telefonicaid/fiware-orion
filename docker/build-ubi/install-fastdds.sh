#!/bin/bash

# Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
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

# yum -y --nogpgcheck install https://dl.rockylinux.org/pub/rocky/9/devel/x86_64/os/Packages/t/tinyxml2-devel-7.0.1-7.el9.x86_64.rpm
# yum -y --nogpgcheck install https://dl.rockylinux.org/pub/rocky/9/devel/x86_64/os/Packages/a/asio-devel-1.16.1-6.el9.x86_64.rpm

yum -y --nogpgcheck install https://dl.fedoraproject.org/pub/fedora/linux/releases/39/Everything/x86_64/os/Packages/t/tinyxml2-devel-9.0.0-3.fc39.x86_64.rpm
yum -y --nogpgcheck install https://dl.fedoraproject.org/pub/fedora/linux/releases/39/Everything/x86_64/os/Packages/a/asio-devel-1.28.1-2.fc39.x86_64.rpm

mkdir /opt/Fast-DDS
cd /opt/Fast-DDS

#
# foonathan_memory_vendor
#
git clone https://github.com/eProsima/foonathan_memory_vendor.git
mkdir foonathan_memory_vendor/build
cd foonathan_memory_vendor/build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_SHARED_LIBS=ON
cmake --build . --target install
cd -

#
# CDR
#
git clone https://github.com/eProsima/Fast-CDR.git
mkdir Fast-CDR/build
cd Fast-CDR/build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_SHARED_LIBS=ON
cmake --build . --target install
cd -

#
# RTPS
#
git clone https://github.com/eProsima/Fast-DDS.git
mkdir Fast-DDS/build

cd Fast-DDS/build
cmake ..  -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_SHARED_LIBS=ON
cmake --build . --target install
