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
# fermin at tid dot es

__author__ = 'fermin'

import os
import re
from sys import argv

header = []
header.append('[#|\*] +Copyright 201[3|4] Telefonica Investigacion y Desarrollo, S.A.U$')
header.append('[#|\*]$')
header.append('[#|\*] +This file is part of Orion Context Broker.$')
header.append('[#|\*]$')
header.append('[#|\*] +Orion Context Broker is free software: you can redistribute it and/or$')
header.append('[#|\*] +modify it under the terms of the GNU Affero General Public License as$')
header.append('[#|\*] +published by the Free Software Foundation, either version 3 of the$')
header.append('[#|\*] +License, or \(at your option\) any later version.$')
header.append('[#|\*]$')
header.append('[#|\*] +Orion Context Broker is distributed in the hope that it will be useful,$')
header.append('[#|\*] +but WITHOUT ANY WARRANTY; without even the implied warranty of$')
header.append('[#|\*] +MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero$')
header.append('[#|\*] +General Public License for more details.$')
header.append('[#|\*]$')
header.append('[#|\*] +You should have received a copy of the GNU Affero General Public License$')
header.append('[#|\*] +along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.$')
header.append('[#|\*]$')
header.append('[#|\*] +For those usages not covered by this license please contact with$')
header.append('[#|\*] +fermin at tid dot es$')

verbose = True

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
    return 'end of file reached without finding header beginning'

def ignore(root, file):

    # Files in the BUILD_* or .git directories are not processed
    if 'BUILD_' in root or '.git' in root:
        return True

    # Files in the rpm/SRPMS, rpm/SOURCES or rpm/RPMS directories are not processed
    if 'SRPMS' in root or 'SOURCES' in root or 'RPMS'in root:
        return True

    # Files in the test/valdring directory ending with .out are not processed
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

    # Files used by the Qt IDE (they start with contextBroker.*) are not processed
    if file.endswith('.creator') or file.endswith('.creator.user') or file.endswith('.config') \
       or file.endswith('.files') or file.endswith('.includes'):
        return True

    # Files used by the PyCharm IDE (in the .idea/ directory) are not processed
    if '.idea' in root:
        return True

    # Particular cases of files that are also ignored
    if file == '.gitignore' or file == '.valgrindrc' or file == '.valgrindSuppressions' \
        or file == 'README.md' or file == 'LICENSE' or file == 'ContributionPolicy.txt' \
        or file == 'CHANGES_NEXT_RELEASE' or file == 'compileInfo.h' or file == 'unittests_that_fail_sporadically.txt':
        return True
    if 'scripts' in root and (file == 'cpplint.py' or file == 'pdi-pep8.py' or file == 'uncrustify.cfg' \
        or file == 'cmake2junit.xsl'):
        return True

    return False

def supported_extension(root, file):
    # FIXME: there should be a smarter way of doing this instead of so long or statement :)
    if file.endswith('.py') or file.endswith('.cpp') or file.endswith('.h') or file.endswith('.xml')\
        or file.endswith('.json') or file.endswith('.test') or file.endswith('.vtest') or file.endswith('.txt')\
        or file.endswith('.sh') or file == 'makefile' or file == 'Makefile' or file.endswith('.spec') \
        or file.endswith('.cfg') or file.endswith('.DISABLED') or file.endswith('.xtest') or file.endswith('.centos') or file.endswith('.js'):
        return True

    if 'config' in root and file == 'contextBroker':
        return True

    filename = os.path.join(root, file)
    print 'not supported extension: ' + filename
    return False

if len(argv) > 1:
    dir = argv[1]
else:
    print 'Usage:   ./check_files_compliance.py <directory>'
    exit(1)

good = 0
bad = 0

for root,dirs,files in os.walk(dir):
    for file in [f for f in files]:
        # DEBUG
        #print(os.path.join(root, file))

        # Only process files that match a given pattern
        if ignore(root, file):
            continue

        # Check that the extension is supported
        if not supported_extension(root, file):
            bad +=1
            continue

        filename = os.path.join(root, file)
        error = check_file(filename)
        if len(error) > 0:
            print filename + ': ' + error
            bad += 1
        else:
            # DEBUG
            #print filename + ': OK'
            good += 1

print '--------------'
print 'Summary:'
print '   good:    ' + str(good)
print '   bad:     ' + str(bad)
print 'Total: ' + str(good + bad)

if bad > 0:
    exit(1)
else:
    exit(0)
