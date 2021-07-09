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

set -e

BUILD_DEPS=(
  'libcurl-devel' \
  'libgcrypt-devel' \
  'gcc-c++' \
  'zlib-devel' \
  'openssl-devel' \
  'libuuid-devel' \
  'cyrus-sasl-devel' \
  'libicu' \
  'libicu-devel' \
)

BUILD_TOOLS=(
 'bzip2' \
 'ca-certificates' \
 'python3' \
 'python2' \
 'cmake' \
 'curl' \
 'wget' \
 'dirmngr' \
 'git' \
 'make' \
 'gnutls' \
)

yum -y --nogpgcheck  install \
    ${BUILD_TOOLS[@]} \
    ${BUILD_DEPS[@]}
