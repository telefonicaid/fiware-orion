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
  echo "${spaces} [-f (file to check)]"
  echo "${spaces} [--filter (test filter)]"
  echo "${spaces} [--no-harness (no harness files)]"
  echo "${spaces} [--dryrun (don't actually execute anything)]"
  echo "${spaces} [--keep (do NOT remove harness files at end of processing)]"
  echo "${spaces} [--xsd-download (download XSD files for FIWARE subversion)]"
  echo "${spaces} [--xsd-dir (directory for XSD files)]"

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
# xsdGet - 
#
function xsdGet()
{
  prefix="$1"
  xfile="$2"

  if [ "$prefix" = "ngsi9" ]
  then
    echo $xsdDir/Ngsi9_Operations.xsd
  elif [ "$prefix" == "ngsi10" ]
  then
    echo $xsdDir/Ngsi10_Operations.xsd
  elif [ "$prefix" == "ngsi" ]
  then
    echo $xsdDir/Ngsi9_10_dataStructures.xsd
  else
    echo "unknown file prefix: '"${prefix}"' for $xfile"
    exit 2
  fi
}



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
    vMsg dryrun: xmllint "$xmlFile" --schema "$xsdFile"
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
  harnessList=""
  TMP_DIR=$(mktemp -d /tmp/xmlCheck.XXXXX)
  vMsg TMP_DIR: $TMP_DIR
  for FILE in $(find $SRC_TOP/test/functionalTest/cases -name *.test)
  do
    PREFIX=$(basename ${FILE%.*})
    FILE_DIR=$(dirname $FILE)
    LAST_SUBDIR=$(basename $FILE_DIR)
    mkdir -p $TMP_DIR/$LAST_SUBDIR
    $SRC_TOP/test/xmlCheck/xmlExtractor.py $FILE $TMP_DIR/$LAST_SUBDIR $PREFIX
  done

  for FILE in $(find $TMP_DIR -name ngsi*.valid.xml)
  do
    grep '\$' $FILE
    if [ "$?" != "0" ]
    then
      continue
    fi

    $SRC_TOP/test/xmlCheck/envVarSubstitute.sh -f "$FILE"
  done

  harnessList=$(find $TMP_DIR -name "ngsi*.valid.xml")
  xmlPartsFound=$(find $TMP_DIR -name "ngsi*.xml" | wc -l)
  xmlPartsValid=$(find $TMP_DIR -name "ngsi*.valid.xml" | wc -l)
  xmlPartsInvalid=$(find $TMP_DIR -name "ngsi*.invalid.xml" | wc -l)
  xmlPartsPostponed=$(find $TMP_DIR -name "ngsi*.postponed.xml" | wc -l)
  xmlPartsUnknown=$(find $TMP_DIR -name "unknown*.xml" | wc -l)
}



# ------------------------------------------------------------------------------
#
# command line options
#
verbose="off"
filter=""
file=""
xsdDir="/tmp/xsd"
harness="on"
dryrun="off"
xsdDownload="off"
keep="off"

while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];             then usage 0;
  elif [ "$1" == "-v" ];             then verbose=on;
  elif [ "$1" == "-f" ];             then file=$2; shift;
  elif [ "$1" == "--filter" ];       then filter=$2; shift;
  elif [ "$1" == "--no-harness" ];   then harness="off";
  elif [ "$1" == "--dryrun" ];       then dryrun="on";
  elif [ "$1" == "--keep" ];         then keep="on";
  elif [ "$1" == "--xsd-download" ]; then xsdDownload="on";
  elif [ "$1" == "--xsd-dir" ];      then xsdDir=$2; shift;
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
#
# Checking the XSD directory
#
if [ ! -d "$xsdDir" ]
then
  echo "$0: error: '"${xsdDir}"': no such directory"
  exit 3
fi




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

  \rm Ngsi10_Operations.xsd Ngsi9_Operations.xsd Ngsi9_10_dataStructures.xsd 2> /dev/null
  wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fiware.org/scmrepos/svn/iot/trunk/schemes/Ngsi10_Operations.xsd
  wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fiware.org/scmrepos/svn/iot/trunk/schemes/Ngsi9_Operations.xsd
  wget -q --no-check-certificate --user=$USER --password=$PASS https://forge.fiware.org/scmrepos/svn/iot/trunk/schemes/Ngsi9_10_dataStructures.xsd

  if [ ! -f Ngsi10_Operations.xsd ] || [ ! -f Ngsi9_Operations.xsd ] || [ ! -f Ngsi9_10_dataStructures.xsd ]
  then
    echo $0: error: wget failed to download latest XSD files
    exit 4
  fi

  mv Ngsi10_Operations.xsd Ngsi9_Operations.xsd Ngsi9_10_dataStructures.xsd $xsdDir

  vMsg "got XSD files"
fi



# ------------------------------------------------------------------------------
#
# XSD files there?
#
if [ ! -f $xsdDir/Ngsi10_Operations.xsd ] || [ ! -f $xsdDir/Ngsi9_Operations.xsd ] || [ ! -f $xsdDir/Ngsi9_10_dataStructures.xsd ]
then
  echo "$0: error: XSD files missing in $xsdDir"
  exit 5
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



# -----------------------------------------------------------------------------
#
# Setting up the file list
#
if [ "$file" != "" ]
then
  fileList=$file
  verbose2=on
else
  if [ "$harness" == "on" ]
  then
    harnessFiles
  fi

  ngsi9List=$(find $SRC_TOP/test -name "ngsi9.*.valid.xml")
  ngsi10List=$(find $SRC_TOP/test -name "ngsi10.*.valid.xml")
  fileList=${ngsi9List}" "${ngsi10List}
  partList=$harnessList
fi



# ------------------------------------------------------------------------------
#
# Counters
#
typeset -i xmlFilesFound        # Total number of XML files under test/
typeset -i xmlFilesValid        # Number of XML files that are valid
typeset -i xmlFilesInvalid      # Number of XML files that are invalid
typeset -i xmlFilesPostponed    # Number of XML files that are postponed
typeset -i xmlFilesBadName      # Number of XML files whose names don't follow the naming convention - should be zero
typeset -i xmlFilesProcessed    # Total number of XML files that were tested
typeset -i xmlFilesOK           # Number of XML files that passed the test
typeset -i xmlFilesErrors       # Number of XML files that did not pass the test

typeset -i harnessFilesFound    # Number of files in the harness directory - to part in many XML files
typeset -i xmlPartsFound        # Number of XML parts created from harness directory
typeset -i xmlPartsValid        # Number of XML parts that are valid
typeset -i xmlPartsInvalid      # Number of XML parts that are invalid
typeset -i xmlPartsPostponed    # Number of XML parts that are postponed
typeset -i xmlPartsUnknown      # Number of XML parts that are unknown
typeset -i xmlPartsProcessed    # Number of XML parts that were tested 
typeset -i xmlPartsOK           # Number of XML parts that passed the test
typeset -i xmlPartsErrors       # Number of XML parts that did not pass the test
typeset -i xmlDocsFound         # xmlFilesFound + xmlPartsFound
typeset -i xmlDocsProcessed     # xmlFilesProcessed + xmlPartsProcessed
typeset -i xmlDocsPostponed     # xmlFilesPostponed + xmlPartsPostponed
typeset -i xmlDocsOk            # xmlFilesOK + xmlPartsOK
typeset -i xmlDocsErrors        # xmlFilesErrors + xmlPartsErrors

xmlFilesFound=$(find $SRC_TOP/test -name "*.xml" | wc -l)
xmlFilesValid=$(find $SRC_TOP/test -name "ngsi*.valid.xml" | wc -l)
xmlFilesInvalid=$(find $SRC_TOP/test -name "ngsi*.invalid.xml" | wc -l)
xmlFilesPostponed=$(find $SRC_TOP/test -name "ngsi*.postponed.xml" | wc -l)
xmlFilesMiddle=$(find $SRC_TOP/test -name "ngsi*.middle.xml" | wc -l)
xmlFilesOrion=$(find $SRC_TOP/test -name "orion.*.xml" | wc -l)
xmlFilesBadName=$(find $SRC_TOP/test -name "*.xml" | egrep -v 'ngsi.*\.valid\.xml' | egrep -v 'ngsi.*\.invalid\.xml' | egrep -v 'ngsi.*\.postponed\.xml' | egrep -v 'ngsi.*\.middle\.xml' | egrep -v 'orion\..*\.xml' | wc -l)
xmlFilesProcessed=0
xmlFilesOK=0
xmlFilesErrors=0
harnessFilesFound=$(find $SRC_TOP/test/functionalTest/cases -name "*.test" | wc -l)
xmlPartsFound=$xmlPartsFound           # already taken care of by function 'harnessFiles'
xmlPartsValid=$xmlPartsValid           # already taken care of by function 'harnessFiles'
xmlPartsInvalid=$xmlPartsInvalid       # already taken care of by function 'harnessFiles'
xmlPartsPostponed=$xmlPartsPostponed   # already taken care of by function 'harnessFiles'
xmlPartsUnknown=$xmlPartsUnknown       # already taken care of by function 'harnessFiles'
xmlPartsProcessed=0
xmlPartsOK=0
xmlPartsErrors=0
xmlDocsFound=$(expr $xmlFilesFound + $xmlPartsFound)
xmlDocsProcessed=0
xmlDocsOk=0
xmlDocsErrors=0



# ------------------------------------------------------------------------------
#
# Applying filter to the file lists
#
if [ "$filter" != "" ]
then
  vMsg appying filter "'"${filter}"'"
  newFileList=""
  for xfile in $fileList
  do
    echo $xfile | grep "$filter" > /dev/null 2>&1
    if [ "$?" == 0 ]
    then
      newFileList=${newFileList}" "${xfile}
      xmlFilesProcessed=$xmlFilesProcessed+1
    fi
  done
  fileList=$newFileList

  newPartList=""
  for part in $partList
  do
    echo $part | grep "$filter" > /dev/null 2>&1
    if [ "$?" == 0 ]
    then
      newPartList=${newPartList}" "${part}
      xmlPartsProcessed=$xmlPartsProcessed+1
    fi
  done
  partList=$newPartList    
fi



# ------------------------------------------------------------------------------
#
# process the list of files to check
#
OK=0
ERR=0
N=0
for xfile in $fileList
do
  fileName=$(basename $xfile)
  prefix=$(echo $fileName | cut -d . -f 1)
  xsd=$(xsdGet "$prefix" "$xfile")
  processFile "$xfile" "$xsd" 
done

xmlFilesProcessed=$N
xmlFilesOK=$OK;
xmlFilesErrors=$ERR



# ------------------------------------------------------------------------------
#
# process the list of parts to check
#
OK=0
ERR=0
N=0
for part in $partList
do
  partName=$(basename $part)
  prefix=$(echo $partName | cut -d . -f 1)
  xsd=$(xsdGet "$prefix" "$part")
  processFile "$part" "$xsd" 
done

xmlPartsProcessed=$N
xmlPartsOK=$OK;
xmlPartsErrors=$ERR


xmlDocsProcessed=$(expr $xmlFilesProcessed + $xmlPartsProcessed)
xmlDocsOk=$(expr $xmlFilesOK + $xmlPartsOK)
xmlDocsErrors=$(expr $xmlFilesErrors + $xmlPartsErrors)





# ------------------------------------------------------------------------------
#
# Statistics not shown if the option '-f' is set
#
if [ "$file" != "" ]
then
  if [ "$ERR" != "0" ]
  then
    exit 6
  else
    exit 0
  fi
fi



# ------------------------------------------------------------------------------
#
# Statistics
#
echo "====================================================================="
if [ "$xmlDocsErrors" != "0" ]
then
  echo
  echo "XML docs that did not pass the XML Validity test:"

  for item in $failingList
  do
    echo "  o " $item
  done

  echo
  echo "====================================================================="
fi


echo "Tested ${xmlDocsProcessed} (${xmlFilesProcessed} files + ${xmlPartsProcessed} parts) out of ${xmlDocsFound} (${xmlFilesFound} files + ${xmlPartsFound} parts) XML documents:"
echo "  ${xmlDocsOk} documents passed the XML validity test"

exitCode=7

if [ "$xmlDocsErrors" != 0 ]
then
  echo "  ${xmlDocsErrors} documents did not pass"
else
  echo "  -----------------------"
  echo "  ALL documents passed!!!"
  echo "  -----------------------"
  exitCode=0
fi

echo
echo "${xmlFilesInvalid} documents were not tested as they on purpose don't follow the XSD"
xmlDocsPostponed=$xmlFilesPostponed+$xmlPartsPostponed
echo "${xmlDocsPostponed} documents were not tested as they still have no XSD"
echo "${xmlFilesMiddle} documents were not tested as they don't start the way the XSD states (middle)"


if [ "$xmlFilesBadName" != 0 ]
then
  echo
  if [ "$xmlFilesBadName" != "0" ]
  then
    echo "WARNING: $xmlFilesBadName XML files do not conform to the naming convention"  
    for xfile in    $(find $SRC_TOP/test -name "*.xml" | grep -v "ngsi*.valid.xml" | grep -v "ngsi*.invalid.xml" | grep -v "ngsi*.postponed.xml" | grep -v "ngsi*.middle.xml" | grep -v "orion.*.xml")
    do
      echo "  o $xfile"
    done
  fi
  exitCode=8
fi


if [ "$xmlPartsUnknown" != 0 ]
then
  echo
  echo "WARNING: parts marked as unknown"
  for xfile in $(find $TMP_DIR -name "unknown*.xml")
  do
    echo "  o $xfile"
  done
  exitCode=9
fi



# ------------------------------------------------------------------------------
#
# Keep?
#
if [ "$keep" == "off" ]
then
  rm -rf $TMP_DIR
fi

if [ "$exitCode" != "0" ]
then
  echo exiting with error code $exitCode
fi

exit $exitCode
