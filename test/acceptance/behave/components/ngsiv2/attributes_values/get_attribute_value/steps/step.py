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


from components.common_steps.initial_steps import *
from components.common_steps.general_steps import *
from components.common_steps.entities.create_update_replace_steps import *
from components.common_steps.entities.get_steps import *
from components.common_steps.entities.delete_steps import *

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

# ---- entities.create_update_replace_steps.py -----------------------------------------
"""
@step(u'a definition of headers')
@step(u'create "([^"]*)" entities with "([^"]*)" attributes')
@step(u'create an entity and attribute with special values in raw')
@step(u'update or append attributes by ID "([^"]*)"')
@step(u'update or append attributes by ID "([^"]*)" in raw mode')
@step(u'update an attribute by ID "([^"]*)" if it exists')
@step(u'update an attribute by ID "([^"]*)" if it exists in raw mode')
@step(u'replace attributes by ID "([^"]*)"')
@step(u'replace attributes by ID "([^"]*)" in raw mode')
@step(u'verify that entities are stored in default tenant at mongo')
@step(u'verify that entities are stored in mongo')
@step(u'verify that entities are not stored in mongo')
@step(u'verify that an entity is updated in mongo')
"""

# ---- entities.get_steps.py -------------------------------------------------------------
"""
@step(u'get all entities')
@step(u'get an entity by ID "([^"]*)"')
@step(u'get an attribute "([^"]*)" by ID "([^"]*)"')
@step(u'verify that all entities are returned')
@step(u'verify an entity in raw mode with type "([^"]*)" in attribute value from http response')
@step(u'verify that the entity by ID is returned')
@step(u'verify that the attribute by ID is returned')
@step(u'verify an attribute by ID in raw mode with type "([^"]*)" in attribute value from http response')
"""

# ---- entities.delete_steps.py -------------------------------------------------------------
"""
@step(u'delete entity with id "([^"]*)"')
"""
