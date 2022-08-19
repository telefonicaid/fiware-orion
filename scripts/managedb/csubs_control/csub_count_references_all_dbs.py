#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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

from pymongo import MongoClient


def add_endpoint(endpoint):
    """
    Add endpoint to map
    :param endpoint:
    :return:
    """

    global endpoints

    if endpoint in endpoints:
        endpoints[endpoint] += 1
    else:
        endpoints[endpoint] = 1


def process_db(db):
    """
    Orion database to process
    :param db:
    :return:
    """

    global total_subs
    global wrong_protocol
    global wrong_reference

    for sub in client[db].csubs.find():
        total_subs += 1

        if not 'reference' in sub:
            wrong_reference =+ 1
            return

        url = sub['reference']

        if not (url.startswith('http://') or url.startswith('https://')):
            wrong_protocol = + 1
            return

        # Endpoint: the string between "http(s):// and next /
        endpoint = url.split('//')[1].split('/')[0]
        add_endpoint(endpoint)


endpoints = {}
wrong_protocol = 0
wrong_reference = 0
total_subs = 0

client = MongoClient('localhost', 27017)

for db in client.database_names():
    if db.startswith('orion'):
        process_db(db)

total_endpoints = 0
total_distinct_endpoints = 0
for endpoint in sorted(endpoints, key=endpoints.__getitem__, reverse=True):
    print '* %s: %d' % (endpoint, endpoints[endpoint])
    total_endpoints += endpoints[endpoint]
    total_distinct_endpoints += 1

print '-----'
print '* Total csubs: %s ' % total_subs
print '* csubs not using https or https protocol: %d' % wrong_protocol
print '* csubs not using reference (corrupt): %d' % wrong_reference
print '* ssubs using endpoints: %d' % total_endpoints
print '* Total distinct endpoints: %d' % total_distinct_endpoints