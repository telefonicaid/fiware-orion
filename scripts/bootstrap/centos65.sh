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
# Author: Leandro Guillen
#

# FIXME: this script is not going to work... we are no longer using mongodb-linux-x86_64-2.2.3.tgz
# This script needs a deep update or being removed if it is obsolete and no longer used

# Setting up EPEL Repo
wget http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
sudo rpm -ivh epel-release-6-8.noarch.rpm

# Install required packages
sudo yum -y install make cmake gcc-c++ scons git libmicrohttpd-devel boost-devel libcurl-devel clang CUnit-devel mongodb-server python python-flask python-jinja2 curl libxml2 nc mongodb-org valgrind libxslt

# Install MongoDB C++ Driver
wget http://downloads.mongodb.org/cxx-driver/mongodb-linux-x86_64-2.2.3.tgz
tar xfvz mongodb-linux-x86_64-2.2.3.tgz
cd mongo-cxx-driver-v2.2
scons                                         # The build/libmongoclient.a library is generated as outcome
sudo scons install                            # This puts .h files in /usr/local/include and libmongoclient.a in /usr/local/lib
sudo chmod a+r -R /usr/local/include/mongo    # It seems that scons install breaks permissions
cd ..

# Install Google Test & Google Mock
# URL was http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 but google removed that package in late August 2016
wget https://www.fiware.org/shared-content/public/gmock-1.5.0.tar.bz2
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
sudo yum update pcre            # otherwise, mongod crashes in CentOS 6.3
sudo /etc/init.d/mongod start

# Set up a more convenient workspace
ln -fs /vagrant /home/vagrant/fiware-orion

# Qt Creator building, Projects->Build Steps->Custom Process Step
# command:
#   vagrant 
# arguments:
#   ssh -c 'make di -C fiware-orion'
