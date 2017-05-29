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
import json

CB_ENDPOINT = 'http://fiware-centos1.hi.inet:1026'

csub_template = {
    "entities": [
        {
            "type": "T",
            "isPattern": "true",
            "id": ".*"
        }
    ],
    "attributes": [ ],
    "reference": "",
    "duration": "P1M",
    "notifyConditions": [
        {
            "type": "ONCHANGE",
            "condValues": [
                "TimeInstant"
            ]
        }
    ]
}

def send(id, payload):
    # We are using NGSIv1 to subscribe (NGSIv2 subscription creation is not implemented in Orion at the present moment)
    url = CB_ENDPOINT + '/v1/subscribeContext'
    headers = {'content-type': 'application/json', 'accept': 'application/json'}
    r = post(url, data=json.dumps(payload), headers=headers)
    if r.status_code != 200:
        print "ERROR sending HTTP request to CB, csub #%d, status code is: %d" % (id, r.status_code)
    else:
        id = r.json()['subscribeResponse']['subscriptionId']
        print "- csub created: %s" % id

def subscribe(i, attrs, reference):

    print "+ Creating subscription: %d" % i
    csub = csub_template
    csub['attributes'] = attrs
    csub['reference']  = reference
    send(i, csub)


def do_subscriptions(n, attrs, reference):
    for i in range (1, n + 1):
        subscribe(i, attrs, reference)

#do_subscriptions(1, [], "http://fiware-centos2.hi.inet:1028/notify")
do_subscriptions(1, [], "http://123.11.1.24/notify")
#do_subscriptions(1, ['TimeInstant'], "http://A.B.207.134:80/notify")
