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

from pymongo import MongoClient
import sys

if len(sys.argv) != 2:
    print "missing db name"
    sys.exit()

seen = {}
dups = {}

DB = sys.argv[1]
COL = 'entities'

need_fix = False
verbose = False

client = MongoClient('localhost', 27017)
db = client[DB]

def msg(s):
    if verbose:
        print s

def in_seen(id, type, attrs_n):
    if (not id in seen):
        return False
    else:
        if type in seen[id]:
            # This is a sanity check. If attribute number differ, we want to know 
            if (seen[id][type] != attrs_n):
                msg('* different attrs count for duplicated entity <%s, %s> stored=%d, compared=%d' % (id, type, seen[id][type], attrs_n))
            return True
        else:
            return False

def add_seen(id, type, attrs_n):
    if (not id in seen):
       seen[id] =  {}
    seen[id][type] = attrs_n 

def in_dups(id, type):
    if (not id in dups):
        return False
    else:
        return type in dups[id]
 
def add_dups(id, type):
    if (not id in dups):
       dups[id] =  {}
    dups[id][type] = 1 

def null_service_path():
    return db[COL].find({"_id.servicePath": None}).count()

def get_sp_dups():
    n = 0
    skipped = 0
    sane = 0
    dups = 0

    print "- processing entities collection looking for duplicates, this may take a while... "

    for doc in db[COL].find():
        n += 1
        if ('servicePath' in doc['_id'] and doc['_id']['servicePath'] != "/"):
            # Ignoring entities with a not default servicePath, such as '/A' or '/A/A1'
            skipped += 1
            continue

        id = doc['_id']['id']
        if ('type' in doc['_id']):
            type = doc['_id']['type']
        else:
            type = ''
        attrs_n = len(doc['attrs'])

        if (in_seen(id, type, attrs_n)):
            msg('* default service path anomany found with entity <%s, %s>' % (id, type))
            add_dups(id, type)
            dups += 1
        else:
            add_seen(id, type, attrs_n)
            sane += 1

    print '- duplicated default service path detection:'
    print '    total entities:                                                                  %d' % n
    print '    entities with service path not null and different that "/" (it is ok):           %d' % skipped 
    print '    sane cases (entities with "/" service path or null without duplicated):          %d' % (sane - dups) 
    print '    duplicated cases of entities with "/" and null "/" service path (NEED FIX):      %d' % (2*dups)

    if (dups > 0):
        global need_fix
        need_fix = True

def fix_null_sps():
    hits = 0
    errors = 0

    print "- processing entities collection fixing null service paths, this may take a while... "

    for doc in db[COL].find():
        id = doc['_id']['id']
        if ('type' in doc['_id']):
            type = doc['_id']['type']
        else:
            type = ''
        attrs_n = len(doc['attrs'])

        # Anomalies or entities already with servicePath are skipeed
        if (in_dups(id, type) or 'servicePath' in doc['_id']):
            continue
        
        hits += 1

        try:
            doc['_id']['servicePath'] = '/'
            db[COL].insert(doc)
            if (type == ''):
                db[COL].remove({'_id.id': id, '_id.type': None, '_id.servicePath': None})
            else:
                db[COL].remove({'_id.id': id, '_id.type': type, '_id.servicePath': None})
        except:
            errors += 1
            print "* Error editing entity at DB:", sys.exc_info()[:2]

    print '- hits: %d, changes: %d, errors %d' % (hits, hits - errors, errors)
    if (errors > 0):
        global need_fix
        need_fix = True

# Warn user
print "WARNING!!!! This script modifies your '%s' database. It is STRONGLY RECOMMENDED that you" % DB
print "do a backup of your database before using it as described in https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide#Backup. Use this script at your own risk."
print "If you are sure you want to continue type 'yes' and press Enter"

confirm = raw_input()

if (confirm != 'yes'):
    sys.exit()

get_sp_dups()
print '- entities with null service path before processing: %d' % null_service_path()
fix_null_sps()
print '- entities with null service path after processing: %d' % null_service_path()

if need_fix:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Please, check the documentation at https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide#Upgrading_to_0.19.0_and_beyond_from_any_pre-0.19.0_version for solving it"
