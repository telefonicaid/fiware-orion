#!/usr/bin/python
# -*- coding: latin-1 -*-
# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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


import sys
import websocket

msg = 0;

def on_message(ws, message):
    print message

    global msg
    msg -= 1
    if (msg == 0):
        ws.close()

def on_error(ws, error):
    print error

def on_close(ws):
    print "Connection closed"

def on_open(ws):
    ws.send(sys.argv[3])

def show_help(reason):
    print "\n" + reason
    print "Usage:"
    print "\t" + sys.argv[0] + " URI protocol payload [responses]\n"
    print "* URI      : websocket server URI."
    print "* protocol : protocol to use in this session."
    print "* payload  : payload to send."
    print "* responses: number of responses to wait before quit. By default is 1"
    print ""


def check_params():
    err = len(sys.argv) < 4
    if (err):
        show_help("Error, missing arguments")
    else:
        global msg
        if (len(sys.argv) == 5):
            try:
                msg = int(sys.argv[4])
            except Exception:
                show_help("Wrong arguments, responses must be a number.")
                return False
        else:
            msg = 1

    return not(err)

if __name__ == "__main__":
    if (check_params()):
        ws = websocket.WebSocketApp(sys.argv[1],
                                    subprotocols=[sys.argv[2]],
                                    on_message = on_message,
                                    on_error = on_error,
                                    on_close = on_close,
                                    on_open = on_open)
        ws.run_forever()
