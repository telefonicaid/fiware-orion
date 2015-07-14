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

import logging

from fabric.api import env, run, get
from fabric.context_managers import hide, cd
from fabric.operations import sudo, local, put
from StringIO import StringIO


#constants
EMPTY             = u''
HOST              = u'host'
HOST_DEFAULT      = u'localhost'
PORT              = u'port'
PORT_DEFAULT      = u'22'
USER              = u'user'
USER_DEFAULT      = u'root'
PASSWORD          = u'password'
CERT_FILE         = u'cert_file'
PATH              = u'path'
RETRY             = u'retry'
RETRY_DEFAULT     = u'1'
SUDO              = u'sudo'
MODE              = u'mode'
COMMAND           = u'command'
HIDE              = u'hide'

__logger__ = logging.getLogger("utils")

class FabricSupport:
    """
    manage fabric tools
    """

    def __init__ (self, **kwargs):
        """
        constructor
        :param host: hostname or IP to connect
        :param port: specify a default port used to connect
        :param hide: show message or not (True or False)
        :param user: implicit user used to connect
        :param password: passwords associated to user
        :param cert_file: certificate file associated to user
        :param retry: number of retries in case of error
        :param sudo: with superuser privileges (True | False)
        """
        self.host                = kwargs.get(HOST,HOST_DEFAULT)
        self.port                = kwargs.get(PORT,PORT_DEFAULT)
        self.hide                = kwargs.get(HIDE, True)
        env.host_string          = "%s:%s" % (self.host, self.port)
        env.user                 = kwargs.get(USER,USER_DEFAULT)
        env.password             = kwargs.get(PASSWORD,EMPTY)
        env.key_filename         = kwargs.get(CERT_FILE,EMPTY)
        env.connection_attempts  = kwargs.get(RETRY,RETRY_DEFAULT)
        env.cwd                  = kwargs.get(PATH, "")
        self.sudo                = kwargs.get(SUDO, False)
        # if host is localhost use Process instead of Fabric. see run method
        if self.host.lower() == "localhost" or self.host == "127.0.0.1":
            self.LOCALHOST = True
        else:
            self.LOCALHOST = False

    def warn_only(self, value):
        """
        Boolean setting determining whether Fabric exits when detecting errors on the remote end
        :param value: ( True |False )
        """
        env.warn_only = value

    def __sub_run (self, command, path, sudo_run):
        """
        run a command independently that the output message by console is displayed or not
        :param command: command o script to execute in remote
        :param path: path where execute the command
        :param sudo_run: with superuser privileges (True | False)
        """
        with cd(path):
            if self.LOCALHOST:
                return local(command)
            elif sudo_run:
                return sudo(command)
            else:
                return run(command)

    def run(self, command, **kwargs):
        """
        run a command or script in remote host, but if host is localhost use Process instead of Fabric
        :param command: command o script to execute in remote
        :param path: path where execute the command
        :param sudo: with superuser privileges (True | False)
        :param hide: show message or not (True or False)
        """
        path     = kwargs.get(PATH, env.cwd)
        sudo_run = kwargs.get(SUDO, self.sudo)
        hide_msg = kwargs.get(HIDE, self.hide)
        try:
            if hide_msg:
                with hide('running', 'stdout', 'stderr'):
                    return self.__sub_run(command, path, sudo_run)
            else:
                return self.__sub_run(command, path, sudo_run)
        except Exception, e:
            assert False, "ERROR  - running the command \"%s\" remotely with Fabric \n       - %s" % (command, str (e))

    def runs(self, ops_list, **kwargs):
        """
        run several commands in a list with its path associated
        :param ops_list: list of commands with its current path associated and if it is necessary of superuser privilege (dictionary).
                         ex:{"command": "ls", "path": "/tmp", "sudo": False}
        :param hide: show message or not (True or False)
        """
        hide_msg = kwargs.get(HIDE, self.hide)
        for op in ops_list:
            self.run(op[COMMAND], path=op[PATH], sudo=op[SUDO], hide=hide_msg)

    def current_directory (self, directory):
        """
        change current directory
        :param directory: directory path
        """
        env.cwd = directory

    def put_file_to_remote(self, file_name, target_path, **kwargs):
        """
        Upload one or more files to a remote host from local host
        :param file_name: path and files to be copied into server
        :param target_path: path where will be put the file
        :param hide_run: show message or not (True or False)
        :param use_sudo:  superuser privileges (True | False)
        :param mode: to specify an exact mode (chmod)
        """
        hide_run = kwargs.get(HIDE, self.hide)
        sudo_run  = kwargs.get(SUDO, self.sudo)
        mode  = kwargs.get(MODE, None)
        if hide_run:
            with hide('running', 'stdout', 'stderr'):
                put(local_path=file_name, remote_path=target_path, use_sudo=sudo_run, mode=mode)

    def __sub_read_file(self, file, sudo_run):
        """
        read a file independently that the output message by console is displayed or not
        :param file: file name to read
        :param sudo_run: with superuser privileges (True | False)
        """
        if self.LOCALHOST:
            with open(file) as config_file:
                return config_file.readlines()
        else:
            fd = StringIO()
            get(file, use_sudo=sudo_run)
            get(file, fd)
            return fd.getvalue()

    def read_file(self, file, **kwargs):
        """
        read a file remotely
        :param file: file name to read
        :param path: path where the file is read
        :param hide: show message or not (True or False)
        :param sudo: with superuser privileges (True | False)
        """
        env.cwd   = kwargs.get(PATH, env.cwd)
        hide_run = kwargs.get(HIDE, self.hide)
        sudo_run  = kwargs.get(SUDO, self.sudo)
        try:
            if hide_run:
                with hide('running', 'stdout', 'stderr'):
                    return self.__sub_read_file(file, sudo_run)
            else:
                return self.__sub_read_file(file, sudo_run)
        except Exception, e:
            assert False, "ERROR  -reading a File \"%s\" remotely with Fabric \n       - %s" % (file, str (e))

