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
#   "Content-Type: application/x-www-form-urlencoded" has been problematic in the past)

from flask import Flask, request, Response
from datetime import datetime
from math import trunc
from time import sleep
import os
import atexit
import string
import signal
import argparse
import sys
from OpenSSL import SSL

# This function is registered to be called upon termination
def all_done():
    os.unlink(pidfile)

# Parse arguments from command line
parser = argparse.ArgumentParser()
parser.add_argument("--port", help='The port that will be listened to', type=int, default=1028)
parser.add_argument("--host", help='The host that will represent the accumulator', default='0.0.0.0')
parser.add_argument("--url", help='The URL that will hold the test resource (i.e. "/notify"',default='/accumulate')
parser.add_argument("--verbose", help='Verbosity on or off', default=0)
parser.add_argument("--cert", help='Certificate to use for the server (PEM)')
parser.add_argument("--key", help='Private key to use for the server (PEM)')
args = parser.parse_args()

# Get arguments from command-line parameters
port = args.port
host = args.host
server_url = args.url
verbose = args.verbose
https = 0

# If we are provided both key and cert we launch in SSL mode
if args.cert or args.key:
    if args.cert and args.key:
        https = 1
	context = (args.cert, args.key)
    else:
        print "Error: You must specify both a certificate and a key to use SSL"
        sys.exit(1)


pid = str(os.getpid())
pidfile = "/tmp/accumulator." + str(port) + ".pid"

#
# If an accumulator process is already running, it is killed.
# First using SIGTERM, then SIGINT and finally SIGKILL
# The exception handling is needed as this process dies in case
# a kill is issued on a non-running process ...
#
if os.path.isfile(pidfile):
    oldpid = file(pidfile, 'r').read()
    opid   = string.atoi(oldpid)
    print "PID file %s already exists, killing the process %s" % (pidfile, oldpid)

    try: 
        oldstderr = sys.stderr
        sys.stderr = open("/dev/null", "w")
        os.kill(opid, signal.SIGTERM);
        sleep(0.1)
        os.kill(opid, signal.SIGINT);
        sleep(0.1)
        os.kill(opid, signal.SIGKILL);
        sys.stderr = oldstderr
    except:
        print "Process %d killed" % opid


#
# Creating the pidfile of the currently running process
#
file(pidfile, 'w').write(pid)

#
# Making the function all_done being executed on exit of this process.
# all_done removes the pidfile
#
atexit.register(all_done)


app = Flask(__name__)

@app.route("/v1/updateContext", methods=['POST'])
@app.route("/v1/queryContext", methods=['POST'])
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
    # Note that using debug=True breaks the the procedure to write the PID into a file. In particular
    # makes the calle os.path.isfile(pidfile) return True, even if the file doesn't exist. Thus,
    # use debug=True below with care :)
    if https == 1:
        app.run(host=host, port=port, ssl_context=context)
    else:
        app.run(host=host, port=port)
