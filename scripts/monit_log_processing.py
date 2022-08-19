#!/usr/bin/env python2
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

import re
from sys import argv

if len(argv) == 5:
    file = argv[1]
    process_name = argv[2]
    target_month = argv[3]
    target_day = argv[4]
else:
    print 'Wrong number of argument'
    print '   Usage:   ./monit_log_processing.py <log_file> <processname> <target_month> <target_day>'
    print '   Example: ./monit_log_processing.py /var/log/contextBroker/monitBROKER.log contextBroker Sep 10'
    exit(1)

last_error_msg = ''
last_error_date = ''
last_info_msg = ''
last_info_date = ''
n = 0

# Example lines (as reference for regular expression matching):
#
# [CEST Sep  5 12:28:50] error    : 'contextBroker' process is not running
# [CEST Sep  5 12:32:03] info     : 'contextBroker' 'contextBroker' total mem amount check succeeded [current total mem amount=2464kB]
#
# Note that both 'CEST' and 'CET' are used as timestamp prefix (the S stands for 'Summer'), thus the regular expressions in use
# match both alternatives

with open(file) as f:
    for line in f:

        # Replace multi-whitespaces with single spaces
        line = ' '.join(line.split())

        # Filter out lines which doesn't include the process name (typically 'contextBroker') or
        # doesn't include the target date
        if re.search('\'' + process_name + '\'', line) and re.search('CES?T\s+' + target_month + '\s+' + target_day + '\s+', line, flags=re.IGNORECASE):
            # Grep line information we need using a regular expression
            m = re.match('\[CES?T (.*)\] (.*) : \'.*\' (.*)', line)
            if m != None:

                # If the line includes the "error" mark, we store it in last_error
                # If the line includes the "info" mark, we store it in last_info
                # If the line includes the "debug" mark, we ignore it (debug lines are not giving important information)

                date = m.group(1)
                type = m.group(2)
                msg = m.group(3)

                if type == 'error':
                    last_error_msg = msg
                    last_error_date = date

                if type == 'info':
                    last_info_msg = msg
                    last_info_date = date

        # We identify a process restart when "trying to restart" appear in a info line. We use the last_error_msg
        # to identify the cause of the problem
        if re.search('trying to restart', last_info_msg):

            print last_info_date + ' restart caused by: ' + last_error_msg
            n += 1

            # The date of both info and causing error messages should be the same. Otherwise we mark this as a
            # warning
            if last_info_date != last_error_date:
                print '   warning: info date <' + last_info_date + '> and error date <' + last_error_date + '> differ'

            # We clear last_info_msg to avoid being processed twice
            last_info_msg = ''

print '------------'
print 'Total number of restarts in the analyzed period: ' + str(n)
