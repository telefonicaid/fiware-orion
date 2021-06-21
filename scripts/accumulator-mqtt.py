#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
__author__ = 'burak'

# The aim of this program is to subscribe to a topic on an MQTT broker and store every message published 
# on the given channel to be dumped upon request.
# The program subscribes to the topic passed (default accumulate_mqtt) and 'dump_mqtt'. Accumulated
# messages are dumped when a new message is published on the 'dump_mqtt' topic.
# accumulator-mqtt.py plays the role of a orion subscriber that wants to be notified via mqtt and is used 
# by the test harness.

import paho.mqtt.client as mqtt
from getopt import getopt, GetoptError
from datetime import datetime
from math import trunc
from time import sleep
import sys
import os
import atexit
import string
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

    print 'Usage: %s --host <host> --port <port> --topic <topic> --pretty-print -v -u' % os.path.basename(__file__)
    print ''
    print 'Parameters:'
    print "  --host <host>: mqtt host to use (default is '0.0.0.0')"
    print "  --port <port>: port to use (default is 1883)"
    print "  --topic <topic>: mqtt topic to use (default is accumulate_mqtt)"
    print "  --pretty-print: pretty print mode"
    print "  -v: verbose mode"
    print "  -u: print this usage message"


# This function is registered to be called upon termination
def all_done():
    os.unlink(pidfile)

# Default arguments
port       = 1883
host       = '0.0.0.0'
acc_topic  = 'accumulate_mqtt'
dump_topic = 'dump_mqtt'
verbose    = 0
pretty     = False

try:
    opts, args = getopt(sys.argv[1:], 'vu', ['host=', 'port=', 'topic=', 'pretty-print' ])
except GetoptError:
    usage_and_exit('wrong parameter')

for opt, arg in opts:
    if opt == '-u':
        usage()
        sys.exit(0)
    elif opt == '--host':
        host = arg
    elif opt == '--topic':
        acc_topic = arg
    elif opt == '--port':
        try:
            port = int(arg)
        except ValueError:
            usage_and_exit('port parameter must be an integer')
    elif opt == '-v':
        verbose = 1
    elif opt == '--pretty-print':
        pretty = True
    else:
        usage_and_exit()

if verbose:
    print "verbose mode is on"
    print "port: " + str(port)
    print "host: " + str(host)
    print "topic: " + str(acc_topic)
    print "pretty: " + str(pretty)

pid     = str(os.getpid())
pidfile = "/tmp/mqtt.accumulator." + str(port) + ".pid"

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

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(acc_topic)
    client.subscribe(dump_topic)

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):

    global ac, t0, times
    s = '' 

    if (msg.topic == dump_topic):
        print(ac)
    elif (msg.topic == acc_topic):
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

        s += '\n'
        if pretty == True:
            raw = json.loads(str(msg.payload))
            s += json.dumps(raw, indent=4, sort_keys=True)
            s +='\n'
        else:
            s += str(msg.payload)

        # Separator
        s += '=======================================\n'

        # Accumulate
        ac += s

        if verbose:
            print s

ac = ''
t0 = ''
times = []

if __name__ == '__main__':
    
    # Initialize the MQTT Client and set callback methods
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(host, port, 60)
 
    # Blocking call that processes network traffic, dispatches callbacks and
    # handles reconnecting.
    # Other loop*() functions are available that give a threaded interface and a
    # manual interface.
    client.loop_forever()

