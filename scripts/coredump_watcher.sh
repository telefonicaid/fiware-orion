#!/usr/bin/env bash

# Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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

# This script can be used in combination with the core-catcher feature
# (activated with the GENERATE_COREDUMP variable in /etc/sysconfig/contextBroker)
#
# The script can be called in a cron and it will send an email when new core 
# appears. Note: {{ var }} are Jinja2 template vars an can be replaced by your own messages.

DIRECTORY_TO_CHECK="/var/cb_cores"
FILE_PATTERN_TO_CHECK="CB_core_*"

LAST_FILE_LIST="${DIRECTORY_TO_CHECK}/.last_file_list.txt"
NEW_FILE_LIST="${DIRECTORY_TO_CHECK}/.new_file_list.txt"
HOSTNAME_FROM="$(hostname)"

ls ${DIRECTORY_TO_CHECK}/${FILE_PATTERN_TO_CHECK} > ${NEW_FILE_LIST} 2> /dev/null

[[ ! -f ${LAST_FILE_LIST} ]] && touch ${LAST_FILE_LIST}

diff -q ${NEW_FILE_LIST} ${LAST_FILE_LIST} &> /dev/null 
if [[ ${?} -eq 1 ]]; then
	mailx  -S smtp={{ smtp_orion_server }} -s "{{ core_mail_subject }}" -r core_checker@${HOSTNAME_FROM} {{ core_mail_recipient_list }} <<< "{{ core_mail_message_body }}"
	rm -f ${LAST_FILE_LIST}
	mv ${NEW_FILE_LIST} ${LAST_FILE_LIST}
else
	echo "Content of ${DIRECTORY_TO_CHECK} has NOT changed"
fi
