#!/bin/bash

# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
# author: 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

if [  "$1" == ""  ]
  then
    echo "ERROR - No host supplied (Mandatory)"
    echo "usage:"
    echo "    ./drop_database_mongo.sh <host> <prefix> "
    echo "    example: ./drop_database_mongo.sh localhost mydb_ "
    exit
  else
    host=$1
fi

if [  "$2" != ""  ]
  then
    prefix=$2
  else
    prefix="orion"
fi




dbs=(`(echo 'show databases' | mongo --host $host) | grep $prefix`)
for db in ${dbs[@]};
    do
      if [[ $db =~ .*$prefix.* ]]
         then
           (echo 'db.dropDatabase()' | mongo $db)
      fi
done


