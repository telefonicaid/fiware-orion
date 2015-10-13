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


from components.common_steps.initial_steps import *
from components.common_steps.general_steps import *

#  common steps
# ---- initial_steps.py --------------------------------------------------------------
"""
@step(u'update properties test file from "([^"]*)" and sudo local "([^"]*)"')
@step(u'update contextBroker config file')
@step(u'start ContextBroker')
@step(u'stop ContextBroker')
@step(u'verify contextBroker is installed successfully')
@step(u'verify mongo is installed successfully')
"""

# ---- general_steps.py ---------------------------------------------------------------
"""
@step(u'send a API entry point request')
@step(u'send a version request')
@step(u'send a statistics request')
@step(u'delete database in mongo')
@step(u'verify that receive an "([^"]*)" http code')
@step(u'verify "([^"]*)" url with "([^"]*)" value in response')
@step(u'verify statistics "([^"]*)" field does exists')
@step(u'verify version "([^"]*)" field does exists')
@step(u'verify if version is the expected')
@step(u'verify that receive several "([^"]*)" http code')
@step(u'verify an error response')
@step(u'verify several error responses')
"""
