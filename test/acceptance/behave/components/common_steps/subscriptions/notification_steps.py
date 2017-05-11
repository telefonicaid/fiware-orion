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
import time

__author__ = 'Ivan Arias Leon (ivan dot ariasleon at telefonica dot com)'

import behave
import logging
from behave import step
import requests

from tools.notification_listener import Daemon
from tools.NGSI_v2 import NGSI


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
    context.d = Daemon(port=port, verbose=False)

    assert context.d.is_alive_and_is_a_daemon(), "ERROR - the notification listener is not alive or it is not a daemon in the port: %s..." % port


@step(u'get notification sent to listener')
def get_notification_sent_to_listener(context):
    """
    get notification sent to listener
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    retry = 10
    delay_to_retry = 1
    url = "%s/last_notification" % context.notification_endpoint
    __logger__.debug("url: %s" % url)
    # if the notification is not received in the listener, it is applied a policy of retries
    for i in range(retry):
        context.resp = requests.get(url)
        if context.resp.text.find("without notification received") < 0:
            break
        time.sleep(delay_to_retry)
        __logger__.debug("number of retries (%s) to catch notification from the listener" % str(i+1))
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
        __logger__.debug("Response: %s" % r.text)
        assert r.find("without notification received") >= 0, " ERROR - the notification listener is not started correctly"
    except Exception, e:
        __logger__.error("NOTIFICATION_LISTENER: %s" % str(e))
    __logger__.info("The notification listener is verified that it is running correctly")


@step(u'verify that no notification is received')
def verify_that_no_notification_is_received(context):
    """
    verify that no notification is received
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("verifying if no notification is received...")
    assert context.resp.text.find(
        "without notification received") >= 0, " ERROR - some notification is received: \n %s" % context.resp.text
    __logger__.info("...verified that no notification is received...")


@step(u'verify the notification in "([^"]*)" format')
def verify_the_notification_in_a_given_format(context, notif_format):
    """
    verify the notification in a given format
    :param notif_format: format expected (normalized | keyValues | values | legacy)
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("verify the notification in \"%s\" format..." % notif_format)
    entity_context = context.cb.get_entity_context()
    subsc_context = context.cb.get_subscription_context()
    payload = context.resp.text
    headers = context.resp.headers
    ngsi = NGSI()
    ngsi.verify_notification(notif_format, payload, headers, entity_context, subsc_context)
    __logger__.debug("...verified the notification in \"%s\" format" % notif_format)


@step(u'verify the custom notification')
def verify_the_custom_notification(context):
    """
    verify the custom notification
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("verifying the notification received with custom template...")

    __logger__.debug("notification received")
    __logger__.debug("headers: \n   %s" % context.resp.headers )
    __logger__.debug("payload: \n   %s" % context.resp.text )

    entity_context = context.cb.get_entity_context()
    subsc_context = context.cb.get_subscription_context()
    payload = context.resp.text
    headers = context.resp.headers
    ngsi = NGSI()
    ngsi.verify_custom_notification(payload, headers, entity_context, subsc_context)
    __logger__.info("...verified the notification received with custom template")

@step(u'verify metadata in notification without special metadata')
@step(u'verify metadata in notification with "([^"]*)"')
def verify_metadata_in_notification(context, metadata_flags="*"):
    """
    verify metadata in notification
    :param metadata_flags: metadata notified
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.debug("verifying metadata in the notification (custom user or special)...")
    entity_context = context.cb.get_entity_context()
    subsc_context = context.cb.get_subscription_context()
    action_type = context.cb.get_action_type()
    previous_value = context.cb.get_previous_value()
    payload = context.resp.text
    ngsi = NGSI()
    ngsi.verify_metadata_notification(metadata_flags, payload, entity_context, subsc_context, action_type, previous_value)
    __logger__.info("...verified metadata in the notification")
