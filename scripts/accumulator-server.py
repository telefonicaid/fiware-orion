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
# fermin at tid dot es

from __future__ import division   # need for seconds calculation (could be removed with Python 2.7)
__author__ = 'fermin'

# This program stores everything it receives by HTTP in a given URL (pased as argument),
# Then return the accumulated data upon receiving 'GET <host>:<port>/dump'. It is aimet
# at harness test for subscription scenarios (so accumulator-server.py plays the role
# of a subscribed application)
#
# Known issues:
#
# * Curl users: use -H "Expect:" (default "Expect: 100-continue" used by curl has been problematic
#   in the past)
# * Curl users: use -H "Content-Type: application/xml"  for XML payload (the default:
#   "Content-Type: application/x-www-form-urlencoded" has been problematic in the pass)

from flask import Flask, request, Response
from sys import argv
from datetime import datetime
from math import trunc
import os
app = Flask(__name__)

# Default arguments
port = 1028
server_url = '/accumulate'
verbose = 0

# Arguments from command line
if len(argv) > 2:
    port = int(argv[1])
    server_url = argv[2]

if len(argv) > 3:
    if argv[3] == 'on':
        print 'verbose mode is on'
        verbose = 1

@app.route(server_url, methods=['GET', 'POST', 'PUT', 'DELETE'])
def record():

    global ac, t0, times
    s = ''
    send_continue = False

    # First notification? Then, set reference datetime. Otherwise, add the
    # timedelta to the list
    if (t0 == ''):
        t0 = datetime.now()
        times.append(0)
    else:
        delta = datetime.now() - t0
        # Python 2.7 could use delta.total_seconds(), but we use this formula
        # for backward compatibility with Python 2.6
        t = (delta.microseconds + (delta.seconds + delta.days * 24 * 3600) * 10**6) / 10**6
        times.append(trunc(round(t)))
        #times.append(t)

    # Store verb and URL
    s += request.method + ' ' + request.url + '\n'

    # Store headers
    for h in request.headers.keys():
        s += h + ': ' + request.headers[h] + '\n'
        if ((h == 'Expect') and (request.headers[h] == '100-continue')):
            send_continue = True

    # Store payload
    if ((request.data is not None) and (len(request.data) != 0)):
        s += '\n'
        s += request.data

    # Separator
    s += '=======================================\n'

    # Accumulate
    ac += s

    if verbose:
        print s

    if send_continue:
        return Response(status=100)
    else:
        return Response(status=200)


@app.route('/dump', methods=['GET'])
def dump():
    return ac


@app.route('/times', methods=['GET'])
def times():
    return ', '.join(map(str,times)) + '\n'


@app.route('/number', methods=['GET'])
def number():
    return str(len(times)) + '\n'


@app.route('/reset', methods=['POST'])
def reset():
    global ac, t0, times
    ac = ''
    t0 = ''
    times = []
    return Response(status=200)


@app.route('/pid', methods=['GET'])
def getPid():
    return str(os.getpid())

# This is the accumulation string
ac = ''
t0 = ''
times = []

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=port, debug=True)
