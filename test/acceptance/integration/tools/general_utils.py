# -*- coding: utf-8 -*-
"""
# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es
"""

__author__ = 'Jon Calderin Goñi (jcaldering@gmail.com)'

import pymongo
from lettuce import world
import os
import subprocess


def check_world_attribute_is_not_none(attributes):
    "Check if world has the attribute, and if its different of None"
    for attribute in attributes:
        if hasattr(world, attribute) and getattr(world, attribute) is None:
            raise AttributeError(
                'This step can not being executed unless the step that define \
                 world.{attribute} has not been executed'.format(attribute=attribute))


def check_key_value(dict_to_check, key, value, start=0):
    """
    Check recursively if in a dict exist in any level a dicto with a key and value
    :param dict_to_check:
    :param key:
    :param value:
    :param start:
    :return:
    """
    # The first iteration has to be a dict
    if start == 0:
        if type(dict_to_check) is not dict:
            return False
    # If the 'check' element is a list, try each list element
    if type(dict_to_check) is list:
        for list_element in dict_to_check:
            if check_key_value(list_element, key, value, 1):
                return True
    # If the element is a dict, search the key
    elif type(dict_to_check) is dict:
        # Search for the key, if its found, check the value, if there is no combination of key-value, return false
        for key_ in dict_to_check.keys():
            if key.lower() == key_.lower():
                # The combination is found
                if dict_to_check[key_] == value:
                    return True
                else:
                    # Transmit the result
                    if check_key_value(dict_to_check[key_], key, value, 1):
                        return True
            else:
                # Transmit the result
                if check_key_value(dict_to_check[key_], key, value, 1):
                    return True
        return False
    else:
        return False


def start_mock():
    """
    Start a mock
    :return:
    """
    path, fl = os.path.split(os.path.realpath(__file__))
    # if platform.system() == 'Windows':
    # path = path[0:path.rfind('\\')] + '\\mocks\\'
    DEVNULL = open(os.devnull, 'wb')
    command = ['python', '{path}\\mock_cb_utils.py'.format(path=path)]
    return subprocess.Popen(command, stdout=DEVNULL, stderr=DEVNULL).pid


def stop_mock():
    """
    Stop the mock
    :return:
    """
    if world.mock_pid != None:
        subprocess.Popen(['taskkill', '/F', '/T', '/PID', str(world.mock_pid)])
        world.mock_pid = None


def drop_database(ip, port, database):
    if database == "":
        pymongo.Connection(ip, port).drop_database('orion'.format(service=database))
    else:
        pymongo.Connection(ip, port).drop_database('orion-{service}'.format(service=database))

