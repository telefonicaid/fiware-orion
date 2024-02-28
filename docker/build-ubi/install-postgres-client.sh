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

echo "---------- Yum utils ----------"
yum -y install yum-utils

yum -y --nogpgcheck install https://vault.centos.org/8.5.2111/PowerTools/x86_64/kickstart/Packages/perl-IO-Tty-1.12-11.el8.x86_64.rpm
yum -y --nogpgcheck install https://vault.centos.org/8.3.2011/PowerTools/x86_64/kickstart/Packages/perl-IPC-Run-0.99-1.el8.noarch.rpm
yum -y --nogpgcheck install https://vault.centos.org/8.3.2011/AppStream/x86_64/os/Packages/perl-Test-Simple-1.302135-1.el8.noarch.rpm

echo "---------- Add repo rpms ----------"
yum -y --nogpgcheck install https://download.postgresql.org/pub/repos/yum/reporpms/EL-8-x86_64/pgdg-redhat-repo-latest.noarch.rpm
yum -y --nogpgcheck install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
yum -y --nogpgcheck install postgresql12 postgresql12-contrib

yum -y install https://rpmfind.net/linux/centos/8-stream/PowerTools/x86_64/os/Packages/libdap-3.19.1-2.el8.x86_64.rpm
yum -y install https://rpmfind.net/linux/centos/8-stream/PowerTools/x86_64/os/Packages/SuperLU-5.2.0-7.el8.x86_64.rpm
yum -y --nogpgcheck install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/blas-3.8.0-8.el8.x86_64.rpm
yum -y --nogpgcheck install https://rpmfind.net/linux/centos/8-stream/AppStream/x86_64/os/Packages/lapack-3.8.0-8.el8.x86_64.rpm
yum -y --nogpgcheck install https://archives.fedoraproject.org/pub/archive/epel/8.1/Everything/x86_64/Packages/a/armadillo-9.700.2-1.el8.x86_64.rpm

# echo "---------- Adding repos ----------"
# yum update -y --nogpgcheck

echo "---------- Install libs ----------"
yum -y --nogpgcheck install hdf5 xerces-c gdal-libs


echo "---------- Install  postgres ----------"
yum -y install libpqxx-devel postgresql12-devel postgresql12-libs

echo "---------- install-postgres-client.sh is DONE ----------"
