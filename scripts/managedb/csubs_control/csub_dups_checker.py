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

import getopt
import hashlib
import pickle
import getpass
import time
from sys import argv, exit
from pymongo import MongoClient, DESCENDING
from bson.objectid import ObjectId
from subprocess import call

def msg(s):
    if verbose:
        print s

def usage():
    print '%s --db <database> [--with-restart <status_file>] [--dry-run] [-v]' % basename

def log_status_restart():
    now = time.strftime('%d-%m-%Y %H:%M:%S')
    with open(status_file, 'a') as file:
        file.write('%s - contextBroker restart by %' %(now, basename))

# Argument parsing
basename = __file__
DB = None
with_restart = False
dry_run = False
verbose = False
try:
   opts, args = getopt.getopt(argv[1:], 'v', ['db=', 'with-restart=', 'dry-run'])
except getopt.GetoptError as err:
   print str(err)
   usage()
   exit(1)
for o, a in opts:
   if o == '--db':
       DB = a
   elif o == '--with-restart':
       with_restart = True
       status_file = a
   elif o == '--dry-run':
       dry_run = True
   elif o == '-v':
       verbose = True
   else:
       assert False, 'unhandled option'

if DB == None:
    print 'ERROR: missing db name'
    exit(1)

if with_restart and getpass.getuser() != 'root':
    print 'ERROR: only root can launch this script using --with-restart'
    exit(1)

COL = 'csubs'
msg('INFO: parameter DB: %s' % DB)
msg('INFO: parameter with_restart %s' % str(with_restart))
msg('INFO: parameter dry_run: %s' % str(dry_run))

client = MongoClient('localhost', 27017)
db = client[DB]

# csub document processing is based on the model described at
# https://github.com/telefonicaid/fiware-orion/blob/develop/doc/manuals/admin/database_model.md#csubs-collection

# Firs stage: analyze csubs documents
duplicates = {}

# Note that sorting by {expiration: -1} will get subscriptions by decreasing expiration order, thus
# in the case of deleting a duplicated document, the first one in the duplicated set (i.e. the
# one with higher expiration time) is the one that is preserved. Using a different ordering order based
# on _id we could change the policy to preserve the first csub created
n = 0
for doc in db[COL].find().sort('expiration', DESCENDING):
    n += 1
    csub_id = str(doc['_id'])

    # The following fields are removed from signature calculation: _id, lastNotification, count and expiration
    # (not sure about throttling... as I have the doubt, I'm leaving it byt the moment)
    doc.pop('_id', None)
    doc.pop('latsNotification', None)
    doc.pop('count', None)
    doc.pop('expiration', None)

    # The following fields are list that need to be ordered: entities, attrs, conditions
    # fields using a dictionary use hashlib/pickle to avoid processing internal fields
    doc['entities'] = sorted(doc['entities'], key=lambda k: hashlib.md5(pickle.dumps(k)).hexdigest())
    doc['conditions'] = sorted(doc['conditions'], key=lambda k: hashlib.md5(pickle.dumps(k)).hexdigest())
    doc['attrs'].sort()

    # In conditions of type ONCHANGE, the conditions value is a list of attribute string that
    # also needs to be ordered
    for cond in doc['conditions']:
        if cond['type'] == 'ONCHANGE':
            cond['value'].sort()

    # Get the hash
    hash = hashlib.md5(pickle.dumps(doc)).hexdigest()
    msg('INFO: csub hash: %s -> %s' % (csub_id, hash))

    # Check hash
    if duplicates.has_key(hash):
        # Seen before: add to the list
        duplicates[hash].append(csub_id)
    else:
        # Not seen before: create list and first element
        duplicates[hash] = [csub_id]

msg('INFO: total csub count %d' % n)

# Second stage: get list to remove
savings = 0
to_delete = []
for hash in duplicates.keys():
    if (len(duplicates[hash]) > 1):
        msg('INFO: duplicate csubs for hash %s: %d (%s)' % (hash, len(duplicates[hash]),duplicates[hash] ))
        savings += len(duplicates[hash]) - 1
        # All except the first one are documents to be deleted
        to_delete.extend(duplicates[hash][1:])

msg('INFO: csubs duplicated documents could be removed: %d' % savings)
msg('INFO: documents to delete: %d' % len(to_delete))

# Third stage: removing
n = 0
if not dry_run:
    need_restart = False
    for csub_id in to_delete:
        n += 1
        print 'INFO: csub to be removed: %s' % csub_id
        if not dry_run:
            # FIXME: a bulk delete would be more efficient
            db[COL].remove({'_id': ObjectId(csub_id)})
            need_restart = True
    msg('INFO: processed %d documents' % n)

    if need_restart and with_restart:
        call(['/etc/init.d/contextBroker', 'restart'])
        log_status_restart()
