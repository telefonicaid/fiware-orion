#!/bin/bash
# Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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

# FIXME: default yum repositories has been changed to vault due to CentOS 8 EOL on February 1st, 2022
# This is just a temporal solution so the build doesn't break while we find a new distro to use
sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-Linux-*
sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-Linux-*
yum upgrade -y
yum -y install epel-release
yum -y install \
  bc \
  boost-devel \
  bzip2 \
  cmake \
  gcc-c++ \
  git \
  gnutls-devel \
  libgcrypt-devel \
  libcurl-devel \
  openssl-devel \
  libuuid-devel \
  make \
  valgrind \
  mongodb-org-shell \
  nc \
  python3 \
  rpm-build \
  tar \
  cyrus-sasl-devel

echo "INSTALL: python special dependencies" \
&& cd /opt \
&& pip3 install virtualenv \
&& virtualenv /opt/ft_env --python=/usr/bin/python3 \
&& . /opt/ft_env/bin/activate \
&& pip install Flask==2.0.2 \
&& pip install paho-mqtt==1.6.1 \
&& deactivate

# Recommended setting for DENABLE_AUTOMATIC_INIT_AND_CLEANUP, to be removed in 2.0.0
# see http://mongoc.org/libmongoc/current/init-cleanup.html#deprecated-feature-automatic-initialization-and-cleanup
echo "INSTALL: mongodb c driver (required by mongo c++ driver)" \
&& curl -L https://github.com/mongodb/mongo-c-driver/releases/download/1.17.4/mongo-c-driver-1.17.4.tar.gz | tar xzC /opt/ \
&& cd /opt/mongo-c-driver-1.17.4 \
&& mkdir cmake-build \
&& cd cmake-build \
&& cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. \
&& make \
&& make install

echo "INSTALL: rapidjson" \
&& curl -L https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz | tar xzC /opt/ \
&& mv /opt/rapidjson-1.1.0/include/rapidjson/ /usr/local/include

echo "INSTALL: libmicrohttpd" \
&& curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.70.tar.gz | tar xzC /opt/ \
&& cd /opt/libmicrohttpd-0.9.70  \
&& ./configure --disable-messages --disable-postprocessor --disable-dauth  \
&& make \
&& make install

echo "INSTALL: gmock" \
&& curl -L https://src.fedoraproject.org/repo/pkgs/gmock/gmock-1.5.0.tar.bz2/d738cfee341ad10ce0d7a0cc4209dd5e/gmock-1.5.0.tar.bz2 | tar xjC /opt/ \
&& cd /opt/gmock-1.5.0 \
&& ./configure \
&& make \
&& make install

echo "INSTALL: mosquitto" \
&& curl -kL http://mosquitto.org/files/source/mosquitto-2.0.12.tar.gz | tar xzC /opt/ \
&& cd /opt/mosquitto-2.0.12 \
&& sed -i 's/WITH_CJSON:=yes/WITH_CJSON:=no/g' config.mk \
&& sed -i 's/WITH_STATIC_LIBRARIES:=no/WITH_STATIC_LIBRARIES:=yes/g' config.mk \
&& sed -i 's/WITH_SHARED_LIBRARIES:=yes/WITH_SHARED_LIBRARIES:=no/g' config.mk \
&& make \
&& make install

ldconfig

yum -y remove \
&& yum -y clean all \
&& rm -rf /var/cache/yum \
&& rm -Rf /opt/mongo-c-driver-1.17.4 \
&& rm -Rf /opt/rapidjson-1.1.0 \
&& rm -Rf /opt/libmicrohttpd-0.9.70 \
&& rm -Rf /opt/mosquitto-2.0.12 \
&& rm -Rf /opt/gmock-1.5.0
