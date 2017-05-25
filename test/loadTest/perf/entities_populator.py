#!/usr/bin/python
# -*- coding: latin-1 -*-
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

__author__ = 'fermin'

from requests import post
from random import randint
import json
from threading import Thread

#CB_ENDPOINT = 'http://fiware-centos1.hi.inet:1026'
CB_ENDPOINT = 'http://localhost:1026'

entity_template = {
    "contextElements": [
        {
            "type": "T",
            "isPattern": "false",
            "id": "E",
            "attributes": [ ]
        }
    ],
    "updateAction": "APPEND"
}

def send(id, payload):
    # We are using NGSIv1 to create entities (although the update test could be using NGSIv2)
    url = CB_ENDPOINT + '/v1/updateContext'
    headers = {'content-type': 'application/json', 'accept': 'application/json'}
    r = post(url, data=json.dumps(payload), headers=headers)
    if r.status_code != 200:
        print "ERROR sending HTTP request to CB, entity: %s, status code is: %d" % (id, r.status_code)

def random_string():
    return str(randint(400,800))

def create_entity(id, attrs_n):

    print "+ Creating entity: %s" % id
    attrs = []
    attrs.append({'name': 'TimeInstant', 'type': 'T', 'value': random_string()})
    for i in range(0, attrs_n):
        name = "A%02d" % (i,)
        attrs.append({'name': name, 'type': 'T', 'value': random_string()})

    entity = entity_template
    entity['contextElements'][0]['id'] = id
    entity['contextElements'][0]['attributes'] = attrs

    send(id, entity)

def create_entity_range(n_0, n_f, attrs_n):
    for i in range (n_0, n_f + 1):
        id = "E%05d" % (i,)
        t = Thread(target=create_entity(id, attrs_n))
        t.daemon = True
        t.start()

create_entity_range(1, 50000, 20)
