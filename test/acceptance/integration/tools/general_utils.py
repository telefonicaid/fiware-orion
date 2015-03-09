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
import json


__author__ = 'Jon Calderin Goñi (jon.caldering@gmail.com)'

import platform
import psutil
import pymongo
from lettuce import world
import os
import subprocess
from fabric.api import env, output
from fabric import api
from fabric.contrib.files import exists



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
    """
    Drop a specific database in a MongoDB that has the "acceptance" prefix in its name
    :param ip:
    :param port:
    :param database:
    :return:
    """
    if database == "":
        pymongo.Connection(ip, port).drop_database('acceptance')
    else:
        pymongo.Connection(ip, port).drop_database('acceptance-{service}'.format(service=database))


def drop_all_test_databases(ip, port):
    """
    Drop all databases in a MongoDB that have the "acceptance" prefix in its name
    :param ip:
    :param port:
    :return:
    """
    db = pymongo.Connection(ip, port)
    for db_name in db.database_names():
        if db_name.find('acceptance') >= 0:
            world.log.debug("Droping database: {db_name}".format(db_name=db_name))
            db.drop_database(db_name)


def raise_property_bad_configuration(section):
    raise ValueError(
        'The {section} section in the properties.json is bad configured: {section_conf} \n Configuration: {config}'.format(
            section=section, section_conf=world.config[section], config=world.config))


def check_properties():
    """
    Funct that check if the properties is set
    :return:
    """
    try:
        # Environment
        checking = world.config['environment']
        if checking['name'] == "" or checking['logs_path'] == "" or checking['log_level'] == "":
            raise_property_bad_configuration('environment')
        # Context Broker
        checking = world.config['context_broker']
        if checking['host'] == "" or checking['port'] == "":
            raise_property_bad_configuration('context_broker')
        else:
            try:
                int(checking['port'])
            except ValueError:
                world.log.error('The port has to be a string with numbers, but is {port}'.format(port=checking['port']))
                raise_property_bad_configuration('context_broker')
        # Mock
        checking = world.config['mock']
        if checking['port'] == "" or checking['bind_ip'] == "":
            raise_property_bad_configuration('mock')
        else:
            try:
                int(checking['port'])
            except ValueError:
                world.log.error('The port has to be a string with numbers, but is {port}'.format(port=checking['port']))
                raise_property_bad_configuration('mock')
        # Mongo
        checking = world.config['mock']
        if checking['host'] == "" and checking['port'] == "":
            raise_property_bad_configuration('mongo')
        else:
            try:
                int(checking['port'])
            except ValueError:
                world.log.error('The port has to be a string with numbers, but is {port}'.format(port=checking['port']))
                raise_property_bad_configuration('mongo')
        # Deploy data
        checking = world.config['deploy_data']
        if checking['host'] == "":
            raise_property_bad_configuration('deploy_data')
        else:
            if checking['host'] != 'localhost' and checking['host'] != '127.0.0.1':
                empty_attrs = ['ssh_port', 'user', 'password', 'bin_path']
                for attr in empty_attrs:
                    if checking[attr] == "":
                        raise_property_bad_configuration('deploy_data')
                try:
                    int(checking['ssh_port'])
                except ValueError:
                    world.log.error('The port has to be a string with numbers, but is {port}'.format(port=checking['ssh_port']))
                    raise_property_bad_configuration('deploy_data')
    except KeyError as e:
        world.log.error("There is a property bad configured/set: properties: {config}".format(config=world.config))
        raise e


def set_ssh_config(localhost):
    """
    Config fabric parms
    :return:
    """
    world.log.info('Setting context broker ssh config')
    output['stdout'] = False
    output['running'] = False
    output['warnings'] = False
    try:
        config = world.config['deploy_data']
        if not localhost:
            # Set ssh connection info
            env.host_string = '{ip}:{ssh_port}'.format(ip=config['host'], ssh_port=config['ssh_port'])
            env.user = config['user']
            env.password = config['password']
            env.sudo_password = config['password']
            env.warn_only = True
    except KeyError as e:
        world.log.error('set_ssh_config: In the properties file there is not definition about the deploy information. Config: {config}\n'.format(
            config=world.config))
        raise e


def get_cb_pid():
    """
    Get the cb pid, if its running
    :return:
    """
    world.log.info('Getting Context Broker pid')
    config = world.config['deploy_data']
    if not config['host'] == 'localhost' or not config['host'] == '127.0.0.1':
        localhost = False
        runner = 'run'
    else:
        runner = 'local'
        localhost = True
    set_ssh_config(localhost)
    if config['pid_file'] == '':
        pid_path = '/tmp/acceptance/contextBroker.pid'
    else:
        pid_path = config['pid_file']
    if exists(pid_path):
        pid_number = getattr(api, runner)('cat {pid_path}'.format(pid_path=pid_path))
        cmd_line = getattr(api, runner)('cat /proc/{pid_number}/cmdline'.format(pid_number=pid_number))
        if cmd_line == 'contextBroker':
            if getattr(api, runner)('ps -p {pid_number} | grep {bin_path}'.format(pid_number=pid_number, bin_path=config['bin_path'])) != '':
                return pid_number
            else:
                return getattr(api, runner)("ps -ef | grep {bin_path} | grep -v grep | awk '{{print $2}}'".format(bin_path=config['bin_path']))
        else:
            return getattr(api, runner)("ps -ef | grep {bin_path} | grep -v grep | awk '{{print $2}}'".format(bin_path=config['bin_path']))
    else:
        return getattr(api, runner)("ps -ef | grep {bin_path} | grep -v grep | awk '{{print $2}}'".format(bin_path=config['bin_path']))


def start_cb(parms):
    """
    Start the Context Broker in the machine set in the properties file
    :param parms:
    :return:
    """
    world.log.info('Starting cb')
    config = world.config['deploy_data']
    if not config['host'] == 'localhost' or not config['host'] == '127.0.0.1':
        localhost = False
        runner = 'run'
        world.log.debug('Context Broker in remote')
    else:
        runner = 'local'
        localhost = True
        world.log.debug('Context Broker in local')
    set_ssh_config(localhost)
    if world.cb_pid != '':
        stop_cb()
    if config['log_path'] == '':
        log_path = '/tmp/acceptance'
    else:
        log_path = config['log_path']
    world.log.debug('Log path set to: {log_path}'.format(log_path=log_path))
    getattr(api, runner)('mkdir -p {log_path}'.format(log_path=log_path))
    if config['pid_file'] == '':
        pid_path = '/tmp/acceptance/contextBroker.pid'
    else:
        pid_path = config['pid_file']
    world.log.debug('Pid path set to: {pid_path}'.format(pid_path=pid_path))
    getattr(api, runner)('mkdir -p {pid_dir}'.format(pid_dir=pid_path[:pid_path.rfind('/')]))
    command = '{bin_path} {parms} -logDir {log_path} -pidpath {pid_path}'.\
        format(bin_path=config['bin_path'], parms=parms, log_path=log_path, pid_path=pid_path)
    resp = getattr(api, runner)(command)
    world.log.debug('The response of the starting Context Broker command is: {response}'.format(response=resp))
    world.cb_pid = get_cb_pid()


def stop_cb():
    """
    Stop a cb instance with the pid in world.cb_pid
    :return:
    """
    world.log.info('Stopping Context Broker')
    config = world.config['deploy_data']
    if not config['host'] == 'localhost' or not config['host'] == '127.0.0.1':
        localhost = False
        runner = 'run'
    else:
        runner = 'local'
        localhost = True
    set_ssh_config(localhost)
    # Check if there is CB running
    if get_cb_pid() != '':
        getattr(api, runner)('kill -15 {pid} && sleep 5'.format(pid=get_cb_pid()))
        if get_cb_pid() != '':
            getattr(api, runner)('kill -9 {pid} && sleep 5'.format(pid=get_cb_pid()))
            if get_cb_pid() != '':
                raise EnvironmentError('After try to kill the Context Broker process, is still running, kill it manually')
    world.cb_pid = get_cb_pid() # It should be '' (empty)


def pretty(json_pret):
    return json.dumps(json_pret, sort_keys=True, indent=4, separators=(',', ': '))





