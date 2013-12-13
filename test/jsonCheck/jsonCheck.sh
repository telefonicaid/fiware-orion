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
# fermin at tid dot es



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

while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];             then usage 0;
  elif [ "$1" == "-v" ];             then verbose=on;
 else
    echo $0: bad parameter/option: "'"${1}"'"
    usage 1
  fi
  shift
done

# -----------------------------------------------------------------------------
#
# SRC_TOP - getting the TOP directory
#
dir=$(dirname $0)
SRC_TOP1=${PWD}/${dir}/../..   # if called with a relative path
SRC_TOP2=${dir}/../..          # if called via $PATH or with an absolute path
if [ -d ${SRC_TOP1} ]
then
  SRC_TOP=${SRC_TOP1}
else
  SRC_TOP=${SRC_TOP2}
fi

cd $SRC_TOP
SRC_TOP=$(pwd)
cd - > /dev/null
vMsg Git repo home: $SRC_TOP

# -----------------------------------------------------------------------------
# starting actual work

exitValue=0
for FILE in $(ls $SRC_TOP/test/unittests/testData/*.json)
do
   cat $FILE | python -m json.tool > /dev/null
   if [ "$?" -eq "1" ]; then
      echo "File $FILE is not a well-formed JSON document"
      exitValue=1
   fi
done
exit $exitValue
