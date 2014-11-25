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

# Calculating version for RPM
currentVersion=$(grep "define ORION_VERSION" src/app/contextBroker/version.h  | awk -F\" '{ print $2 }')
# We assume that currentVersion has the following format: "0.14.0-next"
baseToken=$(echo $currentVersion | awk -F '-' '{print $1}')
timeToken=$(date "+%Y%m%d%H%M%S")
newVersion=${baseToken}_${timeToken}

# Changing version
echo "changing: <$currentVersion> to <$newVersion>"
sed "s/$currentVersion/$newVersion/" src/app/contextBroker/version.h > /tmp/version.h
mv /tmp/version.h src/app/contextBroker/version.h
