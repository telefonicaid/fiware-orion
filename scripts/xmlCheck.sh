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
  echo "${spaces} [-f (file to check)]"
  echo "${spaces} [--filter (test filter)]"
  echo "${spaces} [--noharness (no harness files)]"
  echo "${spaces} [--dryrun (don't actually execute anything)]"
  echo "${spaces} [--xsdDownload (download XSD files for FIWARE subversion)]"
  echo "${spaces} [--cleanup (do NOT remove harness files at end of processing)]"

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
filter=""
file=""
xsd=""
harness="on"
dryrun="off"
xsdDownload="off"
cleanup="on"

while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];            then usage 0;
  elif [ "$1" == "-v" ];            then verbose=on;
  elif [ "$1" == "--filter" ];      then filter=$2; shift;
  elif [ "$1" == "-f" ];            then file=$2; shift;
  elif [ "$1" == "--noharness" ];   then harness="off";
  elif [ "$1" == "--dryrun" ];      then dryrun="on";
  elif [ "$1" == "--cleanup" ];     then cleanup="off";
  elif [ "$1" == "--xsdDownload" ]; then xsdDownload="on";
  else
    echo $0: bad parameter/option: "'"${1}"'"
    usage 1
  fi
  shift
done



# -----------------------------------------------------------------------------
#
# SRC_TOP
#
dir=$(dirname $0)
SRC_TOP=${PWD}/${dir}/../
XSD_DIR=$SRC_TOP/scripts/XSD
vMsg Git repo home: $SRC_TOP



# ------------------------------------------------------------------------------
#
# xsdGet - 
#
function xsdGet()
{
  prefix="$1"
  file="$2"

  if [ "$prefix" = "ngsi9" ]
  then
    echo $XSD_DIR/Ngsi9_Operations_v07.xsd
  elif [ "$prefix" == "ngsi10" ]
  then
    echo $XSD_DIR/Ngsi10_Operations_v07.xsd
  else
    echo "unknown file prefix: '"${prefix}"' for $file"
    exit 1
  fi
}



# ------------------------------------------------------------------------------
#
# Download XSD files from FIWARE repo?
#
if [ "$xsdDownload" == "on" ]
then
  # Get the .xsd files
  echo  -n "Enter username: "
  read USER

  echo -n "Enter password: "
  STTY_ORIG=`stty -g` 
  stty -echo
  read PASS
  stty $STTY_ORIG
  echo

  wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fi-ware.eu/scmrepos/svn/iot/trunk/schemes/Ngsi10_Operations_v07.xsd
  wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fi-ware.eu/scmrepos/svn/iot/trunk/schemes/Ngsi9_Operations_v07.xsd
  wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fi-ware.eu/scmrepos/svn/iot/trunk/schemes/Ngsi9_10_dataStructure_v07.xsd

  mv Ngsi10_Operations_v07.xsd Ngsi9_Operations_v07.xsd Ngsi9_10_dataStructure_v07.xsd $XSD_DIR

  echo "got XSD files"
fi



# ------------------------------------------------------------------------------
#
# variables
#
typeset -i N
typeset -i OK
typeset -i ERR
N=0
OK=0
ERR=0



# ------------------------------------------------------------------------------
#
# processFile - 
#
function processFile()
{
  xmlFile=$1
  xsdFile=$2

  if [ "$xmlFile" == "" ] || [ "$xsdFile" == "" ]
  then
    echo "bad parameters for file processing '"${xmlFile}"', '"${xsdFile}"'" 
    return;
  fi

  N=$N+1

  if [ "$dryrun" == "on" ]
  then
    echo dryrun: xmllint "$xmlFile" --schema "$xsdFile"
    return
  fi

  vMsg XSD: $xsdFile
  if [ "$verbose2" == "on" ]
  then
    xmllint $xmlFile --schema $xsdFile
    RESULT=$?
  else
    xmllint $xmlFile --schema $xsdFile > /dev/null 2>&1
    RESULT=$?
  fi

  if [ "$RESULT" == "0" ]
  then
    echo "$xmlFile: ok"
    OK=$OK+1
  else
    echo "$xmlFile: FAILS (xmllint error: $RESULT)"
    ERR=$ERR+1
    failingList=${failingList}" "$xmlFile
  fi
}



# -----------------------------------------------------------------------------
#
# harnessFiles - 
#
function harnessFiles()
{
  TMP_DIR=$(mktemp -d /tmp/xmlCheck.XXXXX)
  for FILE in $(find $SRC_TOP/test/testharness -name *.test)
  do
    PREFIX=$(basename ${FILE%.*})
    $SRC_TOP/scripts/xmlExtractor2.py $FILE $TMP_DIR $PREFIX
  done

  harnessList=$(ls $TMP_DIR/*ngsi9* $TMP_DIR/*ngsi10*)
}



# -----------------------------------------------------------------------------
#
# Setting up the file list
#
if [ "$file" != "" ]
then
  fileList=$file
  verbose2=on
else
  fileList=$(find . -name "ngsi9.*.valid.xml")
  fileList=$(find . -name "ngsi10.*.valid.xml")

  harnessList=""
  if [ "$harness" == "on" ]
  then
    harnessFiles
  fi

  ngsi9List=$(find . -name "ngsi9.*.valid.xml")
  ngsi10List=$(find . -name "ngsi10.*.valid.xml")
  fileList=${ngsi9List}" "${ngsi10List}" "${harnessList}
fi



# ------------------------------------------------------------------------------
#
# file counters
#
typeset -i all
typeset -i total
typeset -i included

all=0
files=0
included=0



# ------------------------------------------------------------------------------
#
# Counting the total number of files
#
for file in $fileList
do
  total=$total+1
done



# ------------------------------------------------------------------------------
#
# Applying filter to the file list
#
if [ "$filter" != "" ]
then
  vMsg appying filter "'"${filter}"'"
  newFileList=""
  for file in $fileList
  do
    echo $file | grep "$filter" > /dev/null 2>&1
    if [ "$?" == 0 ]
    then
      newFileList=${newFileList}" "${file}
      included=$included+1
    fi
  done
  fileList=$newFileList
fi



# ------------------------------------------------------------------------------
#
# Counting XMLs
#
here=$(find . -name "*.xml" | wc -l)
harness9=$(find $TMP_DIR -name "ngsi9.*" | wc -l)
harness10=$(find $TMP_DIR -name "ngsi10.*" | wc -l)
all=$here+$harness9+$harness10

if [ "$all" != "$total" ]
then
  typeset -i diff
  diff=$(expr $all - $total)
  echo
  echo ' ----------------------------------------------------------------------------------------'
  echo "   Warning - there seem to be $diff XML files not recognized as such for this checker"
  echo "   Please, check their file names ..."
  echo ' ----------------------------------------------------------------------------------------'
  echo
fi

vMsg "Total number of files in test before filter: $total"
vMsg "Files included in test run: $included"
vMsg "Total number of XML files found: $all"



# ------------------------------------------------------------------------------
#
# process the list of files to check
#
for file in $fileList
do
  fileName=$(basename $file)
  prefix=$(echo $fileName | cut -d . -f 1)
  xsd=$(xsdGet "$prefix" "$file")
  processFile "$file" "$xsd" 
done


echo $N files processed
echo $ERR errors
echo $OK files are OK
echo

if [ "$ERR" != "0" ]
then
  echo "List of files with problems:"
  for file in $failingList
  do
    echo "  o " $file
  done
fi

if [ "$cleanup" == "on" ]
then
  rm -rf $TMP_DIR
fi
