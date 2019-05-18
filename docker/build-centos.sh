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
 'cmake' \
 'gcc-c++' \
 'git' \
 'make' \
 'scons' \
)

TEST_TOOLS=(
 'bc' \
 'nano' \
 'nc' \
 'mongodb-org' \
 'mongodb-org-shell' \
 'python-pip'
 'valgrind' \
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

function _usage()
{
  echo -n "Usage: build-debian.sh [options]
  Options:
    -h   --help          show help
    -t   --token         specify token to clone k* tools
    -s   --stage         specify stage (release/deps) - default release
"
}

[[ $# = 0 ]] && _usage && exit 1

reset=true

for arg in "$@"
do
    if [[ -n "$reset" ]]; then
      unset reset
      set --
    fi
    case "$arg" in
       --help) set -- "$@" -h ;;
       --token) set -- "$@" -t ;;
       --stage) set -- "$@" -s;;
       *) set -- "$@" "$arg" ;;
    esac
done

while getopts "ht:s:" opt; do
    case ${opt} in
        h)  _usage; exit 0 ;;
        t)  TOKEN=$OPTARG ;;
        s)  STAGE=$OPTARG ;;
        *) _usage && exit 1;;
        :)
        echo "option -$OPTARG requires an argument"
        _usage; exit 1
        ;;
    esac
done
shift $((OPTIND-1))

if [[ -z "${STAGE}" ]]; then STAGE='release'; fi
if [[ -z "${TOKEN}" ]]; then
    echo "Builder: TOKEN not provided";
    exit 1
fi

if [[ -z "${ROOT}" || -z "${REPOSITORY_SRC}" || -z "${BRANCH_SRC}" || -z "${BROKER}" ]]; then
    echo "Builder: ROOT or REPOSITORY_SRC or BRANCH_SRC or BROKER are not set";
    exit 1
fi

echo "Builder: adding epel"
yum -y install epel-release

echo "Builder: upgrading image"
yum -y upgrade

echo "Builder: installing  tools and dependencies"
yum -y install \
  ${BUILD_TOOLS[@]} \

yum -y install \
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
for kproj in kbase klog kalloc kjson
do
    git clone https://gitlab-ci-token:${TOKEN}@gitlab.com/kzangeli/${kproj}.git ${ROOT}/$kproj
done

for kproj in kbase klog kalloc kjson
do
    cd ${ROOT}/$kproj
    git checkout release/0.2
    make
    make install
done

if [[ "${STAGE}" == 'deps' ]]; then
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
    pip install --upgrade pip && pip install Flask==1.0.2 pyOpenSSL==19.0.0

    echo "Builder: urlencode"
    curl -O https://raw.githubusercontent.com/rpmsphere/x86_64/master/u/urlencode-1.0.3-2.1.x86_64.rpm
    rpm -U urlencode-1.0.3-2.1.x86_64.rpm
    rm -f urlencode-1.0.3-2.1.x86_64.rpm
fi

if [[ ${STAGE} == 'release' ]]; then
    echo "Builder: installing orion"

    git clone ${REPOSITORY_SRC} ${ROOT}/orion
    cd ${ROOT}/orion
    make install
    strip /usr/bin/${BROKER}

    echo "Builder: cleaning1"
    yum -y erase  \
        ${BUILD_DEPS[@]} \
        ${BUILD_TOOLS[@]} \
        ${TO_CLEAN[@]}

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
    /var/lib/yum/history
rpm -vv --rebuilddb

echo "Builder: cleaning last"
rm -Rf \
    /usr/share/cracklib \
    /usr/share/i18n \
    /usr/lib/gconv \
    /usr/lib64/gconv \
    /anaconda-post.log \
    /var/lib/apt/lists/* \
    /var/log/*
