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


import sys
import os
import subprocess

DIR = u'.'
TAGS = u''
SKIP = u''    # It is not necessary to define the "steps" folder
ONLY = False
temp_list = []
command = u'behave '


def __usage():
    """
    usage message
    """
    print " *****************************************************************************************************"
    print " * This script execute all .feature files in a given directory and its subdirectories.               *"
    print " *  usage:                                                                                           *"
    print " *     python behave_all.sh [-u] [-only] [-path==directory_path] [-tags==tags_list] [-skip==folders] *"
    print " *                                                                                                   *"
    print " *  parameters:                                                                                      *"
    print " *     -u: show this usage [optional].                                                               *"
    print " *     -path: find .feature files in the directory and subdirectory (default: .) [optional].         *"
    print " *     -tags: use tags defined (default: nothing) [optional].                                        *"
    print " *     -skip: list of directories skipped, separated by comma (default: steps) [optional].           *"
    print " *     -only: The features are not executed only returns the command line. [optional].               *"
    print " *                                                                                                   *"
    print " *  example:                                                                                         *"
    print " *      python behave_all -path==components/ngsi -tag==-t @skip -skip==types,entities -only          *"
    print " *                                                                                                   *"
    print " * Note: If the folder has not a file with .feature extension, this folder is skipped                *"
    print " *       automatically.                                                                              *"
    print " *                                                                                                   *"
    print " *****************************************************************************************************"
    exit(0)


def configuration(arguments):
    """
    Define values for configuration
    :param arguments: parameters in command line
    """
    global DIR, TAGS, SKIP, ONLY
    error_msg = ""
    for i in range(len(arguments)):
        try:
            if arguments[i].find('-u') >= 0:
                __usage()
            elif arguments[i].find('-path') >= 0:
                error_msg = u'(-path = %s)' % arguments[i]
                DIR = str(arguments[i]).split("==")[1]
            elif arguments[i].find('-tags') >= 0:
                error_msg = u'(-tags = %s)' % arguments[i]
                TAGS = str(arguments[i]).split("==")[1]
            elif arguments[i].find('-skip') >= 0:
                error_msg = u'(-skip = %s)' % arguments[i]
                SKIP = str(arguments[i]).split("==")[1]
            elif arguments[i].find('-only') >= 0:
                ONLY = True
        except Exception, e:
            raise Exception("ERROR - in %s parameter. See -u parameter. \n   - %s" % (error_msg, e))
            exit(0)


def exists_file_with_extension(ext, folder):
    """
    verify if exists any file with extension in folder
    :param ext:
    :param folder
    :return: boolean
    """
    for file in os.listdir(folder):
     if file.endswith(".%s" % ext):
        return True
    return False

def scan_dir(dir, temp_list, skip_folder=u''):
    """
    scan all directories except skipped
    :param dir: directory path
    :param: temp_list: list of subdirectories
    :param skip_folder: folders to skip, the "steps" folder always is skipped
    :return:
    """
    try:
        skip_folder_list = []
        skip_folder_list.append(u'steps')
        if skip_folder != u'':
            for item in skip_folder.split(","):
                skip_folder_list.append(item)
    except Exception, e:
       raise Exception("ERROR - skip folder is incorrect format. See -u parameter. \n   - %s " % e)
       exit(0)
    try:
        if exists_file_with_extension("feature", dir):
            temp_list.append("%s/*.feature" % dir)
        for name in os.listdir(dir):
            skipped = False
            path = os.path.join(dir, name)
            if (os.path.isdir(path)):
                for skip in skip_folder_list:         # list of folders skipped, separated by comma
                    if path.find(skip) >= 0:
                        skipped = True
                    if not exists_file_with_extension("feature", path):      #verify if an extension exists in folder
                        skipped = True
                if not skipped:
                    temp_list.append("%s/*.feature" % path)
                    scan_dir(path, temp_list, skip_folder)
    except Exception, e:
        raise Exception("ERROR - Directory not found. See -u parameter. \n   - %s" %e)
        exit(0)



if __name__ == '__main__':
    configuration(sys.argv)
    scan_dir(DIR, temp_list, SKIP)
    for item in list(set(temp_list)):  # delete paths duplicated
        command = "%s %s" % (command, item)
    command = "%s  %s" % (command, TAGS)
    print command

    if not ONLY:
        p = subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
        output, err = p.communicate()
        print ""
        print "*************************************************\n" \
              "*** Running behave with all features in: %s \n" \
              "*************************************************\n %s" % (DIR, output)






