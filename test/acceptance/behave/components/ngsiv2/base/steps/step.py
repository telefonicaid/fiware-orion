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
from components.common_steps.requests import *

# --------------------------------- common steps -------------------------------------
# ---- initial_steps.py --------------------------------------------------------------
"""
@step(u'update properties.py from "([^"]*)" and sudo local "([^"]*)"')
@step(u'update contextBroker config file and restart service')
@step(u'verify contextBroker is installed successfully')
@step(u'verify mongo is installed successfully')
"""

# ---- requests.py -------------------------------------------------------------------
"""
@step (u'send a base request')
@step (u'verify that receive an "([^"]*)" http code')
@step (u'verify main paths')
"""