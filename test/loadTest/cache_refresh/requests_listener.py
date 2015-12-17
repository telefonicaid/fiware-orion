# -*- coding: utf-8 -*-
"""
 Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U

 This file is part of Orion Context Broker.

 Orion Context Broker is free software: you can redistribute it and/or
 modify it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 Orion Context Broker is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.

 For those usages not covered by this license please contact with
 iot_support at tid dot es
"""
__author__ = 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

import time
import sys

import BaseHTTPServer

# variables
notif_counter = 0
port_number = 8090
start_time = 0
last_time = 1
info = True


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_POST(s):
        """
        POST request
        """
        global notif_counter, start_time, last_time, info
        notif_counter += 1
        if notif_counter == 1:
            start_time = time.time()
        s.send_response(200)
        last_time = time.time()
        print "url: %s/%s" % (s.headers.get('Host'), s.path)
        length = int(s.headers["Content-Length"])
        if info:
            if length > 0:
                body = s.rfile.read(length)
                print "body: %s" % body
            print "Requests: %s" % str(notif_counter)
            print "--------------------------------------------------"

    def do_GET(s):
        """
        GET request
        """
        global notif_counter, start_time, last_time
        body_temp = ""
        if s.path == "/receive":
            seconds = last_time - start_time
            tps = notif_counter / seconds
            body_temp = '{"requests": "%s","tps": "%.1f/s"}' % (str(notif_counter), tps)
        elif s.path == "/reset":
            notif_counter = 0
            start_time = 0
            last_time = 1
            body_temp = '{"msg":"requests counter has been reseted"}'
        else:
            body_temp = '{"error":"unknown path: %s"}' % s.path
        s.send_response(200)
        s.send_header('Content-type', 'application/json')
        s.send_header("Content-Length", len(body_temp))
        s.end_headers()
        s.wfile.write(body_temp)


def __usage():
    """
    usage message
    """
    print " *****************************************************************"
    print " * This listener count all POST requests received.               *"
    print " *  usage: python subs_listener.py <-u> <-p=port>                *"
    print " *           ex: python subs_listener.py -p=8092                 *"
    print " *                                                               *"
    print " *  parameters:                                                  *"
    print " *         -u: show this usage.                                  *"
    print " *         -hide: hide info by console (body and reqs number).   *"
    print " *         -p: change of mock port (by default 8090).            *"
    print " *                                                               *"
    print " *  API requests:                                                *"
    print " *    GET /receive - returns requests total and TPS since first  *"
    print " *                   request to last request received.           *"
    print " *    GET /reset   - reset requests counter                      *"
    print " *                                                               *"
    print " *                    ( use <Ctrl-C> to stop )                   *"
    print " *****************************************************************"
    exit(0)


def configuration(arguments):
    """
    Define values for configuration
    :param arguments: parameters in command line
    """
    global port_number, info
    error_msg = ""
    for i in range(len(arguments)):
        if arguments[i].find('-u') >= 0:
            __usage()
        if arguments[i].find('-hide') >= 0:
            info = False
        try:
            if arguments[i].find('-p') >= 0:
                error_msg = "port parameter"
                port_number = int(str(arguments[i]).split("=")[1])
        except Exception, e:
            print "ERROR - in (" + error_msg + ") see usage below:\n   -  " + str(e)
            __usage()
    if info:
        print "The info is displayed by console (current body and number of requests received...)"
    else:
        print "Hide property is activated, the info received is hidden by console..."

if __name__ == '__main__':
    configuration(sys.argv)
    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class(('0.0.0.0', port_number), MyHandler)
    print " CRTL - C to stop the server"
    print time.asctime(), "Server Starts  "

    try:
        httpd.serve_forever()

    except KeyboardInterrupt:
        print "Closing the server..."
    httpd.server_close()
    print time.asctime(), "Server Stops"
