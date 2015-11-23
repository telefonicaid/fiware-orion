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
import getpass
import socket
import time
from sys import argv, exit
from pymongo import MongoClient, DESCENDING
from bson.objectid import ObjectId
from urlparse import urlparse
from subprocess import call

MAX_PER_IP = 4

def usage():
    print '%s --db <database> [--with-restart <status_file>] [--dry-run] [-v]' % basename

def msg(s):
    if verbose:
        print s

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
n = 0
ips = {}
to_delete = []
for doc in db[COL].find({'conditions.type': 'ONTIMEINTERVAL'}):
    n += 1
    csub_id = str(doc['_id'])

    # Get reference field and process URL
    url = urlparse(doc['reference'])
    if (url.hostname == None):
        msg('WARNING: none hostname in csub %s' % csub_id)
        continue

    # Get IP (using DNS translation, in the case the hostname is not an actual IP)
    ip = socket.gethostbyname(url.hostname)

    # Accumulate results
    if ip in ips.keys():
        # Seen before
        ips[ip] += 1
    else:
        # Not seen before
        ips[ip] = 1

    # If the threshold has been already reached, then csub is add for deletion
    if ips[ip] > MAX_PER_IP:
        msg('INFO: csub_id %s overpass threshold for IP %s (%s)' % (csub_id, url.hostname, ip))
        to_delete.append(csub_id)

msg('INFO: total csub ONTIMEINTERVAL count %d' % n)
msg('INFO: documents to delete: %d' % len(to_delete))

# Removing documents
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
