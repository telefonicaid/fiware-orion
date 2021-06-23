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

apt-get -y install wget
wget http://ftp.de.debian.org/debian/pool/main/s/scons/scons_2.5.1-1_all.deb
apt -y --allow-downgrades install ./scons_2.5.1-1_all.deb

echo -e "\e[1;32m Builder: installing mongo cxx driver \e[0m"
git clone https://github.com/FIWARE-Ops/mongo-cxx-driver ${ROOT_FOLDER}/mongo-cxx-driver
cd ${ROOT_FOLDER}/mongo-cxx-driver
scons --disable-warnings-as-errors --use-sasl-client --ssl
scons install --disable-warnings-as-errors --prefix=/usr/local --use-sasl-client --ssl
cd ${ROOT_FOLDER} && rm -Rf mongo-cxx-driver

echo
echo -e "\e[1;32m Debian Builder: check systemd \e[0m"
apt-get -y install --reinstall systemd  # force reinstall systemd
service dbus start

echo
echo -e "\e[1;32m Builder: installing mongo \e[0m"
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 9DA31620334BD75D9DCB49F368818C72E52529D4

echo 'deb [ arch=amd64 ] https://repo.mongodb.org/apt/debian stretch/mongodb-org/4.0 main' > /etc/apt/sources.list.d/mongodb.list
apt-get -y update
apt-get -y install mongodb-org mongodb-org-shell
