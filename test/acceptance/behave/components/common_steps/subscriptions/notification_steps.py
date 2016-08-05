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


__author__ = 'Ivan Arias Leon (ivan dot ariasleon at telefonica dot com)'

import behave
import logging
from behave import step
import requests

from tools.notification_listener import Daemon


behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")

# constants
NOTIFICATION_ENDPOINT = u'http://localhost:%s'

@step(u'start the subscription listener as a daemon using the port "([^"]*)"')
def start_the_subscription_listener_as_a_daemon(context, port):
    """
    start the subscription listener as a daemon
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param port: port used to start the notification listener.
    """
    __logger__.info("Starting the notification listener using the port: %s" % port)
    context.notification_endpoint = NOTIFICATION_ENDPOINT % port
    __logger__.info("notification endpoint: %s" % context.notification_endpoint)
    d = Daemon(port=port, verbose=False)
    assert d.is_alive_and_is_a_daemon(), "ERROR - the notification listener is not alive or it is not a daemon in the port: %s..." % port


@step(u'get notification sent to listener')
def get_notification_sent_to_listener(context):
    """
    get notification sent to listener
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    url = "%s/last_notification" % context.notification_endpoint
    __logger__.debug("url: %s" % url)
    context.resp = requests.get(url)
    __logger__.debug("notification received:")
    __logger__.debug("   headers")
    for h in context.resp.headers:
        __logger__.debug("     %s: %s" % (h, context.resp.headers[h]))
    __logger__.debug("   payload: %s" % context.resp.text)


# ----  verifications ------
@step(u'verify subscription listener is started successfully')
def verify_subscription_listener_is_started_successfully(context):
    """
    verify subscription listener is started successfully
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    url = "%s/reset" % context.notification_endpoint
    __logger__.debug("url: GET %s" % url)
    try:
        r = requests.get(url).text
        assert r.find("without notification received") >= 0, " ERROR - the notification listener is not started correctly"
    except Exception, e:
        __logger__.error("NOTIFICATION_LISTENER: %s" % str(e))
    __logger__.info("The notification listener is verified that it is running correctly")




