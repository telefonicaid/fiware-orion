#
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
# Author: José Jaime
#

# FIXME: this script is not going to work... we are no longer using legacy-1.0.2.tar.gz
# This scripts needs a deep update or being removed if it is obsolete and no longer used

# Setting up EPEL Repo
#wget http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
#sudo rpm -ivh epel-release-6-8.noarch.rpm

# Install required packages
sudo apt-get update
sudo apt-get -y install make cmake build-essential scons git libmicrohttpd-dev libboost-dev libboost-thread-dev libboost-filesystem-dev libboost-program-options-dev  libcurl4-gnutls-dev clang libcunit1-dev mongodb-server python python-flask python-jinja2 curl libxml2 netcat-openbsd mongodb valgrind libxslt1.1 libssl-dev libcrypto++-dev

# Install MongoDB C++ Driver
wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.0.2.tar.gz
tar xfvz legacy-1.0.2.tar.gz
cd mongo-cxx-driver-legacy-1.0.2
scons                                         # The build/linux2/normal/libmongoclient.a library is generated as outcome
sudo scons install --prefix=/usr/local        # This puts .h files in /usr/local/include/mongo and libmongoclient.a in /usr/local/lib
cd ..

# Install Google Test & Google Mock
# URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but google removed tha package in late August 2016
wget https://www.fiware.org/shared-content/public/gmock-1.5.0.tar.bz2
tar xfvj gmock-1.5.0.tar.bz2
cd gmock-1.5.0
./configure
make
sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
sudo ldconfig      # just in case... it doesn't hurt :)
cd ..

# Install Rapidjson
wget https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz
tar xfvz v1.1.0.tar.gz
sudo mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include

# Start MongoDB
sudo apt-get install libpcre3            # otherwise, mongod crashes in CentOS 6.3
sudo service mongodb start
