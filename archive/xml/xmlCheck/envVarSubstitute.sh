#!/bin/bash
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



# -----------------------------------------------------------------------------
#
# usage
#
function usage()
{
  exitCode=$1

  spaces=$(echo $0 | tr '0-9a-zA-Z /_.\-' ' ')

  echo $0 "[-u (usage)]"
  echo "${spaces} [-v (verbose)]"
  echo "${spaces} -f <file to check>"

  exit $exitCode
}



# -----------------------------------------------------------------------------
#
# vMsg
#
function vMsg()
{
  if [ "$verbose" = "on" ]
  then
    echo $*
  fi
}



# ------------------------------------------------------------------------------
#
# command line options
#
verbose="off"
file=""

while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];             then usage 0;
  elif [ "$1" == "-v" ];             then verbose=on;
  elif [ "$1" == "-f" ];             then file=$2; shift;
  else
    echo $0: bad parameter/option: "'"${1}"'"
    usage 1
  fi
  shift
done

if [ "$file" == "" ]
then
  echo $0: no file specified
  exit 2
fi



# ------------------------------------------------------------------------------
#
# Just substitute 
#
vMsg $file to get variable substitution
unsubstituted=$(cat $file)
vMsg unsubstituted:
vMsg '----------------------------------'
vMsg $unsubstituted
vMsg '----------------------------------'

substituted1=$(echo $unsubstituted | sed 's/\$SUB_ID/123456789012345678901234/g')
substituted2=$(echo $substituted1  | sed 's/\$SUBID/123456789012345678901234/g')
substituted3=$(echo $substituted2  | sed 's/\${LISTENER_PORT}/9999/g')
substituted4=$(echo $substituted3  | sed 's/\$subscriptionId/123456789012345678901234/g')
substituted5=$(echo $substituted4  | sed 's/\$REG_ID/123456789012345678901234/g')

substituted=$substituted5

vMsg
vMsg '----------------------------------'
vMsg $substituted
vMsg '----------------------------------'

echo "$substituted" > ${file}
