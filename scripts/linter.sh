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
  else
  {
    echo $0: bad parameter/option: "'"${1}"'";
    echo
    usage 1
  }
  fi

  shift
done



ME=$(basename $0)

vMsg "Running lint"
scripts/cpplint.py src/app/contextBroker/*.cpp src/app/contextBroker/*.h src/lib/*/*.cpp src/lib/*/*.h 2> LINT

errors=$(grep "Total errors found" LINT | awk -F:\  '{ print $2 }')
lines=$(wc -l src/app/contextBroker/*.cpp src/app/contextBroker/*.h src/lib/*/*.cpp src/lib/*/*.h | grep -v src/ | awk '{ print $1 }')

typeset -i files
filesCpp=$(find src -name "*.cpp" | wc -l)
filesH=$(find src -name "*.h" | wc -l)
files=$filesCpp+$filesH
percentage=$(echo "scale=2; $errors*100/$lines" | bc)
echo $errors errors in $lines lines of source code in $files files \($percentage%\ style-guide-incompatibilities\)


#
# Categories:
# already included at 
# Found C system header after C++ system header
# Lines should be <= 120 characters long
# Lines should very rarely be longer than 150 characters
# Line ends in whitespace
# Never use sprintf
# Use int16/int64/etc
# #ifndef header guard has wrong style
# #endif line should be
# Weird number of spaces at line-start
# Blank line at the end of a code block
# Include the directory when naming
# Do not use namespace using-directives
# Is this a non-const reference
# At least two spaces is best between code and comments
# Labels should always be indented at least one space
# Tab found; better to use spaces
# Add #include
# Blank line at the start of a code block
# More than one command on the same line
# You don't need a ; after a }
# Single-argument constructors should be marked explicit
# Streams are highly discouraged
# Do not leave a blank line after
# Else clause should never be on same line as else
# Do not leave a blank line after
# Almost always, snprintf is better than strcpy
# Missing space after ,
# Extra space before ( in function call
# All parameters should be named in a function
# No copyright message found
# Found C system header after other header
# Should have a space between // and comment
# If an else has a brace on one side, it should have it on both
# Almost always, snprintf is better than strcat
# Using sizeof(type).  Use sizeof(varname) instead if possible
# Missing space before {
# Found C++ system header after other header
# Consider using getpwuid_r(...) instead of getpwuid
# Line contains only semicolon
# sscanf can be ok, but is slow and can overflow buffers
# Missing space before (
# Extra space after ( in function call
# If you can, use sizeof
# Missing spaces around =
# Mismatching spaces inside ()




