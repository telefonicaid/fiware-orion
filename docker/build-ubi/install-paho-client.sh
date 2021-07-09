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
echo -e "\e[1;32m Debian Builder: installing Paho MQTT C library \e[0m"
wget http://mirror.centos.org/centos/7/os/x86_64/Packages/doxygen-1.8.5-4.el7.x86_64.rpm
yum install -y doxygen-1.8.5-4.el7.x86_64.rpm


rm -f /usr/local/lib/libpaho*                                                 # OK
git clone https://github.com/eclipse/paho.mqtt.c.git ${ROOT_FOLDER}/paho.mqtt.c      # OK
cd ${ROOT_FOLDER}/paho.mqtt.c                                                        # OK
git fetch -a
git checkout tags/v1.3.1                                                      # OK - git checkout develop ...
make html                                                                     # OK

echo -e "\e[1;32m Building Paho MQTT C Library \e[0m"
make > /tmp/paho-build 2&>1 || /bin/true
echo -e "\e[1;32m Paho Built ... \e[0m"
echo "============== PAHO BUILD TRACES START ============================="
cat /tmp/paho-build
echo "============== PAHO BUILD TRACES END ==============================="

echo -e "\e[1;32m Installing Paho MQTT C Library \e[0m"
make install > /tmp/paho-install 2&>1 || /bin/true                            # ... ?
echo -e "\e[1;32m Paho Installed ... \e[0m"
echo "============== PAHO INSTALLATION TRACES START ============================="
cat /tmp/paho-install
echo "============== PAHO INSTALLATION TRACES END ==============================="