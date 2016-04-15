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


from components.common_steps.general_steps import *
from iotqatools.fabric_utils import FabricSupport
from iotqatools.mongo_utils import Mongo
from iotqatools.cb_v2_utils import CB
from iotqatools.helpers_utils import *

from tools.properties_config import Properties  # methods in properties class

# constants
CONTEXT_BROKER_ENV = u'context_broker_env'
MONGO_ENV = u'mongo_env'
properties_class = Properties()

behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")


@step(u'update properties test file from "([^"]*)" and sudo local "([^"]*)"')
def update_properties_file(context, properties_file, sudo_run):
    """
    update properties.py file from setting folder or jenkins console
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param properties_file: file to get data fron setting folder to update properties.json
    """
    __logger__.debug("Updating properties.json file...")
    properties_class.update_properties_json_file(properties_file, sudo_run)
    __logger__.debug("...Updated properties.json file")


@step(u'update contextBroker config file')
def update_context_broker_config_file_and_restart_service(context):
    """
    updating /etc/sysconfig/contextBroker file
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    global props_cb, props_mongo
    props_cb = properties_class.read_properties()[CONTEXT_BROKER_ENV]       # context broker properties dict
    __logger__.debug(" Context Broker parameters:")
    props_mongo = properties_class.read_properties()[MONGO_ENV]             # mongo properties dict
    for param in props_cb:
        __logger__.debug("   %s: %s" % (param, props_cb[param]))
    __logger__.debug(" Mongo parameters:")
    for param in props_mongo:
        __logger__.debug("   %s: %s" % (param, props_mongo[param]))
    context.my_fab = FabricSupport(host=props_cb["CB_HOST"], user=props_cb["CB_FABRIC_USER"],
                           password=props_cb["CB_FABRIC_PASS"], cert_file=props_cb["CB_FABRIC_CERT"],
                           retry=props_cb["CB_FABRIC_RETRY"], hide=True, sudo=props_cb["CB_FABRIC_SUDO"])
    context.configuration = properties_class.read_configuration_json()
    __logger__.debug("CB_RUNNING_MODE: %s" % context.configuration["CB_RUNNING_MODE"])
    if context.configuration["CB_RUNNING_MODE"].upper() == "RPM":
        __logger__.debug("Updating /etc/sysconfig/contextBroker file...")
        properties_class.update_context_broker_file(context.my_fab)
        __logger__.info("...Updated /etc/sysconfig/contextBroker file")


@step(u'start ContextBroker')
def start_context_broker(context):
    """
    start ContextBroker
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    global props_cb, props_mongo
    if context.configuration["CB_RUNNING_MODE"].upper() == "RPM":
        __logger__.debug("Starting contextBroker service...")
        context.my_fab.run("service contextBroker restart")
        __logger__.info("...Started contextBroker service")
    else:
        __logger__.debug("Starting contextBroker per command line interface...")
        props_cb["CB_EXTRA_OPS"] = props_cb["CB_EXTRA_OPS"].replace('"', "")
        # hint: the -harakiri option is used to kill contextBroker (must be compiled in DEBUG mode)
        command = "contextBroker -port %s -logDir %s -pidpath %s -dbhost %s -db %s %s -harakiri" %\
                  (props_cb["CB_PORT"], props_cb["CB_LOG_FILE"], props_cb["CB_PID_FILE"], props_mongo["MONGO_HOST"],
                   props_mongo["MONGO_DATABASE"], props_cb["CB_EXTRA_OPS"])
        __logger__.debug("command: %s" % command)
        resp = context.my_fab.run(command)
        __logger__.debug("output: %s" % repr(resp))
        __logger__.info("...Started contextBroker command line interface")


@step(u'stop ContextBroker')
def stop_Context_broker(context):
    """
    stop ContextBroker
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    global props_cb
    if context.configuration["CB_RUNNING_MODE"].upper() == "RPM":
        __logger__.debug("Stopping contextBroker service...")
        context.my_fab.run("service contextBroker stop")
        __logger__.info("...Stopped contextBroker service")
    else:
        __logger__.debug("Stopping contextBroker per harakiri...")
        cb = CB(protocol=props_cb["CB_PROTOCOL"], host=props_cb["CB_HOST"], port=props_cb["CB_PORT"])
        cb.harakiri()
        __logger__.info("...Stopped contextBroker per harakiri")


@step(u'verify contextBroker is installed successfully')
def verify_context_broker_is_installed_successfully(context):
    """
    verify contextBroker is started successfully
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    global props_cb
    __logger__.debug("Verifying if contextBroker is started successfully...")
    cb = CB(protocol=props_cb["CB_PROTOCOL"], host=props_cb["CB_HOST"], port=props_cb["CB_PORT"])
    c = 0
    while (not cb.is_cb_started()) and (int(props_cb["CB_RETRIES"]) > c):
        time.sleep(int(props_cb["CB_DELAY_TO_RETRY"]))
        c += 1
        __logger__.debug("WARN - Retry in verification if context broker is started. No: (%s)" % str(c))
    assert (props_cb["CB_RETRIES"]) > c, "ERROR - context Broker is not started after of %s verification retries" % str(c)
    if props_cb["CB_VERIFY_VERSION"].lower() == "true":
        context.execute_steps(u'Given send a version request')
        context.execute_steps(u'Given verify if version is the expected')
    else:
        __logger__.info("Current version is not verified...")
    __logger__.info("...Verified that contextBroker is started successfully")


@step(u'verify mongo is installed successfully')
def verify_mongo_is_installed_successfully(context):
    """
    verify contextBroker is installed successfully
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    global props_mongo
    __logger__.debug("Verifying if mongo is installed successfully...")
    mongo = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
                  password=props_mongo["MONGO_PASS"], version=props_mongo["MONGO_VERSION"],
                  verify_version=props_mongo["MONGO_VERIFY_VERSION"])
    mongo.connect()
    mongo.eval_version()
    mongo.disconnect()
    __logger__.info("...Verified that mongo is installed successfully")
