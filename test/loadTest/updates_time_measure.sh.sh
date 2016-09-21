#!/bin/bash
# -*- coding: latin-1 -*-
# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#
# Author: Ivan Arias Leon

# variables
HOST="localhost"
PORT="1026"
NGSI_VERSION="v1"
UPDATE_TOTAL=100
SUBSC=false
SUBSC_REFERENCE="http://localhost:1028/notify"

function random_string(){
    # a random string is generated with a given size
    cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w ${1:-32} | head -n 1
}

function usage(){
     # show the script usage
     echo " "
     echo "Usage: "$(basename $0) 
     echo "   -u (usage)"
     echo "   -subsc (create a subscription)"
     echo "   --host <CB host>"
     echo "   --version <NGSI version>"
     echo "   --updates <total of updates>"
     echo "   --subsc_reference <listener url>"
     echo " "
     echo " Example:"
     echo "     "$(basename $0)" -host localhost --version v2 --updates 10000 --subsc_reference http://localhost:1028/notify"
     echo " "
     echo " Note: "
     echo "     - if the subscription is created, the entities are not updated and viceversa..."
     echo "     - if \"--subsc_reference\" parameter is used is not necessary the \"-subsc\" parameter ..."
     echo " "
}

# BEGIN 
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];                 then usage;
  elif [ "$1" == "--host" ];             then HOST="$2"; shift;
  elif [ "$1" == "--version" ];          then NGSI_VERSION="$2"; shift;
  elif [ "$1" == "--updates" ];          then UPDATE_TOTAL="$2"; shift;
  elif [ "$1" == "-subsc" ];             then SUBSC="true"; shift;
  elif [ "$1" == "--subsc_reference" ];  then SUBSC_REFERENCE="$2"; SUBSC="true"; shift;
  else
      echo $0: bad parameter/option: "'"${1}"'";
      usage
      exit
  fi
  shift
done

if [ "$SUBSC" == "true" ]; then
   curl "http://$HOST:$PORT/v2/subscriptions" -H "Fiware-service: test" -H "Fiware-ServicePath: /" -H "Content-Type: application/json" -d '{"subject": {"entities": [{"type": "Room","idPattern": ".*"}], "condition": {"attrs": ["temperature"]}}, "notification": {"http": {"url": "'$SUBSC_REFERENCE'"}, "attrs": ["temperature"]}}' &> /dev/null
   echo "...a subscription has been created..."
else
    echo "...Executing "$UPDATE_TOTAL "updates using NGSI "$NGSI_VERSION "in the host "$HOST "..."
    INIT_DATE=$(date +%s)
    for (( c=1; c<=$UPDATE_TOTAL; c++ ))
       do
           VALUE=$(random_string 8)
           if [ "$NGSI_VERSION" == "v1" ]; then
              curl  "http://$HOST:$PORT/v1/updateContext" -H "Fiware-service: test" -H "Fiware-ServicePath: /" -H "Content-Type: application/json" -d '{"contextElements": [{"type": "Room", "isPattern": "false", "id": " Room1", "attributes": [{"name": "temperature", "value": "'$VALUE'"}]}], "updateAction": "APPEND"}' &> /dev/null
           else
              curl "http://$HOST:$PORT/v2/op/update" -H "Fiware-service: test" -H "Fiware-ServicePath: /" -H "Content-Type: application/json" -d '{"entities": [{"type": "Room", "id": "Room1", "temperature": {"value": "'$VALUE'"}}], "actionType": "APPEND"}' &> /dev/null
           fi
        done
    END_DATE=$(date +%s)
    echo "the test using NGSI "$NGSI_VERSION " has taken" $(( END_DATE - INIT_DATE )) "secs..."
fi
