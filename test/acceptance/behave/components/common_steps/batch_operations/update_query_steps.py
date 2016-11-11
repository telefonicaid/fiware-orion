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

import behave
from behave import step

from iotqatools.helpers_utils import *

behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")


# ------------------ create_entities ------------------------------------------------

@step(u'define a entity properties to update in a single batch operation')
def define_a_entity_to_update_in_a_single_batch_operation(context):
    """
    define a entity to update in a single batch operation (pre-step to be used in "update entities in a single batch operation")
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    entity_dict = {}
    __logger__.info("append new entities properties to update bach operation...")
    if context.table is not None:
        for row in context.table:
            entity_dict[row["parameter"]] = row["value"]
    context.cb.batch_op_entities_properties(entity_dict)

@step(u'update entities in a single batch operation "([^"]*)"')
def update_entities_in_a_single_batch_operation(context, op):
    """
    allows to create, update and/or delete several entities in a single batch operation
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param op: operations allowed (APPEND, APPEND_STRICT, UPDATE, DELETE)
    """
    __logger__.debug("updating entities in a single batch operation using %s..." % op)
    queries_parameters = {}
    if context.table is not None:
        for row in context.table:
            queries_parameters[row["parameter"]] = row["value"]

    context.resp = context.cb.batch_update(queries_parameters, op)
    __logger__.info("...updated entities in a single batch operation")
