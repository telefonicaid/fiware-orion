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
#   "Content-Type: application/x-www-form-urlencoded" has been problematic in the pass)

from OpenSSL import SSL
from flask import Flask, request, Response
from getopt import getopt, GetoptError
from datetime import datetime
from math import trunc
from time import sleep
import sys
import os
import atexit
import string
import signal
import json

def usage_and_exit(msg):
    """
    Print usage message and exit"

    :param msg: optional error message to print
    """

    if msg != '':
        print msg
        print

    usage()
    sys.exit(1)


def usage():
    """
    Print usage message
    """

    print 'Usage: %s --host <host> --port <port> --url <server url> --pretty-print -v -u' % os.path.basename(__file__)
    print ''
    print 'Parameters:'
    print "  --host <host>: host to use database to use (default is '0.0.0.0')"
    print "  --port <port>: port to use (default is 1028)"
    print "  --url <server url>: server URL to use (default is /accumulate)"
    print "  --pretty-print: pretty print mode"
    print "  --https: start in https"
    print "  --key: key file (only used if https is enabled)"
    print "  --cert: cert file (only used if https is enabled)"
    print "  -v: verbose mode"
    print "  -u: print this usage message"


# This function is registered to be called upon termination
def all_done():
    os.unlink(pidfile)

# Default arguments
port       = 1028
host       = '0.0.0.0'
server_url = '/accumulate'
verbose    = 0
pretty     = False
https      = False
key_file   = None
cert_file  = None

try:
    opts, args = getopt(sys.argv[1:], 'vu', ['host=', 'port=', 'url=', 'pretty-print', 'https', 'key=', 'cert=' ])
except GetoptError:
    usage_and_exit('wrong parameter')

for opt, arg in opts:
    if opt == '-u':
        usage()
        sys.exit(0)
    elif opt == '--host':
        host = arg
    elif opt == '--url':
        server_url = arg
    elif opt == '--port':
        try:
            port = int(arg)
        except ValueError:
            usage_and_exit('port parameter must be an integer')
    elif opt == '-v':
        verbose = 1
    elif opt == '--pretty-print':
        pretty = True
    elif opt == '--https':
        https = True
    elif opt == '--key':
        key_file = arg
    elif opt == '--cert':
        cert_file = arg
    else:
        usage_and_exit()

if https:
    if key_file is None or cert_file is None:
        print "if --https is used then you have to provide --key and --cert"
        sys.exit(1)

if verbose:
    print "verbose mode is on"
    print "port: " + str(port)
    print "host: " + str(host)
    print "server_url: " + str(server_url)
    print "pretty: " + str(pretty)
    print "https: " + str(https)
    if https:
        print "key file: " + key_file
        print "cert file: " + cert_file

pid     = str(os.getpid())
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

@app.route("/noresponse", methods=['POST'])
def noresponse():
    sleep(10)
    return Response(status=200)

@app.route("/noresponse/updateContext", methods=['POST'])
def unoresponse():
    sleep(10)
    return Response(status=200)

@app.route("/noresponse/queryContext", methods=['POST'])
def qnoresponse():
    sleep(10)
    return Response(status=200)

# This response has been designed to test the #2360 case, but is general enough to be
# used in other future cases
@app.route("/badresponse/queryContext", methods=['POST'])
def bad_response():
    r = Response(status=404)
    r.data = '{"name":"ENTITY_NOT_FOUND","message":"The entity with the requested id [qa_name_01] was not found."}'
    return r

@app.route("/v1/updateContext", methods=['POST'])
@app.route("/v1/queryContext", methods=['POST'])
@app.route(server_url, methods=['GET', 'POST', 'PUT', 'DELETE', 'PATCH'])
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
    #
    # We have found that request.url can be problematic in some distributions (e.g. Debian 8.2)
    # when used with IPv6, so we use request.scheme, request.host and request.path to compose
    # the URL "manually"
    #
    #  request.url = request.scheme + '://' + request.host + request.path
    #
    s += request.method + ' ' + request.scheme + '://' + request.host + request.path

    # Check for query params
    params = ''
    for k in request.args:
        if (params == ''):
            params = k + '=' + request.args[k]
        else:
            params += '&' + k + '=' + request.args[k]
 
    if (params == ''):
        s += '\n'
    else:
        s += '?' + params + '\n'

    # Store headers
    for h in request.headers.keys():
        s += h + ': ' + request.headers[h] + '\n'
        if ((h == 'Expect') and (request.headers[h] == '100-continue')):
            send_continue = True

    # Store payload
    if ((request.data is not None) and (len(request.data) != 0)):
        s += '\n'
        if pretty == True:
            raw = json.loads(request.data)
            s += json.dumps(raw, indent=4, sort_keys=True)
            s +='\n'
        else:
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
    if (https):
      # According to http://stackoverflow.com/questions/28579142/attributeerror-context-object-has-no-attribute-wrap-socket/28590266, the
      # original way of using context is deprecated. New way is simpler. However, we are still testing this... some environments
      # fail in some configurations (the current one is an attempt to make this to work at jenkins)
      context = SSL.Context(SSL.SSLv23_METHOD)
      context.use_privatekey_file(key_file)
      context.use_certificate_file(cert_file)
      #context = (cert_file, key_file)
      app.run(host=host, port=port, debug=False, ssl_context=context)
    else:
      app.run(host=host, port=port)
