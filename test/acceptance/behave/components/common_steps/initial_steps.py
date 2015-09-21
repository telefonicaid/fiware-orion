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

from iotqatools.fabric_utils import FabricSupport
from iotqatools.mongo_utils import Mongo
from iotqatools.cb_v2_utils import CB
from iotqatools.helpers_utils import *

from tools.properties_config import Properties  # methods in properties class

behave.use_step_matcher("re")
__logger__ = logging.getLogger("steps")


@step(u'update properties test file from "([^"]*)" and sudo local "([^"]*)"')
def update_properties_file(context, properties_file, sudo_run):
    """
    update properties.py file from setting folder or jenkins console
    :param context:
    :param properties_file: file to get data to update properties.py
    """
    global properties_class
    properties_class = Properties()
    properties_class.update_properties_json_file(properties_file, sudo_run)


@step(u'update contextBroker config file and restart service')
def update_context_broker_config_file_and_restart_service(context):
    """
    updating /etc/sysconfig/contextBroker file an restarting service
    :param context:
    """
    global properties_class, props_cb, props_mongo, my_fab, configuration
    __logger__.debug(" >> updating /etc/sysconfig/contextBroker file")
    props = properties_class.read_properties()  # properties dict
    props_cb = props["context_broker_env"]  # context broker properties dict
    __logger__.debug(" Context Broker parameters:")
    for param in props_cb:
        __logger__.debug("   %s: %s" % (param, props_cb[param]))
    props_mongo = props["mongo_env"]  # mongo properties dict
    __logger__.debug(" Mongo parameters:")
    for param in props_mongo:
        __logger__.debug("   %s: %s" % (param, props_mongo[param]))

    my_fab = FabricSupport(host=props_cb["CB_HOST"], user=props_cb["CB_FABRIC_USER"],
                           password=props_cb["CB_FABRIC_PASS"], cert_file=props_cb["CB_FABRIC_CERT"],
                           retry=props_cb["CB_FABRIC_RETRY"], hide=True, sudo=props_cb["CB_FABRIC_SUDO"])
    configuration = properties_class.read_configuration_json()
    __logger__.debug("CB_RUNNING_MODE: %s" % configuration["CB_RUNNING_MODE"])
    if configuration["CB_RUNNING_MODE"].upper() == "RPM":
        properties_class.update_context_broker_file(my_fab)
        __logger__.info(" >> updated /etc/sysconfig/contextBroker file")
        __logger__.debug(" >> restarting contextBroker service")
        my_fab.run("service contextBroker restart")
        __logger__.info(" >> restarted contextBroker service")
    else:
        __logger__.debug(" >> restarting contextBroker per command line interface")
        # hint: the -harakiri option is used to kill contextBroker (must be compiled in DEBUG mode)
        __logger__.debug("contextBroker -port %s -logDir %s -pidpath /var/run/contextBroker/contextBroker.pid -dbhost %s -db %s %s -harakiri" %
            (props_cb["CB_PORT"], props_cb["CB_LOG_FILE"], props_mongo["MONGO_HOST"], props_mongo["MONGO_DATABASE"], props_cb["CB_EXTRA_OPS"]))
        resp = my_fab.run("contextBroker -port %s -logDir %s -pidpath /var/run/contextBroker/contextBroker.pid -dbhost %s -db %s %s -harakiri" %
            (props_cb["CB_PORT"], props_cb["CB_LOG_FILE"], props_mongo["MONGO_HOST"], props_mongo["MONGO_DATABASE"], props_cb["CB_EXTRA_OPS"]))
        __logger__.debug("output: %s" % str(resp))
        __logger__.info(" >> restarted contextBroker command line interface")


@step(u'stop service')
def stop_service(context):
    """
    stop ContextBroker service
    :param context:
    """
    global my_fab, configuration
    if configuration["CB_RUNNING_MODE"].upper() == "RPM":
        __logger__.debug("Stopping contextBroker service...")
        my_fab.run("service contextBroker stop")
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
    :param context:
    """
    global props_cb
    __logger__.debug(" >> verify if contextBroker is started successfully")
    cb = CB(protocol=props_cb["CB_PROTOCOL"], host=props_cb["CB_HOST"], port=props_cb["CB_PORT"])
    c = 0
    while (not cb.is_cb_started()) and (int(props_cb["CB_RETRIES"]) > c):
        time.sleep(int(props_cb["CB_DELAY_TO_RETRY"]))
        c += 1
        __logger__.debug("WARN - Retry in verification if context broker is started. No: (%s)" % str(c))
    assert (props_cb["CB_RETRIES"]) > c, "ERROR - context Broker is not started after of %s verification retries" % str(c)
    __logger__.info(" >> verified that contextBroker is started successfully")


@step(u'verify mongo is installed successfully')
def verify_mongo_is_installed_successfully(context):
    """
    verify contextBroker is installed successfully
    :param context:
    """
    global props_mongo
    __logger__.debug(" >> verify if mongo is installed successfully")
    m = Mongo(host=props_mongo["MONGO_HOST"], port=props_mongo["MONGO_PORT"], user=props_mongo["MONGO_USER"],
              password=props_mongo["MONGO_PASS"], version=props_mongo["MONGO_VERSION"],
              verify_version=props_mongo["MONGO_VERIFY_VERSION"])
    m.connect()
    m.eval_version()
    m.disconnect()
    __logger__.info(" >> verified that mongo is installed successfully")
