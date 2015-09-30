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

import subprocess

from iotqatools.helpers_utils import *

__logger__ = logging.getLogger("utils")


class Properties:
    """
    properties  management
    """

    def __init__(self):
        """
        constructor
        """

    def read_configuration_json(self):
        """
        return properties from configuration.josn file
        :return: dict
        """
        return read_file_to_json("configuration.json")

    def update_properties_json_file(self, file_name, sudo_run="true"):
        """
         update properties. json from script file in setting folders
        :param file_name: script file associated to a feature
        :param sudo_run:  with superuser privileges (True | False)
        """
        if sudo_run.lower() == "true":
            sudo_run = "sudo"
        else:
            sudo_run = ""
        configuration = self.read_configuration_json()
        __logger__.info("configuration.json: %s" % str(configuration))
        if configuration["UPDATE_PROPERTIES_JSON"].lower() == "true":
            __logger__.info(" >> config file used: '%s'", file_name)
            with open("%s/%s" % (configuration["PATH_TO_SETTINGS_FOLDER"], file_name)) as config_file:
                for line in config_file.readlines():
                    __logger__.info("-- properties.json lines: %s %s" % (sudo_run, str(line)))
                    p = subprocess.Popen("%s %s" % (sudo_run, str(line)), shell=True, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT)
                    stdout = p.stdout.readlines()
                    assert stdout == [], "ERROR - modifying config files from %s/%s in setting folder. \n " \
                                         "        %s" % (configuration["PATH_TO_SETTINGS_FOLDER"], file_name, stdout)
            __logger__.info(" >> properties.json is created or updated")
        else:
            __logger__.info("properties.json is not updated")

    def read_properties(self):
        """
        Parse the properties used in project located in the acceptance folder
        :return: properties dict
        """
        self.config = read_file_to_json("properties.json")
        return self.config

    def update_context_broker_file(self, fabric):
        """
        updating /etc/sysconfig/contextBroker
        note: if values have "/" chars are replaced by "\/". Ex: BROKER_LOG_DIR variable
        """
        fabric.run("sed -i 's/%s.*/%s=%s/' /etc/sysconfig/contextBroker" % ("BROKER_USER", "BROKER_USER", self.config["context_broker_env"]["CB_LOG_OWNER"]), sudo=True)
        fabric.run("sed -i 's/%s.*/%s=%s/' /etc/sysconfig/contextBroker" % ("BROKER_PORT", "BROKER_PORT", self.config["context_broker_env"]["CB_PORT"]), sudo=True)
        fabric.run("sed -i 's/%s.*/%s=%s/' /etc/sysconfig/contextBroker" % ("BROKER_LOG_DIR", "BROKER_LOG_DIR", self.config["context_broker_env"]["CB_LOG_FILE"].replace("/", "\/")), sudo=True)
        fabric.run("sed -i 's/%s.*/%s=%s/' /etc/sysconfig/contextBroker" % ("BROKER_DATABASE_HOST", "BROKER_DATABASE_HOST", self.config["mongo_env"]["MONGO_HOST"]), sudo=True)
        fabric.run("sed -i 's/%s.*/%s=%s/' /etc/sysconfig/contextBroker" % ("BROKER_DATABASE_NAME", "BROKER_DATABASE_NAME", self.config["mongo_env"]["MONGO_DATABASE"]), sudo=True)
        fabric.run("sed -i 's/%s.*/%s=%s/' /etc/sysconfig/contextBroker" % ("BROKER_EXTRA_OPS", "BROKER_EXTRA_OPS", self.config["context_broker_env"]["CB_EXTRA_OPS"]), sudo=True)
