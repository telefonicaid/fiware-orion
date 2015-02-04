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

# Setting up EPEL Repo
#wget http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
#sudo rpm -ivh epel-release-6-8.noarch.rpm

# Install required packages
sudo apt-get update
sudo apt-get -y install make cmake build-essential scons git libmicrohttpd-dev libboost-dev libboost-thread-dev libboost-filesystem-dev libboost-program-options-dev  libcurl4-gnutls-dev clang libcunit1-dev mongodb-server python python-flask python-jinja2 curl libxml2 netcat-openbsd mongodb valgrind libxslt1.1 libssl-dev libcrypto++-dev

# Install MongoDB C++ Driver
wget http://downloads.mongodb.org/cxx-driver/mongodb-linux-x86_64-2.2.3.tgz
tar xfvz mongodb-linux-x86_64-2.2.3.tgz
cd mongo-cxx-driver-v2.2
git clone https://github.com/mongodb/mongo-cxx-driver.git
cd mongo-cxx-driver/
git checkout 26compat
scons --use-system-boost
sudo scons --use-system-boost --full
sudo chmod a+r -R /usr/local/include/mongo    # It seems that scons install breaks permissions
cd ..

# Install Google Test & Google Mock
wget http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2
tar xfvj gmock-1.5.0.tar.bz2
cd gmock-1.5.0
./configure
make
sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
sudo ldconfig      # just in case... it doesn't hurt :)
cd ..

# Install cantcoap
git clone https://github.com/staropram/cantcoap
cd cantcoap
git checkout 749e22376664dd3adae17492090e58882d3b28a7
make
sudo cp cantcoap.h /usr/local/include
sudo cp dbg.h /usr/local/include
sudo cp nethelper.h /usr/local/include
sudo cp libcantcoap.a /usr/local/lib
cd ..

# Install CoAP Client
wget http://sourceforge.net/projects/libcoap/files/coap-18/libcoap-4.1.1.tar.gz/download
mv download libcoap-4.1.1.tar.gz
tar xvzf libcoap-4.1.1.tar.gz
cd libcoap-4.1.1
./configure
make
sudo cp examples/coap-client /usr/local/bin
cd ..

# Start MongoDB
sudo apt-get install libpcre3            # otherwise, mongod crashes in CentOS 6.3
sudo service mongodb start

# Set up a more convenient workspace
ln -fs /vagrant /home/vagrant/fiware-orion

# Qt Creator building, Projects->Build Steps->Custom Process Step
# command:
#   vagrant 
# arguments:
#   ssh -c 'make di -C fiware-orion'
