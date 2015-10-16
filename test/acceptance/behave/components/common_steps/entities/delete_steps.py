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

from iotqatools.helpers_utils import *


behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")

# ------------------------- delete steps ----------------------------

@step(u'delete entities with id "([^"]*)"')
def delete_entities_by_id(context, entity_id):
    """
    delete entities
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param entity_id: entity id name
    """
    __logger__.debug("Deleting entities...")
    context.resp_list = context.cb.delete_entities_by_id(context, entity_id)
    __logger__.info("...Entities are deleted")

