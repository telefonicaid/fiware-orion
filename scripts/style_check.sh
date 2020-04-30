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
# iot_support at tid dot es
#
# Author: Ken Zangelin



# -----------------------------------------------------------------------------
#
# vars
#
ME=$(basename $0)



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
# finish - 
#
function finish()
{
  if [ "$mayorErrors" != "0" ]
  then
    mMsg "found $totalErrors errors ($mayorErrors mayor, $minorErrors minor errors) in $lines lines of source code in $files files ($percentage% style-guide-incompatibilities)"
    exit 1
  else
    mMsg "[$dir]: found no mayor errors ($minorErrors minor errors) in $lines lines of source code in $files files ($percentage% style-guide-incompatibilities)"
    exit 0
  fi
}



# ------------------------------------------------------------------------------
#
# Argument parsing
#
export verbose=off
export dir=""

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];            then usage 0;
  elif [ "$1" == "-v" ];            then verbose=on;
  elif [ "$1" == "-d" ];            then dir=$2; shift
  else
  {
    echo $0: bad parameter/option: "'"${1}"'";
    echo
    usage 1
  }
  fi

  shift
done



#
# Check input
#
if [ "$dir" != "" ]
then
  if [ "$dir" == "." ]
  then
    dir="";
  else
    ls $dir/*.cpp > /dev/null 2>&1
    if [ "$?" != 0 ]
    then
      mMsg bad directory \'$dir\'
      exit 1
    fi
  fi
fi



#
# Environment vars
#
if [ "$VERBOSE" == "1" ]
then
  verbose=on
fi

typeset -i lines

if [ "$dir" == "" ]
then
  vMsg "Running TID/google style-guide check on entire project"
  scripts/cpplint.py src/app/contextBroker/*.cpp src/app/contextBroker/*.h src/lib/*/*.cpp src/lib/*/*.h 2> LINT
  lines=$(wc -l src/app/contextBroker/*.cpp src/app/contextBroker/*.h src/lib/*/*.cpp src/lib/*/*.h | grep -v src/ | awk '{ print $1 }')
  filesCpp=$(find src -name "*.cpp" | wc -l)
  filesH=$(find src -name "*.h" | wc -l)
else
  vMsg "Running TID/google style-guide check on $dir"
  scripts/cpplint.py $dir/*.cpp $dir/*.h 2> LINT

  typeset -i lines1
  typeset -i lines2

  lines1=0
  lines2=0

  filesCpp=$(ls $dir/*.cpp 2> /dev/null | wc -l)
  filesH=$(ls $dir/*.h 2> /dev/null | wc -l)

  if [ "$filesCpp" == "1" ]
  then
    lines1=$(wc -l $dir/*.cpp 2> /dev/null | awk '{ print $1 }')
  elif [ "$filesCpp" != "0" ]
  then
    lines1=$(wc -l $dir/*.cpp 2> /dev/null | grep ' total' | awk '{ print $1 }')
  fi

  if [ "$filesH" == "1" ]
  then
    lines2=$(wc -l $dir/*.h 2> /dev/null | awk '{ print $1 }')
  elif [ "$filesH" != "0" ]
  then
    lines2=$(wc -l $dir/*.h 2> /dev/null | grep ' total' | awk '{ print $1 }')
  fi

  lines=$lines1+$lines2
fi

totalErrors=$(grep "Total errors found" LINT | awk -F:\  '{ print $2 }')

typeset -i files

files=$filesCpp+$filesH
vMsg $lines lines in $files files in dir $dir

if [ "$files" != 0 ]
then
  percentage=$(echo "scale=2; $totalErrors*100/$lines" | bc)
else
  percentage=0
fi



if [ "$verbose" = "on" ]
then
  echo
  echo
fi


#
# lint error categories
#
declare -A cat

sev[0]=MAYOR;  cat[0]='already included at'
sev[1]=MAYOR;  cat[1]='Found C system header after C++ system header'
sev[2]=MINOR;  cat[2]='Lines should be <= 120 characters long'
sev[3]=MINOR;  cat[3]='Lines should very rarely be longer than 150 characters'
sev[4]=MAYOR;  cat[4]='Line ends in whitespace'
sev[5]=MAYOR;  cat[5]='Never use sprintf'
sev[6]=MINOR;  cat[6]='Use int16/int64/etc'
sev[7]=MAYOR;  cat[7]='#ifndef header guard has wrong style'
sev[8]=MAYOR;  cat[8]='#endif line should be'
sev[9]=MAYOR;  cat[9]='Weird number of spaces at line-start'
sev[10]=MAYOR; cat[10]='Blank line at the end of a code block'
sev[11]=MAYOR; cat[11]='Include the directory when naming'
sev[12]=MAYOR; cat[12]='Do not use namespace using-directives'
sev[13]=MAYOR; cat[13]='Is this a non-const reference'
sev[14]=MAYOR; cat[14]='At least two spaces is best between code and comments'
sev[15]=MAYOR; cat[15]='Labels should always be indented at least one space'
sev[16]=MAYOR; cat[16]='Tab found; better to use spaces'
sev[17]=MAYOR; cat[17]='Add #include'
sev[18]=MAYOR; cat[18]='Blank line at the start of a code block'
sev[19]=MINOR; cat[19]='More than one command on the same line'
sev[20]=MAYOR; cat[20]="You don't need a ; after a }"
sev[21]=MAYOR; cat[21]='Single-argument constructors should be marked explicit'
sev[22]=MAYOR; cat[22]='Streams are highly discouraged'
sev[23]=MAYOR; cat[23]='THIS LINE IS UNUSED'
sev[24]=MAYOR; cat[24]='Else clause should never be on same line as else'
sev[25]=MAYOR; cat[25]='Do not leave a blank line after "public:"'
sev[26]=MAYOR; cat[26]='Almost always, snprintf is better than strcpy'
sev[27]=MAYOR; cat[27]='Missing space after ,'
sev[28]=MAYOR; cat[28]='Extra space before ( in function call'
sev[29]=MAYOR; cat[29]='All parameters should be named in a function'
sev[30]=MAYOR; cat[30]='No copyright message found'
sev[31]=MAYOR; cat[31]='Found C system header after other header'
sev[32]=MAYOR; cat[32]='Should have a space between // and comment'
sev[33]=MAYOR; cat[33]='If an else has a brace on one side, it should have it on both'
sev[34]=MAYOR; cat[34]='Almost always, snprintf is better than strcat'
sev[35]=MINOR; cat[35]='Using sizeof(type).  Use sizeof(varname) instead if possible'
sev[36]=MAYOR; cat[36]='Missing space before {'
sev[37]=MAYOR; cat[37]='Found C++ system header after other header'
sev[38]=MAYOR; cat[38]='Consider using getpwuid_r(...) instead of getpwuid'
sev[39]=MAYOR; cat[39]='Line contains only semicolon'
sev[40]=MAYOR; cat[40]='sscanf can be ok, but is slow and can overflow buffers'
sev[41]=MAYOR; cat[41]='Missing space before ('
sev[42]=MAYOR; cat[42]='Extra space after ( in function call'
sev[43]=MINOR; cat[43]='If you can, use sizeof'
sev[44]=MAYOR; cat[44]='Missing spaces around ='
sev[45]=MAYOR; cat[45]='Mismatching spaces inside ()'
sev[46]=MAYOR; cat[46]='Closing ) should be moved to the previous line'
sev[47]=MAYOR; cat[47]='Extra space before )'
sev[48]=NONE;  cat[48]="LAST"



# -----------------------------------------------------------------------------
#
# Exceptions per library
#
# o Rule "Use int16/int64/etc" for mongoBackend
#     Unfortunately, overloaded functions in the mongodb client library wont work if we use int64
#     instead of long long (complaints about ambibuity), so, this rule cannot be a stopper for
#     the mongoBackend library
#
if [ "$dir" == "mongoBackend" ]
then
  sev[6]=MINOR
  cat[6]='Use int16/int64/etc (MINOR)'
fi



# -----------------------------------------------------------------------------
#
# Counting occurrences of the line error categories
#
typeset -i ix
ix=0
typeset -i minorErrors
typeset -i mayorErrors
minorErrors=0
mayorErrors=0

echo > LINT_ERRORS
while [ "${cat[$ix]}" != "LAST" ]
do
  errors=$(grep "${cat[$ix]}" LINT | wc -l)
  if [ "$errors" != "" ] && [ "$errors" != "0" ]
  then
    typeset -i errs=$errors
    printf "%04d errors of category '%s'\n" $errs "${cat[$ix]}" >> LINT_ERRORS
    if [ ${sev[$ix]} == MAYOR ]
    then
      mayorErrors=$mayorErrors+$errors
    else
      minorErrors=$minorErrors+errors
    fi
  fi

  ix=$ix+1
done


#
# If not verbose, then we have finished
#
if [ "$verbose" = "off" ]
then
  finish
fi



sort LINT_ERRORS



# -----------------------------------------------------------------------------
#
# Now let's see if we have any new lint errors
#
lintErrors=$(grep -v "already included at" LINT | 
grep -v "Found C system header after C++ system header" |
grep -v "Lines should be <= 120 characters long" |
grep -v "Lines should very rarely be longer than 150 characters" |
grep -v "Line ends in whitespace" |
grep -v "Never use sprintf" |
grep -v "Use int16/int64/etc" |
grep -v "#ifndef header guard has wrong style" |
grep -v "#endif line should be" |
grep -v "Weird number of spaces at line-start" |
grep -v "Blank line at the end of a code block" |
grep -v "Done processing" |
grep -v "Include the directory when naming" |
grep -v "Do not use namespace using-directives" |
grep -v "Is this a non-const reference" |
grep -v "At least two spaces is best between code and comments"  |
grep -v "Labels should always be indented at least one space" |
grep -v "Tab found; better to use spaces" |
grep -v "Add #include"  |
grep -v "Blank line at the start of a code block"  |
grep -v "More than one command on the same line" |
grep -v "You don't need a ; after a }" |
grep -v "Single-argument constructors should be marked explicit" |
grep -v "Streams are highly discouraged" |
grep -v "Else clause should never be on same line as else" |
grep -v "Do not leave a blank line after" |
grep -v "Almost always, snprintf is better than strcpy" |
grep -v "Missing space after ," |
grep -v "Extra space before ( in function call"  |
grep -v "All parameters should be named in a function" |
grep -v "No copyright message found" |
grep -v "Found C system header after other header." |
grep -v "Should have a space between // and comment" |
grep -v "If an else has a brace on one side, it should have it on both" |
grep -v "Almost always, snprintf is better than strcat" |
grep -v "Using sizeof(type).  Use sizeof(varname) instead if possible" |
grep -v "Missing space before {" |
grep -v "Found C++ system header after other header" |
grep -v "Consider using getpwuid_r(...) instead of getpwuid" |
grep -v "Line contains only semicolon" |
grep -v "sscanf can be ok, but is slow and can overflow buffers" |
grep -v "Missing space before (" |
grep -v "Extra space after ( in function call" |
grep -v "If you can, use sizeof" |
grep -v "Missing spaces around =" |
grep -v "Mismatching spaces inside ()" |
grep -v 'Closing ) should be moved to the previous line' |
grep -v 'Extra space before )' |
grep -v 'Total errors found')

if [ "$lintErrors" != "" ]
then
  echo
  echo "New style-guide errors:"
  echo "-----------------------------------------------------"
  echo $lintErrors
  echo "-----------------------------------------------------"
fi



finish
exit 0
