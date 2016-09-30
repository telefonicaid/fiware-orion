# -*- coding: utf-8 -*-
"""
 Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U

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

import json
import logging
import BaseHTTPServer
import threading


__logger__ = logging.getLogger("listener")


# variables
default_payload = json.dumps({"msg": "without notification received"})
unknown_path = json.dumps({"error": "unknown path: %s"})
last_url = ""
last_headers = {}
last_payload = default_payload
VERBOSE = True


def get_last_data(s):
    """
    Store the request data (method, url, path, headers, payload)
    :param s: it is a reference to the BaseHTTPServer object
    """
    global last_url, last_headers, last_payload
    # last url
    last_url = "%s - %s://%s%s" % (s.command, s.request_version.lower().split("/")[0], s.headers["host"], s.path)

    # last headers
    for item in s.headers:
        last_headers[item] = s.headers[item]

    # last payload
    length = int(s.headers["Content-Length"])
    if length > 0:
        last_payload = str(s.rfile.read(length))


def show_last_data():
    """
    display all data from request in log
    :return: string
    """
    global last_url, last_headers, last_payload, VERBOSE, __logger__
    if VERBOSE:
        __logger__.debug("Request data received in listener:\n Url: %s\n Headers: %s\n Payload: %s\n" % (last_url, str(last_headers), last_payload))


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    """
    HTTP server
    """
    def do_POST(self):
        """
        POST request
        """
        global last_headers, last_payload, last_url, get_last_data, show_last_data
        __logger__.info("a POST request is received")
        self.send_response(200)
        get_last_data(self)
        headers_prefix = u'last'
        for item in last_headers:
            self.send_header("%s-%s" % (headers_prefix, item), last_headers[item])
        self.send_header("%s-url" % headers_prefix, "%s" % last_url)

        show_last_data()   # verify VERBOSE global variable

        self.end_headers()
        self.wfile.write(last_payload)

    def do_PUT(self):
        """
        PUT request
        """
        global last_headers, last_payload, last_url, get_last_data, show_last_data
        __logger__.info("a PUT request is received")
        self.send_response(200)
        get_last_data(self)
        headers_prefix = u'last'
        for item in last_headers:
            self.send_header("%s-%s" % (headers_prefix, item), last_headers[item])
        self.send_header("%s-url" % headers_prefix, "%s" % (last_url))

        show_last_data()   # verify VERBOSE global variable

        self.end_headers()
        self.wfile.write(last_payload)

    def do_DELETE(self):
        """
        DELETE request
        """
        global last_headers, last_payload, last_url, get_last_data, show_last_data
        __logger__.info("a DELETE request is received")
        self.send_response(200)
        get_last_data(self)
        headers_prefix = u'last'
        for item in last_headers:
            self.send_header("%s-%s" % (headers_prefix, item), last_headers[item])
        self.send_header("%s-url" % headers_prefix, "%s" % (last_url))

        show_last_data()   # verify VERBOSE global variable

        self.end_headers()
        self.wfile.write(last_payload)

    def do_PATCH(self):
        """
        PATCH request
        """
        global last_headers, last_payload, last_url, get_last_data, show_last_data
        __logger__.info("a PATCH request is received")
        self.send_response(200)
        get_last_data(self)
        headers_prefix = u'last'
        for item in last_headers:
            self.send_header("%s-%s" % (headers_prefix, item), last_headers[item])
        self.send_header("%s-url" % headers_prefix, "%s" % (last_url))

        show_last_data()   # verify VERBOSE global variable

        self.end_headers()
        self.wfile.write(last_payload)

    def do_GET(self):
        """
        GET request
        """
        global last_headers, last_payload, unknown_path, last_url
        __logger__.info("a GET request is received")
        self.send_response(200)
        if self.path == "/last_notification":
            headers_prefix = u'last'
            for item in last_headers:
                self.send_header("%s-%s" % (headers_prefix, item), last_headers[item])
            self.send_header("%s-url" % headers_prefix, "%s" % last_url)
        elif self.path == "/reset":
            last_url = ""
            last_headers = {}
            last_payload = default_payload
        else:
            last_payload = unknown_path % self.path

        show_last_data()   # verify VERBOSE global variable

        self.end_headers()
        self.wfile.write(last_payload)


class Daemon:
    def __init__(self, **kwargs):
        """
        constructor
        :param file: file to execute
        :param param: parameters list (arguments)
        :param name: daemon name
        """
        global VERBOSE
        port = kwargs.get("port", 1030)
        ip_bind = kwargs.get("ip_bind", u'0.0.0.0')
        name = kwargs.get("name", 'Daemon')
        verbose = kwargs.get("verbose", False)
        VERBOSE = verbose
        try:
            self.d = threading.Thread(target=self.__listener, args=(port, ip_bind), name=name, verbose=verbose)
            self.d.setDaemon(True)
            self.d.start()

        except Exception, e:
            print("ERROR - %s" % e)

    def __listener(self, port, ip_bind):
        """
        execute a file (script)
        :param port: port where the listener is running
        :param ip_bind: the server accepts TCP/IP connections on  server host IPv4 or/and IPv6 interfaces
        """
        server_class = BaseHTTPServer.HTTPServer
        httpd = server_class((ip_bind, int(port)), MyHandler)
        __logger__.info("HTTP Server Started as a Daemon using the %s port" % port)
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            exit(0)
            __logger__.debug("Closing the HTTP server...")
        httpd.server_close()

    def is_alive_and_is_a_daemon(self):
        """
        determine if the thread if a daemon and if it is alive
        :return: boolean
        """
        if self.d.is_alive and self.d.isDaemon():
            return True
        return False

    def daemon_config(self):
        """
        returns daemon configuration
        :return: dict
        """
        return self.d.__dict__
