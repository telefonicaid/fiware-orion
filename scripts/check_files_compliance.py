#!/usr/bin/python
# -*- coding: latin-1 -*-
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

__author__ = 'fermin'

import os
import re
from sys import argv

header = []
header.append('\s*Copyright( \(c\))? 20[1|2][0|1|2|3|4|5|6|7|8|9] Telefonica Investigacion y Desarrollo, S.A.U$')
header.append('\s*$')
header.append('\s*This file is part of Orion Context Broker.$')
header.append('\s*$')
header.append('\s*Orion Context Broker is free software: you can redistribute it and/or$')
header.append('\s*modify it under the terms of the GNU Affero General Public License as$')
header.append('\s*published by the Free Software Foundation, either version 3 of the$')
header.append('\s*License, or \(at your option\) any later version.$')
header.append('\s*$')
header.append('\s*Orion Context Broker is distributed in the hope that it will be useful,$')
header.append('\s*but WITHOUT ANY WARRANTY; without even the implied warranty of$')
header.append('\s*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero$')
header.append('\s*General Public License for more details.$')
header.append('\s*$')
header.append('\s*You should have received a copy of the GNU Affero General Public License$')
header.append('\s*along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.$')
header.append('\s*$')
header.append('\s*For those usages not covered by this license please contact with$')
header.append('\s*iot_support at tid dot es$')

header2 = []
header2.append('\s*Copyright( \(c\))? 20[1|2][0|1|2|3|4|5|6|7|8|9] FIWARE Foundation e.V.$')
header2.append('\s*$')
header2.append('\s*This file is part of Orion-LD Context Broker.$')
header2.append('\s*$')
header2.append('\s*Orion-LD Context Broker is free software: you can redistribute it and/or$')
header2.append('\s*modify it under the terms of the GNU Affero General Public License as$')
header2.append('\s*published by the Free Software Foundation, either version 3 of the$')
header2.append('\s*License, or \(at your option\) any later version.$')
header2.append('\s*$')
header2.append('\s*Orion-LD Context Broker is distributed in the hope that it will be useful,$')
header2.append('\s*but WITHOUT ANY WARRANTY; without even the implied warranty of$')
header2.append('\s*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero$')
header2.append('\s*General Public License for more details.$')
header2.append('\s*$')
header2.append('\s*You should have received a copy of the GNU Affero General Public License$')
header2.append('\s*along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.$')
header2.append('\s*$')
header2.append('\s*For those usages not covered by this license please contact with$')
header2.append('\s*orionld at fiware dot org$')

verbose    = True
is_orionld = False


# check_file returns an error string in the case of error or empty string if everything goes ok
def check_file(file):
    # The license header doesn't necessarily starts in the first line, e.g. due to a #define in a .h file
    # or a hashbang (#!/usr/bin/python...). Thus, we locate the starting line and start the comparison from
    # that line

    searching_first_line = True
    with open(file) as f:
        for line in f:
            line = line.rstrip()
            if searching_first_line:
                if re.search(header[0], line):
                    searching_first_line = False
                    i = 1
            else:
                if not re.search(header[i], line):
                    return 'mismatch: header <' + header[i] + '> : line <' + line + '>'
                i += 1
                if i == len(header):
                    # We have reach the end of the header, so the complete check passes
                    return ''

    # We reach this point if the first header line was not found or if we reach the end of the file before
    # reaching the end of the header. Both cases means false
    return 'end of file reached without finding Orion Copyright header'


# check_file_orionld returns an error string in the case of error or empty string if everything goes ok
def check_file_orionld(file):
    # The license header doesn't necessarily start on the first line, e.g. due to a #define in a .h file
    # or a hashbang (#!/usr/bin/python...). Thus, we locate the starting line and start the comparison from
    # that line

    searching_first_line = True
    with open(file) as f:
        for line in f:
            line = line.rstrip()
            if searching_first_line:
                if re.search(header2[0], line):
                    searching_first_line = False
                    i = 1
            else:
                if not re.search(header2[i], line):
                    return 'mismatch: HEADER <' + header2[i] + '> : line <' + line + '>'
                i += 1
                if i == len(header2):
                    # We have reach the end of the header, so the complete check passes
                    return ''

    # We reach this point if the first header line was not found or if we reach the end of the file before
    # reaching the end of the header. Both cases means false
    return 'end of file reached without finding Orion-LD Copyright header'


def ignore(root, file):
    # Files in the BUILD_* or .git directories are not processed
    if 'BUILD_' in root or '.git' in root:
        return True

    if 'ldcontext' in root:
        return True

    # PNG files in manuals o functionalTest are ignored
    if ('manuals' in root or 'functionalTest' in root or 'apiary' in root) and file.endswith('.png'):
        return True

    # Files in the rpm/SRPMS, rpm/SOURCES or rpm/RPMS directories are not processed
    if 'SRPMS' in root or 'SOURCES' in root or 'RPMS' in root:
        return True

    # Files in the test/valgrind directory ending with .out are not processed
    if 'valgrind' in root and file.endswith('.out'):
        return True

    # XML and JSON files in test/manual are not processed
    if 'manual' in root and (file.endswith('.json') or file.endswith('.xml')):
        return True

    # XML and JSON files in test/unittest/testData are not processed
    if 'testData' in root and (file.endswith('.json') or file.endswith('.xml')):
        return True

    # XML and JSON files in test/manual are not processed
    if 'heavyTest' in root and (file.endswith('.json') or file.endswith('.xml')):
        return True

    # Some files in docker/ directory are not processed
    if 'docker' in root and file in ['Dockerfile', 'Dockerfile-base', 'Dockerfile-test', 'Dockerfile-debug', 'Dockerfile-gdb', 'gdbinit', 'docker-compose.yml']:
        return True

    # Some files in test/acceptance/behave directory are not processed
    if 'behave' in root and file in ['behave.ini', 'logging.ini', 'properties.json.base']:
        return True

    # Files used by the Qt IDE (they start with contextBroker.*) are not processed
    if file.endswith('.creator') or file.endswith('.creator.user') or file.endswith('.config') \
            or file.endswith('.files') or file.endswith('.includes'):
        return True

    # Files used by the PyCharm IDE (in the .idea/ directory) are not processed
    if '.idea' in root:
        return True

    # Apib files have an "inline" license, so they are ignored
    extensions_to_ignore = [ 'apib', 'md' ]
    if os.path.splitext(file)[1][1:] in extensions_to_ignore:
        return True

    # JMX files in test/jMeter are ignored
    if 'jMeter' in root and (file.endswith('.jmx') or file.endswith('.jmeter.json')):
        return True

    # JSON files in test/jMeter/cases are ignored
    if 'cases' in root and file.endswith('.json'):
        return True

    # Database migration files in /databases are ignored
    if 'database' in root:
        return True

    # Particular cases of files that are also ignored
    files_names = ['.gitignore', '.valgrindrc', '.valgrindSuppressions', 'LICENSE',
                   'ContributionPolicy.txt', 'CHANGES_NEXT_RELEASE', 'compileInfo.h',
                   'unittests_that_fail_sporadically.txt', 'Vagrantfile', 'contextBroker.ubuntu', 'orionld.ubuntu',
                   'mkdocs.yml', 'fiware-ngsiv2-reference.errata', 'ServiceRoutines.txt', '.travis.yml',
                   '.dockerignore', '.jmeter.json']
    if file in files_names:
        return True
    if 'scripts' in root and (file == 'cpplint.py' or file == 'pdi-pep8.py' or file == 'uncrustify.cfg' \
                                      or file == 'cmake2junit.xsl'):
        return True
    if 'acceptance' in root and (file.endswith('.txt') or file.endswith('.json')):
        return True

    return False


def supported_extension(root, file):
    """
    Check if the file is supported depending of the name, the extension of the name inside a path
    :param root:
    :param file:
    :return:
    """
    extensions = ['py', 'cpp', 'c', 'h', 'xml', 'json', 'test', 'vtest', 'txt', 'sh', 'spec', 'cfg', 'DISABLED', 'xtest',
                  'centos', 'js', 'jmx', 'vtestx', 'feature', 'go', 'jsonld', 'supp' ]
    names = ['makefile', 'Makefile', 'CMakeLists.txt.orion', 'CMakeLists.txt.orionld' ]

    # Check extensions
    if os.path.splitext(file)[1][1:] in extensions:
        return True

    # Check filenames
    if file in names:
        return True

    # Check a filename in a root
    if 'config' in root and file == 'contextBroker':
        return True

    if 'config' in root and file == 'orionld':
        return True
    
    filename = os.path.join(root, file)
    print 'not supported extension: {filename}'.format(filename=filename)
    return False

if len(argv) > 1:
    dir = argv[1]
else:
    print 'Usage:   ./check_files_compliance.py <directory>'
    exit(1)

good = 0
bad = 0

for root, dirs, files in os.walk(dir):
    for file in [f for f in files]:
        # DEBUG
        # print(os.path.join(root, file))

        # Only process files that match a given pattern
        if ignore(root, file):
            continue

        # Check that the extension is supported
        if not supported_extension(root, file):
            bad += 1
            continue

        error = ''
        filename = os.path.join(root, file)

        if os.path.islink(filename):
            continue

        if 'src/app/orionld/' in filename:
            is_orionld = True
        elif 'src/lib/orionld/' in filename:
            is_orionld = True
        elif 'src/app/ssClient/' in filename:
            is_orionld = True
        elif 'test/functionalTest/cases/0000_ngsild' in filename:
            is_orionld = True
        elif 'test/functionalTest/cases/0000_troe' in filename:
            is_orionld = True
        elif 'test/unittests/orionld' in filename:
            is_orionld = True
        elif 'archive/temporal' in filename:
            is_orionld = True
        elif 'test/stress' in filename:
            is_orionld = True
        else:
            is_orionld = False

        if is_orionld:
            error = check_file_orionld(filename)
        else:
            error = check_file(filename)

        if len(error) > 0:
            print filename + ': ' + error
            bad += 1
        else:
            good += 1

# src/lib/orionld/
# src/app/orionld
print '--------------'
print 'Summary:'
print '   good:    {good}'.format(good=str(good))
print '   bad:     {bad}'.format(bad=str(bad))
print 'Total: {total}'.format(total=str(good + bad))

if bad > 0:
    exit(1)
else:
    exit(0)
