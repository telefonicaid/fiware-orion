#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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

from pymongo import MongoClient, DESCENDING
from datetime import datetime, timedelta
from sys import argv

def usage():
    print '   Usage:   ./list-entities.py <db> <hour|day|week|month|all> [entity_filter] '
    print '   Example  ./list-entities.py orion week'
    print '   Example  ./list-entities.py orion week TEST_SENSOR'

def entityString(entity):
    s = entity['id']
    if (entity.has_key('type')):
        s += ' (' + entity['type'] + ')'
    return s

# This script can be easily adapted to used creation date instead of modification date
# just changing the following variable to 'creDate'
refDate = 'modDate'

if argv[1] == "-u": 
    usage()
    exit(0)

if 3 <= len(argv) <= 4:
    db = argv[1]
    range = argv[2]
else:
    print 'Wrong number of arguments'
    usage()
    exit(1)

# Check range string
if (range != "hour" and range != "day" and range != "week" and range != "month" and range != "all"):
    print 'Wrong range string: ' + range
    usage()
    exit(1)

# Optional argument: filter
query = {}
if len(argv) == 4:
    query['_id.id'] = {'$regex': argv[3]}

client = MongoClient('localhost', 27017)
col = client[db]['entities']

now = datetime.now()
last_hour = now - timedelta(hours=1)
last_day = now - timedelta(days=1)
last_week = now - timedelta (days=7)
last_month = now - timedelta (days=30)

day_mark_printed = False
week_mark_printed = False
month_mark_printed = False
old_mark_printed = False

query[refDate] = {'$exists': True}

docs = col.find(query)
if (docs.count() == 0):
    print "no entities"
    exit(0)

# First pass: count documents in each range
n_hour = 0
n_day = 0
n_week = 0
n_month = 0
n_old = 0
for doc in docs:
    date = datetime.fromtimestamp(int(doc[refDate]))

    if (date < last_month):
        n_old += 1
    elif (date < last_week):
        n_month += 1
    elif (date < last_day):
        n_week += 1
    elif (date < last_hour):
        n_day += 1
    else:
        n_hour += 1

# Second pass: printing entity information itself
if (n_hour > 0):
    print "=== updated in last hour (" + str(n_hour) + " entities)"

for doc in col.find(query).sort(refDate, direction=DESCENDING):
    date = datetime.fromtimestamp(int(doc[refDate]))

    if ((range == "hour" and date < last_hour) or (range == "day" and date < last_day) or
        (range == "week" and date < last_week) or (range == "month" and date <last_month)):
        break

    if (date < last_month and not old_mark_printed and n_old > 0):
        print "=== older than one month (" + str(n_old) + " entities)"
        old_mark_printed = True
    elif (date < last_week and not month_mark_printed and n_month > 0):
        print "=== updated in last month (" + str(n_month)+ " entities)"
        month_mark_printed = True
    elif (date < last_day and not week_mark_printed and n_week > 0):
        print "=== updated in last week (" + str(n_week) + " entities)"
        week_mark_printed = True
    elif (date < last_hour and not day_mark_printed and n_day > 0):
        print "=== updated in last day (" + str(n_day) + " entities)"
        day_mark_printed = True

    dateString = date.strftime('%Y-%m-%d %H:%M:%S')
    print '-- ' + dateString + ': ' + entityString(doc['_id']) 

if (range == "all"):
   query[refDate]['$exists'] = False
   docs = col.find(query)
   n = docs.count()
   if (n > 0):
       print "=== without date (" + str(n) + " entities), probably last update was done with Orion 0.8.0 (released in October 9th, 2013)"
       for doc in docs:
           print '-- (no date) : ' + entityString(doc['_id'])
