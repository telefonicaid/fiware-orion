#!/bin/bash

set -e
export DEBIAN_FRONTEND=noninteractive

TOKEN=$1
REV=$2
TEST=$3

HOME='/opt'

DEPS=(
  'libcurl3' \
  'libssl1.1' \
)

DEPS_BOOST=(
 'libboost-thread' \
 'libboost-filesystem' \
 'libboost-regex' \
)

DEPS_BUILD=(
 'libboost-all-dev' \
 'libcurl4-openssl-dev' \
 'libgcrypt-dev' \
 'libgnutls28-dev' \
 'libsasl2-dev' \
 'libssl-dev' \
 'uuid-dev' \
)

TOOLS_DEPS=(
 'ca-certificates' \
 'curl' \
)

TOOLS=(
 'apt-transport-https' \
 'cmake' \
 'g++' \
 'gcc' \
 'git' \
 'dirmngr' \
 'gnupg' \
 'make' \
 'scons' \
)

TOOLS_TEST=(
 'bc' \
 'python' \
 'python-flask' \
 'lcov' \
 'netcat' \
 'valgrind' \
)

TO_CLEAN=(
 'manpages' \
 'cpp-6' \
 'git-man' \
 'openssh-client' \
 'cmake-data' \
 'libc6-dev' \
 'libgcc-6-dev' \
 'linux-libc-dev' \
 'libgpg-error-dev' \
 'libedit2' \
 'openssl' \
)

echo "Building ${REV}"
cd ${HOME}

apt-get -y update
apt-get -y upgrade

apt-get -y install --no-install-recommends \
  ${TOOLS[@]} \
  ${TOOLS_DEPS[@]}

apt-get -y install --no-install-recommends \
  ${DEPS_BUILD[@]}

git clone https://github.com/FIWARE-Ops/mongo-cxx-driver ${HOME}/mongo-cxx-driver
cd ${HOME}/mongo-cxx-driver
scons --disable-warnings-as-errors --use-sasl-client --ssl
scons install --disable-warnings-as-errors --prefix=/usr/local --use-sasl-client --ssl

curl -L https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz | tar xzC ${HOME}
mv ${HOME}/rapidjson-1.0.2/include/rapidjson/ /usr/local/include

curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz | tar xzC ${HOME}
cd ${HOME}/libmicrohttpd-0.9.48
./configure --disable-messages --disable-postprocessor --disable-dauth
make
make install

ldconfig

git clone https://gitlab-ci-token:${TOKEN}@gitlab.com/kzangeli/kbase.git ${HOME}/kbase
cd ${HOME}/kbase
git checkout release/0.2
make
make install

git clone https://gitlab-ci-token:${TOKEN}@gitlab.com/kzangeli/klog.git ${HOME}/klog
cd ${HOME}/klog
git checkout release/0.2
make
make install

git clone https://gitlab-ci-token:${TOKEN}@gitlab.com/kzangeli/kalloc.git ${HOME}/klog
cd ${HOME}/kalloc
git checkout release/0.2
make
make install

git clone https://gitlab-ci-token:${TOKEN}@gitlab.com/kzangeli/kjson.git ${HOME}/kjson
cd ${HOME}/kjson
git checkout release/0.2
mkdir bin
make
make install

if [ -n "${TEST}" ]; then
  apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv 9DA31620334BD75D9DCB49F368818C72E52529D4

  echo 'deb [ arch=amd64 ] https://repo.mongodb.org/apt/debian stretch/mongodb-org/4.0 main' > /etc/apt/sources.list.d/mongodb.list
  apt-get -y update
  apt-get -y install \
    mongodb-org \
    mongodb-org-shell

#  curl -L https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2 | tar xjC ${HOME}/
#  cd ${HOME}/gmock-1.5.0
#  ./configure
#  make
#  make install
#  rm -Rf ${HOME}/gmock-1.5.0

  apt-get -y install --no-install-recommends \
    ${TOOLS_TEST[@]}
fi

git clone https://github.com/Fiware/context.Orion-LD.git ${HOME}/context.Orion-LD
cd ${HOME}/context.Orion-LD
git checkout ${REV}
make di
strip /usr/bin/orionld

BOOST_VER=$(apt-cache policy libboost-all-dev | grep Installed | awk '{ print $2 }' | cut -c -6)

apt-get -y remove --purge ${DEPS_BUILD[@]}
apt-get autoremove -y

for i in ${DEPS_BOOST[@]}; do TO_INSTALL="${TO_INSTALL} ${i}${BOOST_VER}"; done
apt-get install -y ${TO_INSTALL[@]}

apt-get -y remove --purge \
  ${TO_CLEAN[@]} \
  ${TOOLS[@]}

apt-get autoremove -y
apt-get clean autoclean

apt-get install -y --no-install-recommends \
  ${TOOLS_DEPS[@]}

rm -Rf \
  /usr/local/include/microhttpd.h \
  /usr/local/lib/libmicrohttpd.* \
  /usr/local/include/rapidjson \
  /usr/local/include/mongo \
  /usr/local/lib/libmongoclient.a

rm -Rf \
  ${HOME}/* \
  /var/log/* \
  /var/lib/apt/lists/*
