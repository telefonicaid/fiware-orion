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

from __future__ import division # need for seconds calculation (could be removed with Python 2.7)
__author__ = 'fermin'

from pymongo import MongoClient
import sys
from datetime import timedelta, datetime

if len(sys.argv) != 2:
    print "missing db name"
    sys.exit()

DB = sys.argv[1]
COL = 'entities'

need_fix = False

client = MongoClient('localhost', 27017)
db = client[DB]

def warn(n, s):
    print '%5d: WARNING: %s' % (n, s)
    global need_fix
    need_fix = True

def duplicated_attr_name(n, list):
    names = {}
    at_least_one_dup = False
    for attr in list:
        if (attr['name'] in names):
            warn(n, 'attribute %s is duplicated' % attr['name'])
            at_least_one_dup = True
        else:
            names[attr['name']] = 1
    return at_least_one_dup

def has_attr(attr, doc_attrs):
    for a in doc_attrs:
        if a['name'] == attr['name']:
            return True
    return False

def is_attr_in_doc_older(attr, doc, dup):
    """
    This method assumes that attribute is not duplicated in doc or dup (the method is called
    after passing that check)
    """
    
    mod_date_doc = 0
    mod_date_dup = 0

    for a in doc['attrs']:
        if (a['name'] == attr):
            mod_date_doc = a['modDate']

    for a in dup['attrs']:
        if (a['name'] == attr):
            mod_date_dup = a['modDate']

    if (mod_date_doc < mod_date_dup):
        return True
    else:
        return False

def time_string(t_0):
    delta = datetime.now() - t_0
    # Python 2.7 could use delta.total_seconds(), but we need this formula for backward compatibility with Python 2.6
    #t = delta.total_seconds()
    t = (delta.microseconds + (delta.seconds + delta.days * 24 * 3600) * 10**6) / 10**6
    return str(timedelta(seconds=t))

def merge_sp():

    n = 0
    n_dup = 0
    skipped = 0
    print '----- start processing'
    t_0 = datetime.now()

    for doc in db[COL].find({'_id.servicePath': '/'}):
        n += 1
        id = doc['_id']['id']
        if 'type' in doc['_id']:
            type = doc['_id']['type']
        else:
            type = ''

        if type != '':
            dup = db[COL].find_one({'_id.id': id, '_id.type': type, '_id.servicePath': None})
        else:
            dup = db[COL].find_one({'_id.id': id, '_id.type': None, '_id.servicePath': None})
        if dup != None:

           n_dup += 1

           # Sanity check: doc should be newer than dup
           delta = doc['modDate'] - dup['modDate']
           if delta < 0:
               warn(n_dup, 'dup for <%s, %s> is newer than not dup. Skipping.' % (n_dup, id, type))
               skipped += 1
               continue

           print '%5d: candidate to merge for <%s, %s>, time offset: %s' % (n_dup, id, type, str(timedelta(seconds=delta)))

           doc_attrs = doc['attrs']
           dup_attrs = dup['attrs']
           
           # More sanity checks. In release 0.17.0 attribute type was removed from attribute identification, but
           # some old entity document in this way may exist
           if (duplicated_attr_name(n_dup, doc_attrs)):
               warn(n_dup, 'duplicated attribute in doc. Skipping.')
               skipped += 1
               continue
           if (duplicated_attr_name(n_dup, dup_attrs)):
               warn(n_dup, 'duplicated attribute in dup. Skipping.')
               skipped += 1
               continue

           # Parse each attribute in dup. If the attribute is not found in the document, then it is added
           attrs_to_push = []
           for attr in dup['attrs']:
               if not has_attr(attr, doc_attrs):
                   print '%5d: attribute to append <%s>' % (n_dup, attr['name'])
                   attrs_to_push.append(attr)
               else:
                   # Sanity check: modData in the attribute in doc should be newer than modData in dup
                   if is_attr_in_doc_older(attr, doc, dup):
                       warn(n_dup, 'attribute %s is older in doc than in dup. Skipping' % attr)
                       skipped += 1
                       continue

           if (len(attrs_to_push) > 0):
               try:
                   # Pushing attributes
                   print '%5d: pushing %d attributes' % (n_dup, len(attrs_to_push))
                   if type != '':
                       db[COL].update({'_id.id': id, '_id.type': type, '_id.servicePath': '/'}, {'$push': {'attrs': {'$each': attrs_to_push }}})
                   else:
                       db[COL].update({'_id.id': id, '_id.type': None, '_id.servicePath': '/'}, {'$push': {'attrs': {'$each': attrs_to_push }}})
               except:                   
                   print '%5d: ERROR pushing attributes at DB: %s' % (n_dup, str(sys.exc_info()[:2]))
                   global need_fix
                   need_fix = True
                   continue
           else:
               # No attributes to push
               print '%5d: no attributes to push, just remove the old duplicate' % n_dup

           try:
               # Removing dup
               print '%5d: removing duplicate' % n_dup
               if type != '':
                   db[COL].remove({'_id.id': id, '_id.type': type, '_id.servicePath': None})
               else:
                   db[COL].remove({'_id.id': id, '_id.type': None, '_id.servicePath': None})           
           except:                   
               print '%5d: ERROR removing dup at DB: %s' % (n_dup, sys.exc_info()[:2])
               global need_fix
               need_fix = True
               continue

        if (n % 1000 == 0):
            print '----- processed %s (time: %s)' % (n, time_string(t_0))

    print '----- processed in total:  %d (total time: %s)' % (n, time_string(t_0))
    print '----- dup processed:       %d' % n_dup 
    print '----- skipped:             %d' % skipped

# Warn user
print "WARNING!!!! This script modifies your '%s' database. It is STRONGLY RECOMMENDED that you" % DB
print "do a backup of your database before using it as described in https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide#Backup. Use this script at your own risk."
print "If you are sure you want to continue type 'yes' and press Enter"

confirm = raw_input()

if (confirm != 'yes'):
    sys.exit()

merge_sp()

if need_fix:
    print "------------------------------------------------------"
    print "WARNING: some problem was found during the process. Please, check the documentation at https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide#Upgrading_to_0.19.0_and_beyond_from_any_pre-0.19.0_version for solving it"
