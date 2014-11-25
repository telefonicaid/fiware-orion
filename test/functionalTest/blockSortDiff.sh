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
#


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
  echo "$empty [--referenceFile <path>]"
  echo "$empty [--brokerOutputFile <path>]"

  exit $1
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



# -----------------------------------------------------------------------------
#
# filePortion
#
function filePortion()
{
  file=$1
  startLine=$2
  endLine=$3
  sort=$4
  outFile=$5

  vMsg Getting lines $startLine to $endLine from $file
  typeset -i lines
  lines=$endLine-$startLine+1;

  vMsg head -$endLine $file PIPE tail -$lines
  if [ $sort == "SORT" ]
  then
    head -$endLine $file | tail -$lines | sort >> $outFile
  else
    head -$endLine $file | tail -$lines >> $outFile
  fi
}



# -----------------------------------------------------------------------------
#
# blockSortedDiff - 
#
function blockSortedDiff
{
  typeset -i outFrom
  typeset -i outTo
  typeset -i refFrom
  typeset -i refTo

  refFrom=$1
  refTo=$2
  sorts=$3

  refFrom=$refFrom+1
  refTo=$refTo-1

  delta=$(echo "$sorts*2+1" | bc)
  outFrom=$refFrom-$delta
  outTo=$refTo-$delta

  typeset -i sortsNotZero
  sortsNotZero=$sorts+1
  echo "#SORT_START ("$sortsNotZero")" >> $referenceFile.sorted
  echo "#SORT_START ("$sortsNotZero")" >> $brokerOutputFile.sorted

  filePortion $referenceFile    $refFrom $refTo SORT $referenceFile.sorted
  filePortion $brokerOutputFile $outFrom $outTo SORT $brokerOutputFile.sorted

  echo "#SORT_END ("$sortsNotZero")" >> $referenceFile.sorted
  echo "#SORT_END ("$sortsNotZero")" >> $brokerOutputFile.sorted
}



# -----------------------------------------------------------------------------
#
# blockDiff - 
#
function blockDiff
{
  typeset -i outFrom
  typeset -i outTo
  typeset -i refFrom
  typeset -i refTo

  refFrom=$1
  refTo=$2
  sorts=$3
  last=$4

  if [ $ix != 0 ]
  then
    refFrom=$refFrom+1
  fi

  if [ "$last" != "LAST" ]
  then
    refTo=$refTo-1
  fi

  delta=$(echo "$sorts*2" | bc)
  outFrom=$refFrom-$delta
  outTo=$refTo-$delta

  filePortion $referenceFile    $refFrom $refTo NOT_SORT $referenceFile.sorted
  filePortion $brokerOutputFile $outFrom $outTo NOT_SORT $brokerOutputFile.sorted
}



# ------------------------------------------------------------------------------
#
# Find out in which directory this script resides
#
dirname=$(dirname $0)

if [ "$dirname" == "" ]
then
  dirname="."
fi

cd $dirname
SCRIPT_HOME=$(pwd)
cd - > /dev/null 2>&1



# -----------------------------------------------------------------------------
#
# Argument parsing
#
verbose=off
referenceFile=""
brokerOutputFile=""

while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];                 then usage 0;
  elif [ "$1" == "-v" ];                 then verbose=on;
  elif [ "$1" == "--referenceFile" ];    then referenceFile="$2"; shift;
  elif [ "$1" == "--brokerOutputFile" ]; then brokerOutputFile="$2"; shift;
  else
    echo $0: bad parameter/option: "'"${1}"'";
    echo
    usage 1
  fi
  shift
done



# -----------------------------------------------------------------------------
#
# Check
#
if [ $referenceFile == "" ] || [ $brokerOutputFile == "" ]
then
  echo "--referenceFile and --brokerOutputFile are MANDATORY"
  exit 1;
fi


if [ ! -f $referenceFile ] || [ ! -f $brokerOutputFile ]
then
  echo Bad Input files
  exit 2
fi


# -----------------------------------------------------------------------------
#
# Number of lines in the files
#
brokerOutputFileLines=$(wc -l $brokerOutputFile | awk '{ print $1 }')
referenceFileLines=$(wc -l $referenceFile | awk '{ print $1 }')



# -----------------------------------------------------------------------------
#
# List of line numbers of SORT_START in $referenceFile
# List of line numbers of SORT_END in $referenceFile
#
declare -A sortFromHere;
declare -A sortToHere;
typeset -i fromHereIx;
typeset -i toHereIx;

fromHereIx=0;
for fromHere in $(grep -n ^#SORT_START$ $referenceFile | awk -F: '{ print $1 }')
do
  sortFromHere[$fromHereIx]=$fromHere
  fromHereIx=$fromHereIx+1
done

toHereIx=0;
for toHere in $(grep -n ^#SORT_END$ $referenceFile | awk -F: '{ print $1 }')
do
  sortToHere[$toHereIx]=$toHere
  toHereIx=$toHereIx+1
done

if [ $fromHereIx != $toHereIx ]
then
  echo Invalid reference file "(found SORT_FROM_HERE in $fromHereIx places but SORT_TO_HERE in $toHereIx places)"
  exit 3
fi



# -----------------------------------------------------------------------------
#
# Preparing new files to be compared
#
startDate=$(date)
echo $startDate > $referenceFile.sorted
echo $startDate > $brokerOutputFile.sorted



# -----------------------------------------------------------------------------
#
# Counters for the main loop
#
typeset -i lineNo
typeset -i ix;
typeset -i from;
typeset -i to;

lineNo=1
ix=0



# -----------------------------------------------------------------------------
#
# Main loop
#
vMsg Starting ...
while [ $ix -le $fromHereIx ]
do
  if [ $ix == $fromHereIx ]
  then
    from=$lineNo
    to=$referenceFileLines
    blockDiff $from $to $ix LAST
  else
    from=$lineNo
    to=${sortFromHere[$ix]}
    blockDiff $from $to $ix

    from=${sortFromHere[$ix]}
    to=${sortToHere[$ix]}
    blockSortedDiff $from $to $ix
  fi

  lineNo=${sortToHere[$ix]}
  ix=$ix+1
done

#
# output file name and path
#
dirname=$(dirname $brokerOutputFile)
filename=$(basename $brokerOutputFile .out)
outputFile=$dirname/$filename.diff

echo "Comparing $referenceFile.sorted to $brokerOutputFile.sorted, output to $outputFile"
$SCRIPT_HOME/testDiff.py -r $referenceFile.sorted -i $brokerOutputFile.sorted > $outputFile
exitCode=$?
if [ "$exitCode" != 0 ]
then
  echo "Files differ"
else
  echo "Files coincide"
fi

exit $exitCode
