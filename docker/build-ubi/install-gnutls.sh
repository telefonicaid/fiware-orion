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
yum makecache --refresh
yum -y install gmp

yum -y install https://rpmfind.net/linux/centos/8-stream/BaseOS/x86_64/os/Packages/pkgconf-pkg-config-1.4.2-1.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/BaseOS/x86_64/os/Packages/p11-kit-devel-0.23.22-1.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/BaseOS/x86_64/os/Packages/gmp-c++-6.1.2-10.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/BaseOS/x86_64/os/Packages/gmp-devel-6.1.2-10.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/nettle-devel-3.4.1-7.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/libidn2-devel-2.2.0-1.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/libtasn1-devel-4.13-3.el8.x86_64.rpm

yum -y install http://mirror.centos.org/centos/8-stream/BaseOS/x86_64/os/Packages/gnutls-3.6.16-4.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/gnutls-dane-3.6.16-5.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/gnutls-utils-3.6.16-5.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/gnutls-c++-3.6.16-5.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/gnutls-devel-3.6.16-5.el8.x86_64.rpm
