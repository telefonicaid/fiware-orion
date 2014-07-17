#!/bin/bash
# -*- coding: latin-1 -*-
# Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#
# Author: Ken Zangelin



# -----------------------------------------------------------------------------
#
# usage
#
function usage()
{
  sfile="Usage: "$(basename $0)
  empty=$(echo $sfile | tr 'a-zA-z/0-9.:' ' ')

  echo "$sfile [-u (usage)]"
  echo "$empty [-v (verbose)]"
  echo "$empty [-d <directory>]"
  echo "$empty [-f <file>]"

  exit $1
}



# -----------------------------------------------------------------------------
#
# vMsg - 
#
function vMsg()
{
  if [ "$verbose" = "on" ]
  then
    echo $ME: $*
  fi
}



# -----------------------------------------------------------------------------
#
# mMsg - 
#
function mMsg()
{
  echo $ME: $*
}



# ------------------------------------------------------------------------------
#
# Argument parsing
#
verbose=off
dirList=""
fileList=""

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];            then usage 0;
  elif [ "$1" == "-v" ];            then verbose=on;
  elif [ "$1" == "-d" ];            then dirList=$dirList+$2; shift;
  elif [ "$1" == "-f" ];            then fileList=$fileList+$2; shift;
  else
  {
    echo $0: bad parameter/option: "'"${1}"'";
    echo
    usage 1
  }
  fi

  shift
done



# ------------------------------------------------------------------------------
#
# Check arguments
#
if [ "$dirList" == "" ] && [ "$fileList" == "" ]
then
  dirList="src/app/contextBroker/*.cpp src/app/contextBroker/*.h src/lib/*/*.cpp src/lib/*/*.h"
fi



ME=$(basename $0)
vMsg "lint on $dirList $fileList"


mMsg "Running lint"

scripts/cpplint.py src/app/contextBroker/*.cpp src/app/contextBroker/*.h src/lib/*/*.cpp src/lib/*/*.h 2> LINT

grep -v 'should almost always be at the end of the previous line' LINT  > LINT2
grep -v 'Lines should very rarely be longer than 100 characters'  LINT2 > LINT
rm LINT2
cat LINT
