# -*- coding: utf-8 -*-
"""
# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
import platform
import psutil

__author__ = 'Jon Calderin Goñi (jon.caldering@gmail.com)'

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
    stderr_file = open('logs/mock_err.log', 'w')
    stdout_file = open('logs/mock_out.log', 'w')
    if platform.system() == 'Windows':
        command = ['python', '{path}\\mock.py'.format(path=path), '--host',
                   '{bind_ip}'.format(bind_ip=world.config['mock']['bind_ip']), '--port',
                   '{port}'.format(port=world.config['mock']['port'])]
    elif platform.system() == 'Linux':
        command = ['python', '{path}/mock.py'.format(path=path), '--host',
                   '{bind_ip}'.format(bind_ip=world.config['mock']['bind_ip']), '--port',
                   '{port}'.format(port=world.config['mock']['port'])]
    else:
        raise ValueError, 'The SO is not compatible with the mock'
    print command
    return subprocess.Popen(command, stderr=stderr_file, stdout=stdout_file)


def stop_mock():
    """
    Stop the mock
    :return:
    """
    if world.mock != None:
        if platform.system() == 'Windows':
            subprocess.Popen(['taskkill', '/F', '/T', '/PID', str(world.mock.pid)])
        elif platform.system() == 'Linux':
            kill(world.mock.pid)
        world.mock = None


def kill(proc_pid):
    """
    Funct to kill all process with his children
    :param proc_pid:
    :return:
    """
    process = psutil.Process(proc_pid)
    for proc in process.get_children(recursive=True):
        proc.kill()
    process.kill()


def drop_database(ip, port, database):
    if database == "":
        pymongo.Connection(ip, port).drop_database('acceptance')
    else:
        pymongo.Connection(ip, port).drop_database('acceptance-{service}'.format(service=database))


def drop_all_test_databases(ip, port):
    db = pymongo.Connection(ip, port)
    for db_name in db.database_names():
        if db_name.find('acceptance') >= 0:
            print "Droping database: {db_name}".format(db_name=db_name)
            db.drop_database(db_name)


