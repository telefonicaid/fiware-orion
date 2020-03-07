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

echo "Builder: building started"

echo "Builder: create folders and user"
useradd -s /bin/false -r ${ORION_USER}
mkdir -p /var/{log,run}/${BROKER}

echo "Builder: update apt"
apt-get -y update

echo "Builder: installing  tools and dependencies"
apt-get -y install --no-install-recommends \
    ${BUILD_TOOLS[@]} \
    ${BUILD_DEPS[@]}

echo "Builder: installing mongo cxx driver"
git clone https://github.com/FIWARE-Ops/mongo-cxx-driver ${ROOT}/mongo-cxx-driver
cd ${ROOT}/mongo-cxx-driver
scons --disable-warnings-as-errors --use-sasl-client --ssl
scons install --disable-warnings-as-errors --prefix=/usr/local --use-sasl-client --ssl
cd ${ROOT} && rm -Rf mongo-cxx-driver

echo "Builder: installing rapid json"
curl -L https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz | tar xzC ${ROOT}
mv ${ROOT}/rapidjson-1.0.2/include/rapidjson/ /usr/local/include
cd ${ROOT} && rm -Rf rapidjson-1.0.2

echo "Builder: installing libmicronhttpd"
curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz | tar xzC ${ROOT}
cd ${ROOT}/libmicrohttpd-0.9.48
./configure --disable-messages --disable-postprocessor --disable-dauth
make
make install
cd ${ROOT} && rm -Rf libmicrohttpd-0.9.48

ldconfig

echo "Builder: installing k tools"
for kproj in kbase klog kalloc kjson khash
do
    git clone https://gitlab.com/kzangeli/${kproj}.git ${ROOT}/$kproj
done

for kproj in kbase klog kalloc kjson khash
do
    cd ${ROOT}/$kproj
    git checkout release/0.4
    make
    make install
done

if [[ "${STAGE}" == 'deps' ]]; then
    echo "Builder: installing mongo"
    apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 9DA31620334BD75D9DCB49F368818C72E52529D4

    echo 'deb [ arch=amd64 ] https://repo.mongodb.org/apt/debian stretch/mongodb-org/4.0 main' > /etc/apt/sources.list.d/mongodb.list
    apt-get -y update
    apt-get -y install \
        mongodb-org \
        mongodb-org-shell

    echo "Builder: installing gmock"
    curl -L https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2 | tar xjC ${ROOT}
    cd ${ROOT}/gmock-1.5.0
    ./configure
    make
    make install
    cd ${ROOT} && rm -Rf gmock-1.5.0

    echo "Builder: installing  tools and dependencies"
    apt-get -y install --no-install-recommends \
        ${TEST_TOOLS[@]}

    echo "Builder: installing python dependencies"
    pip install --upgrade setuptools wheel
    pip install Flask==1.0.2 pyOpenSSL==19.0.0
    yes | pip uninstall setuptools wheel
fi

if [[ ${STAGE} == 'release' ]]; then

    echo "Builder: installing orion"
    git clone ${REPOSITORY} ${PATH_TO_SRC}
    cd ${PATH_TO_SRC}
    git checkout ${REV}
    make install
    strip /usr/bin/${BROKER}

    BOOST_VER=$(apt-cache policy libboost-all-dev | grep Installed | awk '{ print $2 }' | cut -c -6)

    echo "Builder: cleaning1"
    apt-get -y remove --purge ${BUILD_DEPS[@]}
    apt-get -y autoremove

    echo "Builder: installing boost ops deps"
    for i in ${OPS_DEPS_BOOST[@]}; do TO_INSTALL="${TO_INSTALL} ${i}${BOOST_VER}"; done
    apt-get install -y ${TO_INSTALL[@]}

    echo "Builder: cleaning2"
    apt-get -y remove --purge \
        ${TO_CLEAN[@]} \
        ${BUILD_TOOLS[@]}

    echo "Builder: cleaning3"
    apt-get -y autoremove
    apt-get -y clean autoclean

    echo "Builder: installing core ops deps"
    apt-get -y install --no-install-recommends \
        ${OPS_DEPS_CORE[@]}

    echo "Builder: cleaning4"
    rm -Rf \
        ${ROOT}/* \
        /usr/local/include/microhttpd.h \
        /usr/local/lib/libmicrohttpd.* \
        /usr/local/include/rapidjson \
        /usr/local/include/mongo \
        /usr/local/lib/libmongoclient.a
fi

echo "Builder: cleaning last"
rm -Rf \
    /var/lib/apt/lists/* \
    /var/log/*
