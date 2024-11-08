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
# Author:               Dmitrii Demin
# Re-worked for Debian: Fermín Galán

# Install security updates
apt-get -y update
apt-get -y upgrade
# Install dependencies
apt-get -y install \
  curl \
  gnupg \
  python3 \
  python3-pip \
  python3-venv \
  netcat-traditional \
  bc \
  valgrind \
  cmake \
  libssl-dev \
  git \
  g++ \
  libcurl4-openssl-dev \
  libboost-dev \
  libboost-regex-dev \
  libboost-filesystem-dev \
  libboost-thread-dev \
  uuid-dev \
  libgnutls28-dev \
  libsasl2-dev \
  libgcrypt-dev

echo "INSTALL: MongoDB shell" \
&& curl -L https://www.mongodb.org/static/pgp/server-6.0.asc | apt-key add - \
&& echo "deb http://repo.mongodb.org/apt/debian buster/mongodb-org/6.0 main" | tee /etc/apt/sources.list.d/mongodb-org-6.0.list \
&& apt-get -y update \
&& apt-get -y install mongodb-mongosh

echo "INSTALL: python special dependencies" \
&& cd /opt \
&& python3 -m venv /opt/ft_env \
&& . /opt/ft_env/bin/activate \
&& pip install Flask==2.0.2 \
&& pip install Werkzeug==2.0.2 \
&& pip install paho-mqtt==1.6.1 \
&& pip install amqtt==0.11.0b1 \
&& deactivate

# Recommended setting for DENABLE_AUTOMATIC_INIT_AND_CLEANUP, to be removed in 2.0.0
# see http://mongoc.org/libmongoc/current/init-cleanup.html#deprecated-feature-automatic-initialization-and-cleanup
echo "INSTALL: mongodb c driver" \
&& curl -L https://github.com/mongodb/mongo-c-driver/releases/download/1.29.0/mongo-c-driver-1.29.0.tar.gz | tar xzC /opt/ \
&& cd /opt/mongo-c-driver-1.29.0 \
&& mkdir cmake-build \
&& cd cmake-build \
&& cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. \
&& make \
&& make install

echo "INSTALL: rapidjson" \
&& curl -L https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz | tar xzC /opt/ \
&& mv /opt/rapidjson-1.1.0/include/rapidjson/ /usr/local/include

echo "INSTALL: libmicrohttpd" \
&& curl -L https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-1.0.1.tar.gz | tar xzC /opt/ \
&& cd /opt/libmicrohttpd-1.0.1  \
&& ./configure --disable-messages --disable-postprocessor --disable-dauth  \
&& make \
&& make install

echo "INSTALL: gmock" \
&& curl -L https://src.fedoraproject.org/repo/pkgs/gmock/gmock-1.5.0.tar.bz2/d738cfee341ad10ce0d7a0cc4209dd5e/gmock-1.5.0.tar.bz2 | tar xjC /opt/ \
&& cd /opt/gmock-1.5.0 \
&& patch -p1 gtest/scripts/fuse_gtest_files.py < /opt/archive/fuse_gtest_files.py.patch \
&& ./configure \
&& make \
&& make install

echo "INSTALL: mosquitto" \
&& curl -kL https://mosquitto.org/files/source/mosquitto-2.0.20.tar.gz | tar xzC /opt/ \
&& cd /opt/mosquitto-2.0.20 \
&& sed -i 's/WITH_CJSON:=yes/WITH_CJSON:=no/g' config.mk \
&& sed -i 's/WITH_STATIC_LIBRARIES:=no/WITH_STATIC_LIBRARIES:=yes/g' config.mk \
&& sed -i 's/WITH_SHARED_LIBRARIES:=yes/WITH_SHARED_LIBRARIES:=no/g' config.mk \
&& make \
&& make install

ldconfig

apt-get -y clean \
&& rm -Rf /opt/mongo-c-driver-1.29.0 \
&& rm -Rf /opt/rapidjson-1.1.0 \
&& rm -Rf /opt/libmicrohttpd-1.0.1 \
&& rm -Rf /opt/mosquitto-2.0.20 \
&& rm -Rf /opt/gmock-1.5.0
