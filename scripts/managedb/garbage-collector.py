#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

from pymongo import MongoClient
from time import time
from datetime import timedelta, datetime
from sys import argv

def check_coll(collection, collection_name):
    n = 0
    for doc in collection.find():

        id = doc['_id']
        if collection_name == CSUB_COLL:
            ref = doc['reference']
            prefix = '-- ID ' + str(id) + ' (' + ref + '): '
        elif collection_name == REG_COLL:
            # Note that registration could include several entities, but we only
            # print the first one (and a '+' sign) in that case, to avoid long lines
            l_c = len (doc['contextRegistration'])
            l_e = len (doc['contextRegistration'][0]['entities'])

            entId = doc['contextRegistration'][0]['entities'][0]['id']
            if (doc['contextRegistration'][0]['entities'][0]).has_key('type'): 
                type = doc['contextRegistration'][0]['entities'][0]['type']
            else:
                type = '<no type>'

            if (l_c > 1) or (l_e > 1):
                prefix = '-- ID ' + str(id) + ' ' + entId + ' (' + type + ') [+] : '
            else:
                prefix = '-- ID ' + str(id) + ' ' + entId + ' (' + type + '): '
        else:
            prefix = '-- ID ' + str(id) + ': '
        n += 1

        try:
            expiration = int(doc['expiration'])
            interval = expiration - time()
            if (interval < 0):
                interval_str = str(timedelta(seconds=-interval))
                print prefix + 'expired by ' + interval_str
                doc['expired'] = 1
                collection.save(doc)
            else:
                # In this case, we touch the document only if have already expired: 1,
                # this would correspond to the case of an expired registration/subscription that has
                # been "reactivated" after receiving an update in duration
                if doc.has_key('expired'):
                    doc.pop('expired', None)
                    collection.save(doc)
                interval_str = str(timedelta(seconds=interval))
                print prefix + interval_str + ' left to expiration'


        except ValueError:
            print prefix + 'invalid expiration format!'

    print 'document processed: ' + str(n)

DB = 'orion'
REG_COLL   = 'registrations'
CSUB_COLL  = 'csubs'

client = MongoClient('localhost', 27017)
db = client[DB]

now = datetime.now()
print 'Current time: ' + str(now)

# The scripts uses a list of collection as argument, so a given collection is
# processed only if its name appears there

if REG_COLL in argv:
   print 'Checking collection: ' + REG_COLL
   check_coll(db[REG_COLL], REG_COLL)
if CSUB_COLL in argv:
   print 'Checking collection: ' + CSUB_COLL
   check_coll(db[CSUB_COLL], CSUB_COLL)
