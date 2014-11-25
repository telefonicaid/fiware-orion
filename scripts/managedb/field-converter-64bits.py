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

# This script takes two argument as parameters: collection name and field name. For the documents
# in the collection which field is a number type (Double or 32 bits integer) it converts to a 64 bits integer.
#
# USE THIS SCRIPT WITH CARE, IT WILL MODIFY YOUR DATABASE!! IT IS ADVISABLE TO TAKE A BACKUP OF YOUR
# DATABSE BEFORE USING THIS SCRIPT

from pymongo import MongoClient
from sys import argv

# From http://docs.mongodb.org/manual/reference/operator/query/type/
DOUBLE_TYPEID    = 1
INTEGER32_TYPEID = 16
INTEGER65_TYPEID = 18

def convert_type(type, col, field):
    n = 0
    for doc in col.find({field: {'$type': type}}):
        doc[field] = long(doc[field])
        col.save(doc)
        n += 1
    return n

# Grab arguments
if len(argv) == 3:
    col = argv[1]
    field = argv[2]
else:
    print 'Wrong number of arguments'
    print '   Usage:   ./field-converter-64bits.py.py <colection> <field>'
    exit(1)

client = MongoClient('localhost', 27017)
db = client['orion']

# Process all documents with Double and 32-bits integer
n_double = convert_type(DOUBLE_TYPEID, db[col], field)
n_int32  = convert_type(INTEGER32_TYPEID, db[col], field)
print 'Converted (Double): ' + str(n_double)
print 'Converted (32-bit int): ' + str(n_int32)
