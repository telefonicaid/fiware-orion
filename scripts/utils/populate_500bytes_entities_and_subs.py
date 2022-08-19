#!/usr/bin/env python
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

# This script creates a set of entities and a set of subscriptions (hardwired number
# of 1500 but it can be easily changed). Each entity and subscription is designed
# to have a 500 bytes length when rendered in NGSIv2.

from requests import post
import json


url_entities = 'http://localhost:1026/v2/entities'
url_subscription = 'http://localhost:1026/v2/subscriptions'
headers = {'content-type': 'application/json'}

# This payload is calculated to have exactly 500 bytes when rendered with GET /v2/entities/ID operation
payload_entities = {
  "id": "0000000000",         
  "type": "T",
  "Attribute1": {
    "type": "string",
    "value": "Lorem ipsum dolor sit amet, r adipiscing t, sed eiusmod tor incidunt ut labore et dolore magna aliqua"
  },
  "Attribute2": {
    "type": "string",
    "value": "Lorem ipsum dolor sit amet, r adipiscing t, sed eiusmod tor incidunt ut labore et dolore magna aliqua"
  },
  "Attribute3": {
    "type": "string",
    "value": "Lorem ipsum dolor sit amet, r adipiscing t, sed eiusmod tr incidunt ut labore et dolore magna aliqua"
  }
}

# This payload is calculated to have exactly 500 bytes when rendered with GET /v2/subscriptions/ID operation
payload_sub = {
  "description": "A subscription to get info about Room1",
  "subject": {
    "entities": [
      {
        "id": "Room1",
        "type": "Room"
      },
	  {
        "id": "Room2",
        "type": "Room"
      },
	  {
        "id": "Room3",
        "type": "Room"
      },
	  {
        "id": "Room4",
        "type": "Room"
      }
    ],
    "condition": {
      "attrs": [
        "presre", "temperat", "humity"
      ]
    }
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/accumulate/papath3/path4"
    },
    "attrs": [
      "presre", "teraturexx", "humity"
    ]
  },
  "expires": "2040-01-01T14:00:00.00Z",
  "throttling": 5
}

for i in range(0, 1500):
    print '* Creating entity ' + str(i)
    payload_entities['id'] = str(i).zfill(10)
    r = post(url_entities, data=json.dumps(payload_entities), headers=headers)
    #print r.text
	
for i in range(0, 1500):
    print '* Creating subscription ' + str(i)
    r = post(url_subscription, data=json.dumps(payload_sub), headers=headers)
    #print r.text
