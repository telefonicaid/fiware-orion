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

import requests
import logging

from tools import general_utils


# general constants
EMPTY   = u''
TRUE    = u'true'
XML     = u'XML'
JSON    = u'JSON'

# requests constants
VERSION = u'version'
ORION   = u'orion'


#HTTP status code
status_codes = {'OK': 200,
                'Created': 201,
                'No Content': 204,
                'Moved Permanently': 301,
                'Redirect': 307,
                'Bad Request': 400,
                'unauthorized': 401,
                'Not Found': 404,
                'Bad Method': 405,
                'Not Acceptable':406,
                'Conflict': 409,
                'Unsupported Media Type': 415,
                'Internal Server Error': 500}

__logger__ = logging.getLogger("utils")

class CB:
    """
    manage Context broker operations
    """
    def __init__(self,  **kwargs):
        """
        constructor
        :param protocol: protocol used in context broker requests
        :param host: host used by context broker
        :param port: port used by context broker
        :param version : contextBroker version
        :param verify_version : determine if verify context broker version or not (True | False)
        :param user: user used to connect by fabric
        :param password: password used to connect by fabric, if use cert file, password will be None
        :param cert_file: cert_file used to connect by fabric, if use password, cert_file will be None
        :param retry: Number of times Fabric will attempt to connect when connecting to a new server
        :param sudo: operations in cygnus with superuser privileges (True | False)
        :param log_file: log file used by context broker
        :param log_owner: log file's owner of context broker
        :param log_group: log file's group of context broker
        :param log_mod: log file's mod of context broker
        """
        self.cb_protocol         = kwargs.get("protocol", "http")
        self.cb_host             = kwargs.get("host", "localhost")
        self.cb_port             = kwargs.get("port", "1026")
        self.cb_version          = kwargs.get("version", "0.22.0")
        self.cb_verify_version   = kwargs.get("verify_version", "false")
        self.cb_fabric_user      = kwargs.get("user", EMPTY)
        self.cb_fabric_pass      = kwargs.get("password", EMPTY)
        self.cb_fabric_cert      = kwargs.get("cert_file", EMPTY)
        self.cb_fabric_retry     = kwargs.get("retry", "3")
        self.cb_fabric_sudo      = kwargs.get("sudo", "False")
        self.cb_log_file         = kwargs.get("log_file", EMPTY)
        self.cb_log_owner        = kwargs.get("log_owner", "orion")
        self.cb_log_group        = kwargs.get("log_group", "orion")
        self.cb_log_mod          = kwargs.get("log_mod", "777")

        self.cb_url              = "%s://%s:%s" % (self.cb_protocol, self.cb_host, self.cb_port)

    # ------------------------------------ requests --------------------------------------------

    def send_base_request(self):
        """
        send a base request
        """
        self.resp = requests.get(url="%s/%s" % (self.cb_url, "v2"))

    # ------------------------------------ validations ------------------------------------------

    def verify_version(self, version=EMPTY):
        """
        verify if the context broker version is th expected
        :param version: version expected (OPTIONAL)
        """
        if version != EMPTY: self.cb_version = version
        if self.cb_verify_version.lower().find(TRUE) >=0:
            resp = requests.get(url="%s/%s" % (self.cb_url, VERSION), headers={"Accept": "application/json"})
            assert resp.status_code == 200, " ERROR - status code in context broker version request. \n " \
                                            " status code: %s \n " \
                                            " body: %s" % (resp.status_code, resp.text)
            __logger__.info(" -- status code is 200 OK in base request v2")
            resp_dict = general_utils.convert_str_to_dict(str(resp.text), JSON)

            assert resp_dict[ORION][VERSION].find(self.cb_version) >= 0, " ERROR in context broker version  value, \n" \
                                                                 " expected: %s \n" \
                                                                 " installed: %s" % (self.cb_version, resp_dict[ORION][VERSION])
            __logger__.info("-- version %s is correct in base request v2" % self.cb_version)

    def verify_http_code(self, http_code):
        """
        Evaluate if the status code is the expected
        :param http_code: http code expected
        """
        assert self.resp.status_code == status_codes[http_code], " ERROR - http code is wrong\n" \
                                                                 " expected: %s \n" \
                                                                 " received: %s" % (str(status_codes[http_code]), str(self.resp.status_code))

    def verify_main_paths(self):
        """
        verify_main_paths ib base request ex:
            {
                "entities_url":"/v2/entities",
                "types_url":"/v2/types",
                "subscriptions_url":"/v2/subscriptions",
                "registrations_url":"/v2/registrations"
            }
        """
        resp_dict = general_utils.convert_str_to_dict(self.resp.text, JSON)
        assert resp_dict["entities_url"] == "/v2/entities", " ERROR - in \"entities_url\": \"/v2/entities\" "
        assert resp_dict["types_url"] == "/v2/types", " ERROR - in \"types_url\":\"/v2/types\""
        assert resp_dict["subscriptions_url"] == "/v2/subscriptions", " ERROR - in \"subscriptions_url\":\"/v2/subscriptions\""
        assert resp_dict["registrations_url"] == "/v2/registrations", " ERROR - in \"registrations_url\":\"/v2/registrations\""


