#!/bin/bash

# Copyright 2022 Telefonica Investigacion y Desarrollo, S.A.U
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

curl -L -v -O ftp://ftp.pbone.net/mirror/download.fedora.redhat.com/pub/fedora/epel/7/x86_64/Packages/s/SuperLU-5.2.0-5.el7.x86_64.rpm
if [ -f SuperLU-5.2.0-5.el7.x86_64.rpm ]
then
    yum localinstall -y SuperLU-5.2.0-5.el7.x86_64.rpm
else
    echo Error downloading SuperLU-5.2.0-5.el7.x86_64.rpm
fi


curl -v -L -O https://kojipkgs.fedoraproject.org//packages/arpack/3.8.0/4.fc36/src/arpack-3.8.0-4.fc36.src.rpm
if [ -f arpack-3.8.0-4.fc36.src.rpm ]
then
    yum localinstall -y arpack-3.8.0-4.fc36.src.rpm
else
    echo Error downloading arpack-3.8.0-4.fc36.src.rpm
fi


curl -v -L -O http://rpmfind.net/linux/opensuse/tumbleweed/repo/oss/x86_64/libhdf5-103-32bit-1.10.8-5.2.x86_64.rpm
if [ -f libhdf5-103-32bit-1.10.8-5.2.x86_64.rpm ]
then
    yum localinstall -y libhdf5-103-32bit-1.10.8-5.2.x86_64.rpm
else
    echo Error downloading libhdf5-103-32bit-1.10.8-5.2.x86_64.rpm
fi


curl -v -L -O http://rpmfind.net/linux/fedora/linux/releases/36/Everything/x86_64/os/Packages/l/lapack-3.10.0-5.fc36.i686.rpm
if [ -f lapack-3.10.0-5.fc36.i686.rpm ]
then
    yum localinstall -y lapack-3.10.0-5.fc36.i686.rpm
else
    echo Error downloading lapack-3.10.0-5.fc36.i686.rpm
fi


curl -v -L -O http://yum.stanford.edu/mrepo/epel-EL8-x86_64/RPMS.all/armadillo-9.700.2-1.el8.x86_64.rpm
if [ -f armadillo-9.700.2-1.el8.x86_64.rpm ]
then
    yum localinstall -y armadillo-9.700.2-1.el8.x86_64.rpm
else
    echo Error downloading armadillo-9.700.2-1.el8.x86_64.rpm
fi
