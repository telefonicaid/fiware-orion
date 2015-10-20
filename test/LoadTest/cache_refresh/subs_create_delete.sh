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

service=entities_append_long_time
service_path=/test
listen_host=localhost
listen_port=8090


if [  "$1" == ""  ]
  then
    echo "ERROR - No host defined (Mandatory)"
    echo "usage:"
    echo "    ./subs_create_delete.sh <host> [<subcriptionId>]"
    echo "    example: ./subs_create_delete.sh localhost "
    echo "    example: ./subs_create_delete.sh localhost 51c04a21d714fb3b37d7d5a7 "
    exit
fi

if [ "$2" == "" ]
   then
curl $1:1026/NGSI10/subscribeContext -v -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' --header "Fiware-Service: $service" --header "Fiware-ServicePath: $service_path" -d @- <<EOF
{
  "entities": [
   {
	 "type": "house",
	 "isPattern": "true",
	 "id": "room_*"
   }
  ],
  "attributes": [
    "temperature"
  ],
  "reference": "http://$listen_host:$listen_port/notify",
  "duration": "PT10000S",
  "notifyConditions": [
	 {
       "type": "ONCHANGE",
       "condValues": [
          "temperature"
       ]
	 }
  ]
}
EOF
   else
curl $1:1026/v1/unsubscribeContext -v -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' --header "Fiware-Service: $service" --header "Fiware-ServicePath: $service_path" -d @- <<EOF
{
    "subscriptionId": "$2"
}
EOF
fi
