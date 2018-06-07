#!/bin/bash

yum -y install epel-release
yum -y install \
  bc \
  boost-devel \
  bzip2 \
  cmake \
  gcc-c++ \
  git \
  gnutls-devel \
  jq \
  libgcrypt-devel \
  libcurl-devel \
  openssl-devel \
  libuuid-devel \
  make \
  mongodb-org \
  mongodb-org-shell \
  nc \
  pyOpenSSL \
  python \
  python-flask \
  rpm-build \
  scons \
  tar

curl -L https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz | tar xzC /opt/ \
 && cd /opt/mongo-cxx-driver-legacy-1.1.2 \
 && scons --disable-warnings-as-errors \
 && scons install --disable-warnings-as-errors --prefix=/usr/local \
 && rm -Rf /opt/mongo-cxx-driver-legacy-1.1.2

curl -L https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz | tar xzC /opt/ \
 && mv /opt/rapidjson-1.0.2/include/rapidjson/ /usr/local/include \
 && rm -Rf /opt/rapidjson-1.0.2

curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz | tar xzC /opt/ \
 && cd /opt/libmicrohttpd-0.9.48  \
 && ./configure --disable-messages --disable-postprocessor --disable-dauth  \
 && make \
 && make install \
 && rm -Rf /opt/libmicrohttpd-0.9.48

curl -L https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2 | tar xjC /opt/ \
 && cd /opt/gmock-1.5.0 \
 && ./configure \
 && make \
 && make install \
 && rm -Rf /opt/gmock-1.5.0

curl -L http://mosquitto.org/files/source/mosquitto-1.5.tar.gz | tar xzC /opt/ \
 && cd /opt/mosquitto-1.5 \
 && make \
 && make install \
 && rm -Rf /opt/mosquitto-1.5

ldconfig
