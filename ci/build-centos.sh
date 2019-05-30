#!/bin/bash

# Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
# Author: Dmitrii Demin <mail@demin.co>
#

set -e

OPS_DEPS_BOOST=(
 'boost-filesystem' \
 'boost-regex' \
 'boost-thread' \
)

BUILD_DEPS=(
 'boost-devel' \
 'cyrus-sasl-devel' \
 'gnutls-devel' \
 'libcurl-devel' \
 'libgcrypt-devel' \
 'libuuid-devel' \
 'openssl-devel' \
)

BUILD_TOOLS=(
 'bzip2' \
 'cmake' \
 'gcc-c++' \
 'git' \
 'make' \
 'rpm-build' \
 'scons' \
)

TEST_TOOLS=(
 'bc' \
 'nc' \
 'python-pip' \
)

TO_CLEAN=(
  '*-devel' \
  '*-headers' \
  'boost-*' \
  'cpp' \
  'dbus-glib' \
  'fipscheck*' \
  'libarchive' \
  'libedit' \
  'libgomp' \
  'mpfr' \
  'openssh*' \
  'perl* \'
  'rsync' \
  'sysvinit-tools' \
)

echo "Builder: building started"

echo "Builder: create folders and user"
useradd -s /bin/false -r ${ORION_USER}
mkdir -p /var/{log,run}/${BROKER}

echo "Builder: adding epel"
yum -y install epel-release

echo "Builder: installing  tools and dependencies"
yum -y install \
  ${BUILD_TOOLS[@]} \
  ${BUILD_DEPS[@]}

echo "Builder: installing mongo cxx driver"
git clone https://github.com/FIWARE-Ops/mongo-cxx-driver ${ROOT}/mongo-cxx-driver
cd ${ROOT}/mongo-cxx-driver
scons --disable-warnings-as-errors --use-sasl-client --ssl
scons install --disable-warnings-as-errors --prefix=/usr/local --use-sasl-client --ssl
cd ${ROOT} && rm -Rf mongo-cxx-driver

echo "Builder: installing rapidjson"
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

if [[ "${STAGE}" == 'deps' ]]; then
    echo "Builder: installing mongo"
    echo -n '[mongodb-org-4.0]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/$releasever/mongodb-org/4.0/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://www.mongodb.org/static/pgp/server-4.0.asc
' > /etc/yum.repos.d/mongodb-org-4.0.repo

    yum -y install \
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
    yum -y install \
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

    echo "Builder: cleaning1"
    yum -y remove  \
        ${BUILD_DEPS[@]} \
        ${BUILD_TOOLS[@]} \
        ${TO_CLEAN[@]} \
        epel-release

    echo "Builder: installing boost ops deps"
    yum -y install \
        ${OPS_DEPS_BOOST[@]}

    echo "Builder: cleaning2"
    rm -Rf \
        ${ROOT}/* \
        /usr/local/include/microhttpd.h \
        /usr/local/lib/libmicrohttpd.* \
        /usr/local/include/rapidjson \
        /usr/local/include/mongo \
        /usr/local/lib/libmongoclient.a
fi

echo "Builder: cleaning locales"

find /usr/share/locale -mindepth 1 -maxdepth 1 ! -name 'en_US' ! -name 'locale.alias' | xargs -r rm -r
localedef --list-archive | grep -v -e "en_US" | xargs localedef --delete-from-archive
/bin/cp -f /usr/lib/locale/locale-archive /usr/lib/locale/locale-archive.tmpl && build-locale-archive

echo "Builder: cleaning rpm db and yum cache"
rpm -qa groff | xargs -r rpm -e --nodeps
yum clean all
rm -Rf \
    /var/lib/yum/yumdb \
    /var/lib/yum/history \
    /var/cache/yum/
rpm -vv --rebuilddb

echo "Builder: cleaning last"
rm -Rf \
    /usr/share/cracklib \
    /usr/share/i18n \
    /usr/lib/gconv \
    /usr/lib64/gconv \
    /anaconda-post.log \
    /usr/lib/udev/hwdb.d/* \
    /etc/udev/hwdb.bin \
    /var/log/*
