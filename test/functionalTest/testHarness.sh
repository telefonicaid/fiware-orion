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

date
testStartTime=$(date +%s.%2N)
MAX_TRIES=${CB_MAX_TRIES:-3}



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
export SCRIPT_HOME=$(pwd)
cd - > /dev/null 2>&1



# ------------------------------------------------------------------------------
#
# Debug mode?
#
if [ "$ORION_FT_DEBUG" == "1" ]
then
  _debug='on'
fi



# -----------------------------------------------------------------------------
#
# Log file for debugging
#
rm -f /tmp/orionFuncTestDebug.log
echo $(date) > /tmp/orionFuncTestDebug.log



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
  echo "$empty [--match <string for test to match>]"
  echo "$empty [--keep (don't remove output files)]"
  echo "$empty [--dryrun (don't execute any tests)]"
  echo "$empty [--dir <directory>]"
  echo "$empty [--fromIx <index of test where to start>]"
  echo "$empty [--ixList <list of test indexes>]"
  echo "$empty [--stopOnError (stop at first error encountered)]"
  echo "$empty [--no-duration (removes duration mark on successful tests)]"
  echo "$empty [ <directory or file> ]*"
  echo
  echo "* Please note that if a directory is passed as parameter, its entire path must be given, not only the directory-name"
  echo "* If a file is passed as parameter, its entire file-name must be given, including '.test'"
  echo
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
  errorText=$2
  testFile=$3
  errorString=$4
  errorFile=$5
  forced=$6

  echo -n "(ERROR $exitCode - $errorText) "

  if [ "$stopOnError" == "on" ] || [ "$forced" == "DIE" ]
  then
    echo $ME/$NAME: $errorString

    if [ "$errorFile" != "" ] && [ -f "$errorFile" ]
    then
      cat $errorFile 2> /dev/null
    fi

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
vMsg "$ME, in directory $SCRIPT_HOME"



# ------------------------------------https://github.com/telefonicaid/fiware-orion/pull/394#discussion_r13321709------------------------------------------
#
# Argument parsing
#
typeset -i fromIx
verbose=off
dryrun=off
keep=off
stopOnError=off
testFilter=${TEST_FILTER:-"*.test"}
match=""
dir=$SCRIPT_HOME/cases
dirOrFile=""
dirGiven=no
filterGiven=no
showDuration=on
fromIx=0
ixList=""

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];            then usage 0;
  elif [ "$1" == "-v" ];            then verbose=on;
  elif [ "$1" == "--dryrun" ];      then dryrun=on;
  elif [ "$1" == "--keep" ];        then keep=on;
  elif [ "$1" == "--stopOnError" ]; then stopOnError=on;
  elif [ "$1" == "--filter" ];      then testFilter="$2"; filterGiven=yes; shift;
  elif [ "$1" == "--match" ];       then match="$2"; shift;
  elif [ "$1" == "--dir" ];         then dir="$2"; dirGiven=yes; shift;
  elif [ "$1" == "--fromIx" ];      then fromIx=$2; shift;
  elif [ "$1" == "--ixList" ];      then ixList=$2; shift;
  elif [ "$1" == "--no-duration" ]; then showDuration=off;
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

vMsg "options parsed"


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
    xdir=$(basename $dirPart);
    vMsg "dirPart: $dirPart"
    vMsg "filePath: $filePath"

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
vMsg "Script in $SCRIPT_HOME"



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
  if [ -f "$SCRIPT_HOME/testEnv.sh" ]
  then
    # First, we try with a testEnv.sh file in the script home (usual situation in the
    # RPM deployment case)
    vMsg Sourcing $SCRIPT_HOME/testEnv.sh
    source $SCRIPT_HOME/testEnv.sh
  elif [ -f "$SCRIPT_HOME/../../scripts/testEnv.sh" ]
  then
    # Second, we try with a testEnv.sh file in the script/testEnv.sh (realtive to git repo home).
    # Note that the script home in this case is test/functionaTest
    vMsg Sourcing $SCRIPT_HOME/../../scripts/testEnv.sh
    source $SCRIPT_HOME/../../scripts/testEnv.sh
  else
    echo "------------------------------------------------------------------"
    echo "Please source testEnv.sh before running the functional test suite."
    echo "------------------------------------------------------------------"
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
dMsg Functional Tests Starting ...
if [ "$dirOrFile" != "" ] && [ -d "$dirOrFile" ]
then
  cd $dirOrFile
elif [ ! -d "$dir" ]
then
  exitFunction 1 "$dir is not a directory" "HARNESS" "$dir" "" DIE
else
  cd $dir
fi


echo "Orion Functional tests starting" > /tmp/orionFuncTestLog
date >> /tmp/orionFuncTestLog



# ------------------------------------------------------------------------------
#
# Preparations - number of test cases
#
vMsg find in $(pwd), filter: $testFilter
if [ "$match" == "" ]
then
  fileList=$(find . -name "$testFilter" | sort | sed 's/^.\///')
else
  fileList=$(find . -name "$testFilter" | grep "$match" | sort | sed 's/^.\///')
fi
vMsg "fileList: $fileList"
typeset -i noOfTests
typeset -i testNo



# ------------------------------------------------------------------------------
#
# Count total number of tests (for progressing info in messages)
#
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
  path=$3
  dir=$(dirname $path)

  vMsg "---------------------------------------------------------"
  vMsg "In fileCleanup for $filename in $dir"
  vMsg "---------------------------------------------------------"

  if [ "$keepOutputFiles" != "on" ]
  then
    olddir=$PWD
    cd $dir

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
    rm $filename.out.sorted         2> /dev/null
    rm $filename.regexpect.sorted   2> /dev/null
    rm $filename.blockSortDiff.out  2> /dev/null
    rm $filename.diff               2> /dev/null

    cd $olddir
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
    exitFunction 2 "--NAME-- part is missing" "$path" "($path)" "" DIE
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
    exitFunction 3 "--SHELL-INIT-- part is missing" $path "($path)" "" DIE
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
    exitFunction 4 "--SHELL-- part is missing" $path "($path)" "" DIE
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
    exitFunction 5 "--REGEXPECT-- part is missing" $path "($path)" "" DIE
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
    exitFunction 6 "--TEARDOWN-- part is missing" $path "($path)" "" DIE
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
  __tryNo=$4

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
    if [ $__tryNo == $MAX_TRIES ]
    then
      exitFunction 7 "$what: output on stderr" $path "($path): $what produced output on stderr" $dirname/$filename.$what.stderr "$forcedDie"
    else
      echo -n "(ERROR 7 - $what: output on stderr) "
    fi

    partExecuteResult=7
    return
  fi


  #
  # Check that exit code is ZERO
  #
  if [ "$exitCode" != "0" ]
  then
    if [ $__tryNo == $MAX_TRIES ]
    then
      exitFunction 8 $path "$what exited with code $exitCode" "($path)" $dirname/$filename.$what.stderr "$forcedDie"
    else
      echo -n "(ERROR 8 - $what: exited with code $exitCode) "
    fi

    partExecuteResult=8
    return
  fi


  #
  # Compare produced output with expected output
  #
  if [ "$what" == "shell" ]
  then
    mv $dirname/$filename.$what.stdout $dirname/$filename.out # We are very much used to this name ...

    #
    # Special sorted diff or normal REGEX diff ?
    #
    blockDiff='no'
    grep '^#SORT_START$' $dirname/$filename.regexpect > /dev/null 2>&1
    if [ $? == 0 ]
    then
      $SCRIPT_HOME/blockSortDiff.sh --referenceFile $dirname/$filename.regexpect --brokerOutputFile $dirname/$filename.out > $dirname/$filename.blockSortDiff.out
      exitCode=$?
      blockDiff='yes'
    else
      $DIFF -r $dirname/$filename.regexpect -i $dirname/$filename.out > $dirname/$filename.diff
      exitCode=$?
    fi

    if [ "$exitCode" != "0" ]
    then
      if [ $__tryNo == $MAX_TRIES ]
      then
        exitFunction 9 ".out and .regexpect differ" $path "($path) output not as expected" $dirname/$filename.diff
      else
        echo -n "(ERROR 9 - .out and .regexpect differ) "
      fi

      if [ "$CB_DIFF_TOOL" != "" ] && [ $__tryNo == $MAX_TRIES ]
      then
        endDate=$(date)
        if [ $blockDiff == 'yes' ]
        then
          $CB_DIFF_TOOL $dirname/$filename.regexpect.sorted $dirname/$filename.out.sorted
        else
          $CB_DIFF_TOOL $dirname/$filename.regexpect $dirname/$filename.out
        fi
      fi
      partExecuteResult=9
      return
    fi
  fi

  partExecuteResult=0
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
  _tryNo=$2

  runTestStatus="ok"

  vMsg path=$path
  dirname=$(dirname $path)
  filename=$(basename $path .test)
  dir=""

  if [ "$dirname" != "." ] && [ "$dirname" != "" ]
  then
    path=$dirname/$filename.test
    vMsg New path: $path
  fi

  vMsg running test $path

  # 1. Remove old output files
  fileCleanup $filename removeAll $path
  if [ "$toBeStopped" == "yes" ]
  then
    echo toBeStopped == yes
    runTestStatus="stopped"
    return
  fi

  # 2. Create the various test files from '$path'
  fileCreation $path $filename
  if [ "$toBeStopped" == "yes" ]
  then
    runTestStatus="stopped2"
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
    exitFunction 10 "SHELL-INIT produced output on stderr" $path "($path)" $dirname/$filename.shellInit.stderr
    runTestStatus="shell-init-error"
    return
  fi

  if [ "$exitCode" != "0" ]
  then

    #
    # 3.2 Run the SHELL-INIT part AGAIN
    #
    # This 're-run' of the SHELL-INIT part is due to errors we've seen that seem to be caused by
    # a try to start a broker while the old one (from the previous functest) is still running.
    # No way to test this, except with some patience.
    #
    # We have seen 'ERROR 11' around once every 500-1000 functests (the suite is of almost 400 tests)
    # and this fix, if working, will make us not see those 'ERROR 11' again.
    # If we keep seeing 'ERROR 11' after this change then we will need to investigate further.
    #
    sleep 1
    rm -f $dirname/$filename.shellInit.stderr
    rm -f $dirname/$filename.shellInit.stdout
    $dirname/$filename.shellInit > $dirname/$filename.shellInit.stdout 2> $dirname/$filename.shellInit.stderr
    exitCode=$?
    linesInStderr=$(wc -l $dirname/$filename.shellInit.stderr | awk '{ print $1}' 2> /dev/null)

    if [ "$linesInStderr" != "" ] && [ "$linesInStderr" != "0" ]
    then
      exitFunction 20 "SHELL-INIT II produced output on stderr" $path "($path)" $dirname/$filename.shellInit.stderr
      runTestStatus="shell-init-output-on-stderr"
      return
    fi

    if [ "$exitCode" != "0" ]
    then
      exitFunction 11 "SHELL-INIT exited with code $exitCode" $path "($path)" "" DIE
      runTestStatus="shell-init-exited-with-"$exitCode
      return
    fi
  fi

  # 4. Run the SHELL part (which also compares - FIXME P2: comparison should be moved to separate function)
  partExecute shell $path "DontDie - only for SHELL-INIT" $_tryNo
  shellResult=$partExecuteResult
  if [ "$toBeStopped" == "yes" ]
  then
    runTestStatus="shell-failed"
    return
  fi

  # 5. Run the TEARDOWN part
  partExecute teardown $path "DIE" 0
  teardownResult=$partExecuteResult
  vMsg "teardownResult: $teardownResult"
  vMsg "shellResult: $shellResult"

  if [ "$shellResult" == "0" ] && [ "$teardownResult" == "0" ]
  then
    # 6. Remove output files
    vMsg "Remove output files: fileCleanup $filename $keep"
    fileCleanup $filename $keep $path
  else
    file=$(basename $path .test)
    cp /tmp/contextBroker.log $file.contextBroker.log
    runTestStatus="test-failed"
  fi
}



# ------------------------------------------------------------------------------
#
# Main loop
#
vMsg Total number of tests: $noOfTests
testNo=0
for testFile in $fileList
do
  if [ -d "$testFile" ]
  then
    continue
  fi

  testNo=$testNo+1

  if [ $fromIx != 0 ] && [ $testNo -lt $fromIx ]
  then
    continue;
  fi

  if [ "$ixList" != "" ]
  then
    hit=$(echo ' '$ixList' ' | grep ' '$testNo' ')
    if [ "$hit" == "" ]
    then
      continue
    fi
  fi

  startDate=$(date)
  start=$(date --date="$startDate" +%s)
  endDate=""
  typeset -i tryNo
  tryNo=1

  if [ "$dryrun" == "off" ]
  then
    while [ $tryNo -le $MAX_TRIES ]
    do
      if [ "$verbose" == "off" ]
      then
        tryNoInfo=""
        if [ $tryNo != "1" ]
        then
           tryNoInfo="(intent $tryNo)"
        fi

        init=$testFile" ................................................................................................................."
        init=${init:0:110}
        printf "%03d/%d: %s %s " "$testNo" "$noOfTests" "$init" "$tryNoInfo"
      else
        printf "Running test %03d/%d: %s\n" "$testNo" "$noOfTests" "$testFile"
      fi

      runTest $testFile $tryNo
      if [ $shellResult == "0" ]
      then
        if [ $tryNo != 1 ]
        then
          echo "OK"
        fi
        break
      else
        tryNo=$tryNo+1
        echo
      fi
    done
  else
    if [ "$verbose" == "off" ]
    then
      init=$testFile" ................................................................................................................."
      init=${init:0:110}
      printf "%03d/%d: %s " "$testNo" "$noOfTests" "$init"
    else
      printf "Running test %03d/%d: %s\n" "$testNo" "$noOfTests" "$testFile"
    fi
  fi

  if [ "$endDate" == "" ]  # Could have been set in 'partExecute'
  then
    endDate=$(date)
  fi

  end=$(date --date="$endDate" +%s)
  typeset -i secs
  secs=$end-$start
  if [ "$showDuration" == "on" ]
  then
    if [ $secs -lt 10 ]
    then
      xsecs=0$secs
    else
      xsecs=$secs
    fi
    echo $xsecs seconds
  else
    echo "SUCCESS"
  fi
done


testEndTime=$(date +%s.%2N)
testDiffTime=$(echo $testEndTime - $testStartTime | bc)" seconds"
echo Total test time: $testDiffTime


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
