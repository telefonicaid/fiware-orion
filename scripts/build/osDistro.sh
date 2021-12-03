# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

ubuntu_distro=$(grep DISTRIB_DESCRIPTION= /etc/lsb-release 2>/dev/null | cut -d\" -f 2 | sed 's/ /_/g')

debian_distro=$(cat /etc/debian_version 2> /dev/null)

suse_distro=$(cat /etc/SuSE-release 2> /dev/null | grep SUSE | cut -d ' ' -f 1-2 | sed 's/ /_/g')

centos_distro=$(cat /etc/redhat-release 2> /dev/null | awk '{print $3}')

# In some cases (e.g. CentOS 7.x) we have found that the /etc/redhat-release content uses the following pattern:
#
#  CentOS Linux release 7.2.1511 (Core)
#
# Instead the one in previous versions:
#
#  CentOS release 6.6 (Final)
#
# The check in the next line ensures that the distro version is correctly detected in this case.
if [ "$centos_distro" = "release" ]
then
	centos_distro=$(cat /etc/redhat-release 2> /dev/null | awk '{print $4}')
fi

# In some other cases we have found that the /etc/redhat-release content uses the following pattern:
#
#  Red Hat Enterprise Linux Server release 6.7 (Santiago)
#  Red Hat Enterprise Linux release 8.4 (Ootpa)
#
# The check in the next line ensures that the distro version is correctly detected in this case.
if [ "$centos_distro" = "Enterprise" ]
then
	redhat_distro=$(cat /etc/redhat-release 2> /dev/null | awk '{print $6}')
  unset centos_distro
fi

fedora_distro=$(cat /etc/redhat-release 2> /dev/null | awk '{print $1}')

distro="Unknown_1.0"

if [ "$ubuntu_distro" != "" ]
then
  distro=$ubuntu_distro
elif [ "$debian_distro" != "" ]
then
  distro="Debian_$debian_distro"
elif [ "$suse_distro" != "" ]
then
  distro=$suse_distro 
elif [ "$fedora_distro" = "Fedora" ]
then
  distro=${fedora_distro}_${centos_distro}
elif [ "$centos_distro" != "" ]
then
  distro=CentOS_$centos_distro 
elif [ "$redhat_distro" != "" ]
then
  distro=RedHat_$redhat_distro
fi


echo -n $distro
