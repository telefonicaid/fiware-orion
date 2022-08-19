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

MONGO_URI="mongodb://127.0.0.1:27017"
#MONGO_URI="mongodb://mongodb1:27017,mongodb2:27017,mongodb3:27017/?replicaSet=cb_rs0"

for db in $(echo 'show dbs' | mongo $MONGO_URI | grep '^orion' | awk -F ' ' '{print $1}')
do
  echo "Processing $db db"
  # Edit next line to set the script you want
  #PYTHONIOENCODING=utf8 python check_entities_consistency.py $db 
done

