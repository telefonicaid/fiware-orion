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


# ------------------------------------------------------------------------------
#
# Set home directory
#
dirname=$(dirname $0)

if [ "$dirname" == "" ]
then
  dirname="."
fi

cd $dirname
SCRIPT_HOME=$(pwd)
cd - > /dev/null 2>&1
echo SCRIPT_HOME: $SCRIPT_HOME

# -----------------------------------------------------------------------------
#
# Env vars
#
export LC_ALL=C
export NAME="testFile"
declare -A testErrorV
typeset -i testError
export DIFF=$SCRIPT_HOME/testDiff.py
testError=0


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
  echo "$empty [--filter <test filter>]"
  echo "$empty [--keep (don't remove output files)]"
  echo "$empty [--dryrun (don't execute any tests)]"
  echo "$empty [--dir <directory>]"
  echo "$empty [--stopOnError (stop at first error encountered)]"
  echo "$empty [ <directory or file> ]"
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
    echo $ME: $*
  fi
}



# -----------------------------------------------------------------------------
#
# exitFunction
#
function exitFunction()
{
  exitCode=$1
  testFile=$2
  errorString=$3
  errorFile=$4
  forced=$5

  echo -n "(ERROR $exitCode) "

  if [ "$stopOnError" == "on" ] || [ "$forced" == "DIE" ]
  then
    echo $ME/$NAME: $errorString
    cat $errorFile 2> /dev/null
    exit $exitCode
  fi

  echo                         >> /tmp/orionFuncTestLog
  echo '----- ' $NAME ' -----' >> /tmp/orionFuncTestLog
  echo $errorString            >> /tmp/orionFuncTestLog

  if [ "$errorFile" != "" ] && [ -f "$errorFile" ]
  then
    cat $errorFile               >> /tmp/orionFuncTestLog   2> /dev/null
    echo                         >> /tmp/orionFuncTestLog
  fi

  echo                         >> /tmp/orionFuncTestLog

  testErrorV[$testError]=$testFile
  testError=$testError+1
  toBeStopped=true;
}



# ------------------------------------------------------------------------------
#
# ME - name of script, to be used in error and verbose messages 
#
ME=$(basename $0)



# ------------------------------------------------------------------------------
#
# Argument parsing
#
verbose=off
dryrun=off
keep=off
stopOnError=off
testFilter=${TEST_FILTER:-"*.test"}
dir=$SCRIPT_HOME/cases
dirOrFile=""
dirGiven=no
filterGiven=no

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];            then usage 0;
  elif [ "$1" == "-v" ];            then verbose=on;
  elif [ "$1" == "--dryrun" ];      then dryrun=on;
  elif [ "$1" == "--keep" ];        then keep=on;
  elif [ "$1" == "--stopOnError" ]; then stopOnError=on;
  elif [ "$1" == "--filter" ];      then testFilter="$2"; filterGiven=yes; shift;
  elif [ "$1" == "--dir" ];         then dir="$2"; dirGiven=yes; shift;
  else
    if [ "$dirOrFile" == "" ]
    then
      dirOrFile="$1"
    else
      echo $0: bad parameter/option: "'"${1}"'";
      echo
      usage 1
    fi
  fi
  shift
done



# ------------------------------------------------------------------------------
#
# Check unmatching --dir and 'parameter that is a directory' AND
#       unmatching --filter and 'parameter that is a file'
#
# 1. If it is a directory - just change the 'dir' variable and continue
# 2. Else, it must be a file, or a filter.
#    If the 
#
if [ "$dirOrFile" != "" ]
then
  vMsg dirOrFile: $dirOrFile
  vMsg dirGiven: $dirGiven
  vMsg filterGiven: $filterGiven
  vMsg dir: $dir
  vMsg testFilter: $testFilter

  if [ -d "$dirOrFile" ]
  then
    if [ "$dirGiven" == "yes" ]
    then
      echo "$0: both '--dir' option and directory parameter given - not allowed"
      exit 1
    fi
    dir="$dirOrFile"
  else
    if [ "$filterGiven" == "yes" ]
    then
      echo "$0: both '--filter' option and file parameter given - not allowed"
      exit 1
    fi

    #
    # If just a filename is given, keep the directory as is.
    # If a whole path is given, use the directory-part as directory and the file-part as filter
    #
    dirPart=$(dirname $dirOrFile)
    filePath=$(basename $dirOrFile)

    if [ "$dirPart" != "." ]
    then
      dir=$(dirname $dirOrFile)
      testFilter=$(basename $dirOrFile)
    else
      testFilter=$(basename $dirOrFile)
    fi
  fi
fi

vMsg directory: $dir
vMsg testFilter: $testFilter

# -----------------------------------------------------------------------------
#
# Other global variables
#
toBeStopped=false



# -----------------------------------------------------------------------------
#
# Init files already sourced?
#
if [ "$CONTEXTBROKER_TESTENV_SOURCED" != "YES" ]
then
  if [ -f scripts/testEnv.sh ]
  then
    vMsg Sourcing scripts/testEnv.sh
    source scripts/testEnv.sh
  else
    echo "--------------------------------------------------------------------------"
    echo "Please source scripts/testEnv.sh before running the functional test suite."
    echo "--------------------------------------------------------------------------"
    exit 1
  fi
fi

if [ "$CONTEXTBROKER_HARNESS_FUNCTIONS_SOURCED" != "YES" ]
then
  if [ -f $SCRIPT_HOME/harnessFunctions.sh ]
  then
    vMsg Sourcing $SCRIPT_HOME/harnessFunctions.sh
    source $SCRIPT_HOME/harnessFunctions.sh
  else
    echo "--------------------------------------------------------------------------------------------"
    echo "Please source $SCRIPT_HOME/harnessFunctions.sh before running the functional test suite."
    echo "--------------------------------------------------------------------------------------------"
    exit 1
  fi
fi



# ------------------------------------------------------------------------------
#
# Preparations - cd to the test directory
#
if [ ! -d "$dir" ]
then
  exitFunction 1 "HARNESS" "$dir is not a directory" "" DIE
fi
cd $dir

echo "Orion Functional tests starting" > /tmp/orionFuncTestLog
date >> /tmp/orionFuncTestLog



# ------------------------------------------------------------------------------
#
# Preparations - number of test cases
#
vMsg find in $(pwd), filter: $testFilter
fileList=$(find . -name "$testFilter" | sort | sed 's/^.\///')
typeset -i noOfTests
typeset -i testNo

for i in $fileList
do
  noOfTests=$noOfTests+1
done



# ------------------------------------------------------------------------------
#
# fileCleanup - 
#
function fileCleanup()
{
  filename=$1
  keepOutputFiles=$2

  if [ "$keepOutputFiles" != "on" ]
  then
    rm $filename.name               2> /dev/null
    rm $filename.shellInit          2> /dev/null
    rm $filename.shellInit.*        2> /dev/null
    rm $filename.shell              2> /dev/null
    rm $filename.shell.*            2> /dev/null
    rm $filename.teardown           2> /dev/null
    rm $filename.teardown.*         2> /dev/null
    rm $filename.valgrind.out       2> /dev/null
    rm $filename.valgrind.stop.out  2> /dev/null
    rm $filename.out                2> /dev/null
    rm $filename.regexpect          2> /dev/null
    rm $filename.diff               2> /dev/null
  fi
}



# ------------------------------------------------------------------------------
#
# fileCreation - create the files for test execution
#
function fileCreation()
{
  path=$1
  filename=$2
  ret=0

  dirname=$(dirname $path)
  filename=$(basename $path .test)
  
  if [ "$dirname" != "." ] && [ "$dirname" != "" ]
  then
    pathWithoutExt=$dirname/$filename
    vMsg New path: $path
  else
    pathWithoutExt=$filename
  fi

  vMsg Creating test files for $pathWithoutExt

  
  #
  # Extract the NAME
  #
  NAME=$(sed -n '/--NAME--/,/^--/p' $path | grep -v "^--")
  if [ "$NAME" == "" ]
  then
    exitFunction 2 "$path" "($path): --NAME-- part is missing" "" DIE
    exit 2 # Just in case
  fi

  #
  # Extract the shell init script
  #
  if [ $(grep "\-\-SHELL\-INIT\-\-" $path | wc -l) -eq 1 ]
  then
    TEST_SHELL_INIT=${pathWithoutExt}.shellInit
    vMsg "Creating $TEST_SHELL_INIT at $PWD"
    sed -n '/--SHELL-INIT--/,/^--/p' $path  | grep -v "^--" > $TEST_SHELL_INIT
  else
    exitFunction 3 $path "($path): --SHELL-INIT-- part is missing" "" DIE
  fi

  #
  # Extract the test shell script
  #
  if [ $(grep "\-\-SHELL\-\-" $path | wc -l) -eq 1 ]
  then
    TEST_SHELL=${pathWithoutExt}.shell
    vMsg "Creating $TEST_SHELL at $PWD"
    sed -n '/--SHELL--/,/^--/p' $path  | grep -v "^--" > $TEST_SHELL
  else
    exitFunction 4 $path "($path): --SHELL-- part is missing" "" DIE
  fi

  #
  # Extract the REGEXPECT part
  #
  if [ $(grep "\-\-REGEXPECT\-\-" $path | wc -l) -eq 1 ]
  then
    TEST_REGEXPECT=${pathWithoutExt}.regexpect
    vMsg "Creating $TEST_REGEXPECT at $PWD"
    sed -n '/--REGEXPECT--/,/^--/p' $path  | grep -v "^--" > $TEST_REGEXPECT
  else
    exitFunction 5 $path "($path): --REGEXPECT-- part is missing" "" DIE
  fi

  #
  # Extract the teardown script
  #
  if [ $(grep "\-\-TEARDOWN\-\-" $path | wc -l) -eq 1 ]
  then
    TEST_TEARDOWN=${pathWithoutExt}.teardown
    vMsg "Creating $TEST_TEARDOWN at $PWD"
    sed -n '/--TEARDOWN--/,/^--/p' $path  | grep -v "^--" > $TEST_TEARDOWN
  else
    exitFunction 6 $path "($path): --TEARDOWN-- part is missing" "" DIE
  fi
}



# ------------------------------------------------------------------------------
#
# partExecute
#
function partExecute()
{
  what=$1
  path=$2
  forcedDie=$3

  vMsg Executing $what part for $path
  dirname=$(dirname $path)
  filename=$(basename $path .test)
  
  if [ "$dirname" != "." ] && [ "$dirname" != "" ]
  then
    path=$dirname/$filename.test
  fi

  #
  # Prepare to execute
  #
  chmod 755 $dirname/$filename.$what
  rm -f $dirname/$filename.$what.stderr
  rm -f $dirname/$filename.$what.stdout
  $dirname/$filename.$what > $dirname/$filename.$what.stdout 2> $dirname/$filename.$what.stderr
  exitCode=$?
  linesInStderr=$(wc -l $dirname/$filename.$what.stderr | awk '{ print $1}' 2> /dev/null)

  #
  # Check that stdout is empty
  #
  if [ "$linesInStderr" != "" ] && [ "$linesInStderr" != "0" ]
  then
    exitFunction 7 $path "($path): $what produced output on stderr" $dirname/$filename.$what.stderr "$forcedDie"
    return 2
  fi


  #
  # Check that exit code is ZERO
  #
  if [ "$exitCode" != "0" ]
  then
    exitFunction 8 $path "($path): $what exited with code $exitCode" $dirname/$filename.$what.stderr "$forcedDie"
    return 1
  fi


  #
  # Compare produced output with expected output
  #
  if [ "$what" == "shell" ]
  then
    mv $dirname/$filename.$what.stdout $dirname/$filename.out # We are used to this name ...
    $DIFF -r $dirname/$filename.regexpect -i $dirname/$filename.out > $dirname/$filename.diff
    exitCode=$?
    if [ "$exitCode" != "0" ]
    then
      exitFunction 9 $path "($path) output not as expected" $dirname/$filename.diff
      if [ "$CB_DIFF_TOOL" != "" ]
      then
        endDate=$(date)
        $CB_DIFF_TOOL $dirname/$filename.regexpect $dirname/$filename.out
      fi
    fi
  fi
}



# ------------------------------------------------------------------------------
#
# runTest - the function that runs ONE test case
#
# 1.    Remove old output files
# 2.1.  Create the various test files from '$path'
# 3.1.  Run the SHELL-INIT part
# 3.2.  If [ $? != 0 ] || [ STDERR != empty ]   ERROR
# 4.1.  Run the SHELL part
# 4.2.  If [ $? != 0 ] || [ STDERR != empty ]   ERROR
# 5.1.  Run the TEARDOWN part
# 5.2.  If [ $? != 0 ] || [ STDERR != empty ]   ERROR
# 6.1.  Compare output with regexpect (or expect)
# 6.2.  Not EQUAL: ERROR
# 7.    If [ "$keep" != "yes" ]  Remove all output files
#
#
function runTest()
{
  path=$1
  dirname=$(dirname $path)
  filename=$(basename $path .test)
  
  if [ "$dirname" != "." ] && [ "$dirname" != "" ]
  then
    path=$dirname/$filename.test
    vMsg New path: $path
  fi

  vMsg running test $path

  # 1. Remove old output files
  fileCleanup $filename removeAll
  if [ "$toBeStopped" == "yes" ]
  then
    return
  fi

  # 2. Create the various test files from '$path'
  fileCreation $path $filename
  if [ "$toBeStopped" == "yes" ]
  then
    return
  fi

  # 3. Run the SHELL-INIT part
  vMsg Executing SHELL-INIT part for $path
  chmod 755 $dirname/$filename.shellInit
  rm -f $dirname/$filename.shellInit.stderr
  rm -f $dirname/$filename.shellInit.stdout
  $dirname/$filename.shellInit > $dirname/$filename.shellInit.stdout 2> $dirname/$filename.shellInit.stderr
  exitCode=$?
  linesInStderr=$(wc -l $dirname/$filename.shellInit.stderr | awk '{ print $1}' 2> /dev/null)

  if [ "$linesInStderr" != "" ] && [ "$linesInStderr" != "0" ]
  then
    exitFunction 10 $path "($path): SHELL-INIT produced output on stderr" $dirname/$filename.shellInit.stderr
    return 2
  fi

  if [ "$exitCode" != "0" ]
  then
    exitFunction 11 $path "($path): SHELL-INIT exited with code $exitCode" "" DIE
    return 1
  fi

  # 4. Run the SHELL part (which also compares - FIXME P2: comparison should be moved to separate function)
  partExecute shell $path "DontDie - only for SHELL-INIT"
  if [ "$toBeStopped" == "yes" ]
  then
    return
  fi

  # 5. Run the TEARDOWN part
  partExecute teardown $path "DIE"

  # 6. Remove output files
  vMsg "Remove output files: fileCleanup $filename $keep"
  fileCleanup $filename $keep
}

# ------------------------------------------------------------------------------
#
# Main loop
#
vMsg Total number of tests: $noOfTests
testNo=1
for testFile in $fileList
do
  if [ "$verbose" == "off" ]
  then
    printf "%03d/%d: %s ... " "$testNo" "$noOfTests" "$testFile"
  else
    printf "Running test %03d/%d: %s\n" "$testNo" "$noOfTests" "$testFile"
  fi

  testNo=$testNo+1
  startDate=$(date)
  start=$(date --date="$startDate" +%s)
  endDate=""
  runTest $testFile

  if [ "$endDate" == "" ]  # Could have been set in 'partExecute'
  then
    endDate=$(date)
  fi

  end=$(date --date="$endDate" +%s)
  typeset -i secs
  secs=$end-$start
  echo $secs seconds
done



# ------------------------------------------------------------------------------
#
# Check for errors - if any, print to stdout
#
if [ "$testError" != "0" ]
then
  echo
  echo "Orion Functional Test Log File:"
  echo "================================================================================"
  cat /tmp/orionFuncTestLog 2> /dev/null
  echo "================================================================================"
  echo
  echo "----------- Failing tests ------------------"
  typeset -i ix
  ix=0
  while [ "$ix" != "$testError" ]
  do
    echo "  o " ${testErrorV[$ix]}
    ix=$ix+1
  done

  exit 1
fi

exit 0
