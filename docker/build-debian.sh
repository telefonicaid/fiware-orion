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
#
# Author: Dmitrii Demin
#

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

BUILD_DEPS=(
 'libboost-all-dev' \
 'libcurl4-openssl-dev' \
 'libgcrypt-dev' \
 'libgnutls28-dev' \
 'libsasl2-dev' \
 'libssl-dev' \
 'uuid-dev' \
)

BUILD_TOOLS=(
 'apt-transport-https' \
 'bzip2' \
 'ca-certificates' \
 'cmake' \
 'curl' \
 'dirmngr' \
 'g++' \
 'gcc' \
 'git' \
 'gnupg' \
 'make' \
 'scons' \
)

TEST_TOOLS=(
 'bc' \
 'nano' \
 'netcat' \
 'python-pip' \
 'gridsite-clients' \
 'valgrind' \
)

TO_CLEAN=(
 'cmake-data' \
 'cpp-6' \
 'git-man' \
 'manpages' \
 'libc6-dev' \
 'libgcc-6-dev' \
 'linux-libc-dev' \
 'libgpg-error-dev' \
 'libedit2' \
 'openssl' \
 'openssh-client' \
)

echo -e "\e[1;32m Builder: create folders and user \e[0m"
useradd -s /bin/false -r ${ORION_USER}
mkdir -p /var/{log,run}/${BROKER}

echo
echo -e "\e[1;32m Builder: update apt \e[0m"
apt-get -y update

echo
echo -e "\e[1;32m Builder: installing tools and dependencies \e[0m"
apt-get -y install -f --no-install-recommends \
    ${BUILD_TOOLS[@]} \
    ${BUILD_DEPS[@]}

echo
echo -e "\e[1;32m Builder: installing mongo cxx driver \e[0m"
git clone https://github.com/FIWARE-Ops/mongo-cxx-driver ${ROOT}/mongo-cxx-driver
cd ${ROOT}/mongo-cxx-driver
scons --disable-warnings-as-errors --use-sasl-client --ssl
scons install --disable-warnings-as-errors --prefix=/usr/local --use-sasl-client --ssl
cd ${ROOT} && rm -Rf mongo-cxx-driver

echo
echo -e "\e[1;32m Builder: installing rapidjson \e[0m"
curl -L https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz | tar xzC ${ROOT}
mv ${ROOT}/rapidjson-1.0.2/include/rapidjson/ /usr/local/include
cd ${ROOT} && rm -Rf rapidjson-1.0.2

echo
echo -e "\e[1;32m Builder: installing libmicrohttpd \e[0m"
curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz | tar xzC ${ROOT}
cd ${ROOT}/libmicrohttpd-0.9.48
./configure --disable-messages --disable-postprocessor --disable-dauth
make
make install
cd ${ROOT} && rm -Rf libmicrohttpd-0.9.48

# Resolve the issue in travis about docker build
# the postinst for server includes "systemctl daemon-reload" (and we don't have "systemctl")

#echo ln to /bin/systemctl solves the issue.
echo
echo -e "\e[1;32m Debian Builder: check systemd \e[0m"
# dpkg -l | grep systemd      # Library exist
# echo $PATH           # No place to find in path
# whereis systemctl    # It is not found systemctl in the system
apt-get -y install --reinstall systemd  # force reinstall systemd
# dpkg-query -S /bin/systemctl # Query if systemctl is installed
service dbus start

echo
echo -e "\e[1;32m Builder: installing mongo \e[0m"
apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 9DA31620334BD75D9DCB49F368818C72E52529D4

echo 'deb [ arch=amd64 ] https://repo.mongodb.org/apt/debian stretch/mongodb-org/4.0 main' > /etc/apt/sources.list.d/mongodb.list
apt-get -y update
apt-get -y install mongodb-org mongodb-org-shell

echo
echo -e "\e[1;32m Debian Builder: installing k libs \e[0m"
for kproj in kbase klog kalloc kjson khash
do
    git clone https://gitlab.com/kzangeli/${kproj}.git ${ROOT}/$kproj
done

for kproj in kbase klog kalloc kjson khash
do
    cd ${ROOT}/$kproj
    git checkout release/0.5
    make
    make install
done

echo
echo -e "\e[1;32m Debian Builder: installing Paho MQTT C library \e[0m"
apt-get -y install doxygen                                                    # OK - with -y. NOT OK without -y !!!
apt-get -y install graphviz 
rm -f /usr/local/lib/libpaho*                                                 # OK
git clone https://github.com/eclipse/paho.mqtt.c.git ${ROOT}/paho.mqtt.c      # OK
cd ${ROOT}/paho.mqtt.c                                                        # OK
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

echo
echo -e "\e[1;32m Builder: installing MQTT - not! \e[0m"
#
# FIXME
#   For unknown reasons, 'mosquitto' can't be installed in this repo
#   As workaround, all MQTT functests are disabled for travis. 
#
# echo "Builder: installing and starting mosquitto"
# apt-get -y install mosquitto
# sudo service mosquitto start
#


ldconfig

if [[ "${STAGE}" == 'deps' ]]; then
    echo
    echo -e "\e[1;32m Builder: installing gmock \e[0m"
    #curl -L https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2 | tar xjC ${ROOT}
    #cd ${ROOT}/gmock-1.5.0
    #./configure
    #make
    #make install
    #cd ${ROOT} && rm -Rf gmock-1.5.0
    apt-get -y install libgtest-dev google-mock

    echo
    echo -e "\e[1;32m Builder: installing  tools and dependencies \e[0m"
    apt-get -y install --no-install-recommends \
        ${TEST_TOOLS[@]}

    echo
    echo -e "\e[1;32m Builder: installing python dependencies \e[0m"
    pip install --upgrade setuptools wheel
    pip install Flask==1.0.2 pyOpenSSL==19.0.0 # paho-mqtt
    yes | pip uninstall setuptools wheel
fi

if [[ ${STAGE} == 'release' ]]; then
    echo
    echo -e "\e[1;32m Debian Builder: compiling and installing orion as a DEBUGGABLE executable \e[0m"
    git clone ${REPOSITORY} ${PATH_TO_SRC}
    cd ${PATH_TO_SRC}
    git checkout ${REV}
    make debug install
    strip /usr/bin/${BROKER}

    BOOST_VER=$(apt-cache policy libboost-all-dev | grep Installed | awk '{ print $2 }' | cut -c -6)

    echo
    echo -e "\e[1;32m Builder: cleaning1 \e[0m"
    apt-get -y remove --purge ${BUILD_DEPS[@]}
    apt-get -y autoremove

    echo
    echo -e "\e[1;32m Builder: installing boost ops deps \e[0m"
    for i in ${OPS_DEPS_BOOST[@]}; do TO_INSTALL="${TO_INSTALL} ${i}${BOOST_VER}"; done
    apt-get install -y ${TO_INSTALL[@]}

    echo
    echo -e "\e[1;32m Builder: cleaning2 \e[0m"
    apt-get -y remove --purge \
        ${TO_CLEAN[@]} \
        ${BUILD_TOOLS[@]}

    echo
    echo -e "\e[1;32m Builder: cleaning3 \e[0m"
    apt-get -y autoremove
    apt-get -y clean autoclean

    echo
    echo -e "\e[1;32m Builder: installing core ops deps \e[0m"
    apt-get -y install --no-install-recommends \
        ${OPS_DEPS_CORE[@]}

    echo
    echo -e "\e[1;32m Builder: cleaning4 \e[0m"
    rm -Rf \
        ${ROOT}/* \
        /usr/local/include/microhttpd.h \
        /usr/local/lib/libmicrohttpd.* \
        /usr/local/include/rapidjson \
        /usr/local/include/mongo \
        /usr/local/lib/libmongoclient.a
fi

echo
echo -e "\e[1;32m Builder: cleaning last \e[0m"
rm -Rf \
    /var/lib/apt/lists/* \
    /var/log/*
