#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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


# Copyright 2019 FIWARE Foundation e.V.
#
# This file is part of Orion-LD Context Broker.
#
# Orion-LD Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion-LD Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# orionld at fiware dot org



# -----------------------------------------------------------------------------
#
# To install paho:
#   pip install paho-mqtt
#   
#



# -----------------------------------------------------------------------------
#
# Imports
#
import paho.mqtt.client as mqtt
from getopt import getopt, GetoptError
import sys
import os
import json



# -----------------------------------------------------------------------------
#
# log file
#
logFile = open("/tmp/mqttSend.log", "a+")
logFile.write("mqttSend.py has started\n")
logFile.flush()


# -----------------------------------------------------------------------------
#
# usage -
#
def usage():
    """
    Print usage message
    """

    print 'Usage: %s --host <broker ip> --port <port number> --topic <topic> --payload <payload> -v -u' % os.path.basename(__file__)
    print ''
    print 'Parameters:'
    print "  --host <MQTT Broker IP-address>: (default is 'localhost')"
    print "  --port <port number>: MQTT broker port (default is 1883)"
    print "  --topic <topic>: MQTT topic to subscribe to (default is 'notification')"
    print "  --payload <payload>: payload of the command (default is 'ping')"
    print "  -v: verbose mode"
    print "  -u: print this usage message"



# -----------------------------------------------------------------------------
#
# Command Line Arguments
#
verbose  = 0
host     = "localhost"
port     = 1883
topic    = "notification"
payload  = 'ping'
qos      = 0

logFile.write("Parsing Command Line Arguments\n")
logFile.flush()

try:
    opts, args = getopt(sys.argv[1:], 'vu', [ 'host=', 'port=', 'topic=', 'payload=' ])
except GetoptError:
    print 'Invalid command-line argument\n'
    usage()
    sys.exit(1)


for opt, arg in opts:
    if opt == '-u':
        usage()
        sys.exit(0)
    elif opt == '-v':
        verbose = 1
    elif opt == '--host':
        host = arg
    elif opt == '--port':
        try:
            port = int(arg)
        except ValueError:
            print 'the "--port" value must be an integer'
            sys.exit(1)
    elif opt == '--topic':
        topic = arg
    elif opt == '--payload':
        payload = arg
    else:
        print "no such command-line argument: " + opt
        sys.exit(1)


client = mqtt.Client()
client.connect(host, port, 60)
logFile.write("MQTT Send Client connected to " + host + ":" + str(port) + "\n")
logFile.write("Publishing on '" + topic + "': " + payload + "\n")
client.publish(topic, payload)

logFile.write("mqttSend.py has ended\n\n")
logFile.flush()
sys.exit(0)
