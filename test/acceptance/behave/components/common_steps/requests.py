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

import behave
from behave import step
import logging

from tools.properties_config import Properties
from tools.CB import CB

behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")

@step (u'send a base request')
def send_a_base_request(context):
    """
    send a base request
    :param context:
    """
    global cb
    __logger__.debug(" >> sending a base request: /v2")
    properties_class = Properties()
    props = properties_class.read_properties()["context_broker_env"]
    cb = CB(protocol=props["CB_PROTOCOL"], host=props["CB_HOST"], port=props["CB_PORT"], version=props["CB_VERSION"], verify_version=props["CB_VERIFY_VERSION"])
    cb.send_base_request()
    __logger__.info(" >> sent a base request: /v2 correctly")

#  ------------------------------------- validations ----------------------------------------------
@step (u'verify that receive an "([^"]*)" http code')
def verify_that_receive_an_http_code(context, http_code):
    """
    verify that receive an http code
    :param context:
    :param http_code:
    """
    global cb
    __logger__.debug(" >> verifying that return an http code")
    cb.verify_http_code(http_code)
    __logger__.info(" >> verified that http code returned is %s" % http_code)

@step (u'verify main paths')
def verify_main_paths(context):
    """
    verify main paths
    :param context:
    """
    global cb
    __logger__.debug(" >> verifying main paths")
    cb.verify_main_paths()
    __logger__.info(" >> verified that main paths are correct")
