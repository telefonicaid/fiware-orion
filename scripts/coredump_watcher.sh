#!/usr/bin/env bash

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
