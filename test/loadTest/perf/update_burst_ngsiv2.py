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
from random import randint, sample
import json
from threading import Thread
from datetime import datetime

CB_ENDPOINT = 'http://fiware-centos1.hi.inet:1026'

def send(id, payload):
    print str(payload)
    url = CB_ENDPOINT + '/v2/entities/' + id
    headers = {'content-type': 'application/json', 'accept': 'application/json'}
    r = post(url, data=json.dumps(payload), headers=headers)
    if r.status_code != 201:
        print "ERROR sending HTTP request to CB, entity: %s, status code is: %d" % (id, r.status_code)

def random_string():
    return str(randint(1,80000))

def random_entity():
    n = randint(1,50000)
    return "E%04d" % (n,)

def random_attr_list(attr_n):
    return sample(range (1, 21), attr_n)

def update_entity(attr_n):
    id = random_entity()
    attr_numbers = random_attr_list(attr_n)

    print "+ Updating entity: %s %s" % (id, str(attr_numbers))

    update_msg = {}
    for i in attr_numbers:
        attr_name =  "A%02d" % (i,)
        update_msg[attr_name] = {'type': 'T', 'value': random_string()}

    #time = datetime.now().strftime('%Y-%m-%dT%H:%M:%S+0100')
    time = datetime.now().isoformat() # miliseconds accuraci
    update_msg['TimeInstant'] = {'type': 'T', 'value': time}

    send(id, update_msg)

def update_burst(burst_size, attr_n, parallel):
    to_launch = []
    for i in range (0, burst_size):
        if parallel:
            t = Thread(target=update_entity(attr_n))
            t.daemon = True
            to_launch.append(t)
        else:
            update_entity(attr_n)
    if parallel:
        for i in to_launch:
            i.start()

update_burst(1, 2, True)

