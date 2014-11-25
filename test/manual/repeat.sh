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

noOfTimes=$1
op=$2
file=$3

if [ $# -ne 3 ]
then
  echo "usage: $0 <no of times> <operation> <file>"
  exit 1
fi

typeset -i t
t=0

while [ $t -lt $noOfTimes ]
do
  cbTest.sh $op $file > /dev/null 2>&1
  t=$t+1
  echo done test $t of $noOfTimes
done
