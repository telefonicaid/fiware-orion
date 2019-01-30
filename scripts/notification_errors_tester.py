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

__author__ = 'fermin'

# This is a simplified version of accumulator_server.py to test notification error processing in Orion.
#
# Example on how to start CB and this program for the tests:
#
#   contextBroker -fg -httpTimeout 10000 -logLevel INFO -notificationMode threadpool:10:100 -multiservice -subCacheIval 180
#   notification_error_tester.py -v
#
# The following fragment can be very useful (just copy-paste in your terminal :) to create subscriptions
# to be used in tests with this program.

"""
### Subscriptions (8 cases)

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test 200 reponses",
  "subject": {
    "entities": [
      {
        "id": "E200",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/giveme200"
    }    
  }  
}
EOF

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test 200 reponses",
  "subject": {
    "entities": [
      {
        "id": "E400",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/giveme400"
    }    
  }  
}
EOF

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test 404 reponses",
  "subject": {
    "entities": [
      {
        "id": "E404",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/giveme404"
    }    
  }  
}
EOF

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test 500 reponses",
  "subject": {
    "entities": [
      {
        "id": "E500",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/giveme500"
    }    
  }  
}
EOF

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test reponses with timeout",
  "subject": {
    "entities": [
      {
        "id": "EDelay",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }    
  }  
}
EOF

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test reponses to unknown port",
  "subject": {
    "entities": [
      {
        "id": "EUnknownPort",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://localhost:9999/giveme"
    }    
  }  
}
EOF

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test reponses to unknown name",
  "subject": {
    "entities": [
      {
        "id": "EUnknownName",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://foo.bar.bar.com:9999/giveme"
    }    
  }  
}
EOF

curl -v localhost:1026/v2/subscriptions -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "Test reponses to invalid IP",
  "subject": {
    "entities": [
      {
        "id": "EInvalidIP",
        "type": "T"
      }
    ]
  },
  "notification": {
    "http": {
      "url": "http://12.34.56.78:9999/giveme"
    }    
  }  
}
EOF

### To create entities (8 cases):

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E200",
  "type": "T",
  "A": {//alarmMgr.notificationError(url, "(curl_easy_perform failed: " + std::string(curl_easy_strerror(res)) + ")");
    "value": 1,
    "type": "Number"
  }
}
EOF

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E400",
  "type": "T",
  "A": {
    "value": 1,
    "type": "Number"
  }
}
EOF

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E404",
  "type": "T",
  "A": {
    "value": 1,
    "type": "Number"
  }
}
EOF

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E500",
  "type": "T",
  "A": {
    "value": 1,
    "type": "Number"
  }
}
EOF

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "EDelay",
  "type": "T",
  "A": {
    "value": 1,
    "type": "Number"
  }
}
EOF

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "EUnknownPort",
  "type": "T",
  "A": {
    "value": 1,
    "type": "Number"
  }
}
EOF

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "EUnknownName",
  "type": "T",
  "A": {
    "value": 1,
    "type": "Number"
  }
}
EOF

curl -v localhost:1026/v2/entities -s -S -H 'fiware-service: AAA' -H 'fiware-servicepath: /BBB' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "EInvalidIP",
  "type": "T",
  "A": {
    "value": 1,
    "type": "Number"
  }
}
EOF

"""

# FIXME: they come from accumulator-server.py, maybe some of them are now uneeded and should be removed
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

    print 'Usage: %s --host <host> --port <port> --url <server url> -v -u' % os.path.basename(__file__)
    print ''
    print 'Parameters:'
    print "  --host <host>: host to use database to use (default is '0.0.0.0')"
    print "  --port <port>: port to use (default is 1028)"
    print "  --url <server url>: server URL to use (default is /)"
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
url_root   = '/'
verbose    = 0
https      = False
key_file   = None
cert_file  = None

try:
    opts, args = getopt(sys.argv[1:], 'vu', ['host=', 'port=', 'url=', 'https', 'key=', 'cert=' ])
except GetoptError:
    usage_and_exit('wrong parameter')

for opt, arg in opts:
    if opt == '-u':
        usage()
        sys.exit(0)
    elif opt == '--host':
        host = arg
    elif opt == '--url':
        url_root = arg
    elif opt == '--port':
        try:
            port = int(arg)
        except ValueError:
            usage_and_exit('port parameter must be an integer')
    elif opt == '-v':
        verbose = 1
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
    print "url_root: " + str(url_root)
    print "https: " + str(https)
    if https:
        print "key file: " + key_file
        print "cert file: " + cert_file


app = Flask(__name__)

@app.route(url_root + "giveme200", methods=['POST'])
def giveme200():
    return Response(status=200)

@app.route(url_root + "giveme400", methods=['POST'])
def giveme400():
    return Response(status=400)

@app.route(url_root + "giveme404", methods=['POST'])
def giveme404():
    return Response(status=404)

@app.route(url_root + "giveme500", methods=['POST'])
def giveme500():
    return Response(status=500)

@app.route(url_root + "givemeDelay", methods=['POST'])
def givemeDelay():
    sleep(60)
    return Response(status=200)


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
