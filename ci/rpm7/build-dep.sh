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

#curl -L http://mosquitto.org/files/source/mosquitto-1.5.tar.gz | tar xzC /opt/ \
#  && cd /opt/mosquitto-1.5 \
#  && make \
#  && make install \
#  && rm -Rf /opt/mosquitto-1.5

ldconfig

yum -y remove \
  scons \
&& yum -y clean all \
&& rm -rf /var/cache/yum
