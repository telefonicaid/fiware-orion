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
import logging
from behave import step

from iotqatools.mongo_utils import Mongo

from tools.properties_config import Properties
from tools.NGSI_v2 import NGSI

# constants
MONGO_ENV = u'mongo_env'
LISTENER_ENV = u'listeners'

properties_class = Properties()

behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")


# ------------------ create subscriptions ------------------------------------------------

@step(u'properties to subscriptions')
def properties_to_subscriptions(context):
    """
    properties to subscription (previous step to create or update subcription request)
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    listener_endpoint = properties_class.read_properties()[LISTENER_ENV]["LISTENER_NOTIF"]  # listeners properties dict

    __logger__.info("Define the properties used in the subcription request")
    context.cb.properties_to_subcription(context, listener_endpoint)


@step(u'create a new subscription')
def create_a_new_subscription(context):
    """
    create an subscription with the properties loaded in the "properties to subcriptions" step
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Creating an new subscription...")
    context.resp = context.cb.create_subscription()
    __logger__.info("...Created an new subscription")

@step(u'create a new subscription in raw mode')
def create_a_new_subscription(context):
    """
    create an subscription with the properties loaded in the "properties to subcriptions" step in raw mode
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("Creating an new subscription in raw mode...")
    context.resp = context.cb.create_subscription_in_raw_mode()
    __logger__.info("...Created an new subscription in raw mode")


#  -------------------- verifications -----------------------------------------------------
@step(u'verify that the subscription is stored in mongo')
def verify_that_the_subscription_is_stored_in_mongo(context):
    """
    verify that the subscription is stored in mongo
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    props_mongo = properties_class.read_properties()[MONGO_ENV]  # mongo properties dict
    __logger__.debug(" >> verifying that subscription is stored in mongo")
    mongo = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
                  password=props_mongo["MONGO_PASS"])
    ngsi = NGSI()
    ngsi.verify_subscription_stored_in_mongo(mongo, context.cb.get_subscription_context(), context.cb.get_headers(), context.resp)
    __logger__.info(" >> verified that subscription is stored in mongo")





