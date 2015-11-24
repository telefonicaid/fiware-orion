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

# Summary:
# 1 -> base csub
# 2 -> changing 'attributes' order
# 3 -> channing 'notifyConditions' order
# 4 -> changing ONCHANGE cond value order
# 5 -> all changes all the same time

# Run script with:
#
# ./csub_dups_checker.py --db orion --dry-run -v
# 
# - it should show that all them correspond to the same subscription: 
# - it should discard all except number 4 (the one with longer expiration):

HOST=localhost
PORT=1026

# Sub 1:
(curl $HOST:$PORT/v1/subscribeContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature", "pressure"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "pressure", "humidity"
            ]
        },
        {
            "type": "ONTIMEINTERVAL",
            "condValues": [
                "PT10S"
            ]
        }		
    ],
    "throttling": "PT5S"
}
EOF
sleep 0.5s

# Sub 2:
(curl $HOST:$PORT/v1/subscribeContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "pressure", "temperature"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "pressure", "humidity"
            ]
        },
        {
            "type": "ONTIMEINTERVAL",
            "condValues": [
                "PT10S"
            ]
        }		
    ],
    "throttling": "PT5S"
}
EOF
sleep 0.5s

# Sub 3:
(curl $HOST:$PORT/v1/subscribeContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature", "pressure"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONTIMEINTERVAL",
            "condValues": [
                "PT10S"
            ]
        },
		{
            "type": "ONCHANGE",
            "condValues": [
                "pressure", "humidity"
            ]
        }
    ],
    "throttling": "PT5S"
}
EOF
sleep 0.5s

# Sub 4:
(curl $HOST:$PORT/v1/subscribeContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "temperature", "pressure"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P5M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "humidity", "pressure"
            ]
        },
        {
            "type": "ONTIMEINTERVAL",
            "condValues": [
                "PT10S"
            ]
        }		
    ],
    "throttling": "PT5S"
}
EOF
sleep 0.5s

# Sub 5:
(curl $HOST:$PORT/v1/subscribeContext -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "entities": [
        {
            "type": "Room",
            "isPattern": "false",
            "id": "Room1"
        }
    ],
    "attributes": [
        "pressure", "temperature"
    ],
    "reference": "http://localhost:1028/accumulate",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONTIMEINTERVAL",
            "condValues": [
                "PT10S"
            ]
        },
        {
            "type": "ONCHANGE",
            "condValues": [
                "humidity", "pressure"
            ]
        }
    ],
    "throttling": "PT5S"
}
EOF
