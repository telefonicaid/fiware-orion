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
  cmake3 \
  gcc-c++ \
  git \
  gnutls-devel \
  libgcrypt-devel \
  libcurl-devel \
  openssl-devel \
  libuuid-devel \
  make \
  mongodb-org-shell \
  nc \
  python \
  python-pip \
  rpm-build \
  scons \
  tar \
  cyrus-sasl-devel

# FIXME: remove legacy driver when it gets discontinued in the code
echo "INSTALL: mongodb legacy c++ driver" \
&& curl -L https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz | tar xzC /opt/ \
&& cd /opt/mongo-cxx-driver-legacy-1.1.2 \
&& scons --disable-warnings-as-errors --use-sasl-client --ssl \
&& scons install --disable-warnings-as-errors --prefix=/usr/local --use-sasl-client --ssl \
&& rm -Rf /opt/mongo-cxx-driver-legacy-1.1.2

# Recommended setting for DENABLE_AUTOMATIC_INIT_AND_CLEANUP, to be removed in 2.0.0
# see http://mongoc.org/libmongoc/current/init-cleanup.html#deprecated-feature-automatic-initialization-and-cleanup
echo "INSTALL: mongodb c driver (required by mongo c++ driver)" \
&& curl -L https://github.com/mongodb/mongo-c-driver/releases/download/1.16.0/mongo-c-driver-1.16.0.tar.gz | tar xzC /opt/ \
&& cd /opt/mongo-c-driver-1.16.0 \
&& mkdir cmake-build \
&& cd cmake-build \
&& cmake3 -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. \
&& make \
&& make install \
&& rm -Rf /opt/mongo-c-driver-1.16.0

echo "INSTALL: rapidjson" \
&& curl -L https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz | tar xzC /opt/ \
&& mv /opt/rapidjson-1.1.0/include/rapidjson/ /usr/local/include \
&& rm -Rf /opt/rapidjson-1.1.0

echo "INSTALL: libmicrohttpd" \
&& curl -L http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.70.tar.gz | tar xzC /opt/ \
&& cd /opt/libmicrohttpd-0.9.70  \
&& ./configure --disable-messages --disable-postprocessor --disable-dauth  \
&& make \
&& make install \
&& rm -Rf /opt/libmicrohttpd-0.9.70

echo "INSTALL: gmock" \
&& curl -L https://src.fedoraproject.org/repo/pkgs/gmock/gmock-1.5.0.tar.bz2/d738cfee341ad10ce0d7a0cc4209dd5e/gmock-1.5.0.tar.bz2 | tar xjC /opt/ \
&& cd /opt/gmock-1.5.0 \
&& ./configure \
&& make \
&& make install \
&& rm -Rf /opt/gmock-1.5.0

# FIXME: the MQTT notification work is yet ongoing, so this is not needed yet. It should be aligned
# which the same procedure described in "Build from source" documentation
#  echo "INSTALL: mosquitto" \
#  && curl -L http://mosquitto.org/files/source/mosquitto-1.5.tar.gz | tar xzC /opt/ \
#  && cd /opt/mosquitto-1.5 \
#  && make \
#  && make install \
#  && rm -Rf /opt/mosquitto-1.5

# CentOS 7 installs Flask==0.10.1 which depends on Werkzeug==0.9.1. There is a bug
# in Werkzeug which makes an empty content-length header appear in the accumulator-server.py
# dumps. The bug is fixed in Werkzeug==0.11.16. Thus, we override the system setting,
# installing in the virtual env Flask==1.0.2, which depends on Werkzeug==0.15.2
#
# In addition, note we upgrade pip before installing virtualenv. The virtualenv installation
# may fail otherwise. Note that due to Python 2.7 End-of-Life we have to add "pip < 21.0"
# (see https://stackoverflow.com/questions/65896334/python-pip-broken-wiith-sys-stderr-writeferror-exc)
echo "INSTALL: python special dependencies" \
&& cd /opt \
&& pip install --upgrade "pip < 21.0" \
&& pip install virtualenv \
&& virtualenv /opt/ft_env \
&& . /opt/ft_env/bin/activate \
&& pip install Flask==1.0.2 \
&& pip install pyOpenSSL==19.0.0 \
&& deactivate

ldconfig

yum -y remove \
  scons \
&& yum -y clean all \
&& rm -rf /var/cache/yum
