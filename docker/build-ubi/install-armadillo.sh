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


#curl -v -L -O http://yum.stanford.edu/mrepo/epel-EL8-x86_64/RPMS.all/armadillo-9.700.2-1.el8.x86_64.rpm
#if [ -f armadillo-9.700.2-1.el8.x86_64.rpm ]
#then
    # yum install -y arpack
    # yum install -y hdf5-devel
    # yum install -y lapack-devel
    # yum install -y SuperLU
    # yum localinstall armadillo-9.700.2-1.el8.x86_64.rpm
    yum makecache --refresh
    yum install -y armadillo
#else
#    echo Error downloading armadillo-9.700.2-1.el8.x86_64.rpm
#fi
