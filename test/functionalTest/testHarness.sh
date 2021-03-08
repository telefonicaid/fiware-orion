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
echo $testStartTime > /tmp/brokerStartCounter


# -----------------------------------------------------------------------------
#
# DISABLED - funct tests that are disabled, for some reason
#
DISABLED=('test/functionalTest/cases/0000_bad_requests/exit.test' \
          'test/functionalTest/cases/0917_queryContext_behaves_differently/query_with_and_without_forwarding.test' \
          'test/functionalTest/cases/0000_ipv6_support/ipv4_only.test' \
          'test/functionalTest/cases/0000_ipv6_support/ipv6_only.test' \
          'test/functionalTest/cases/1310_suspect_200OK/suspect_200OK.test');



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
declare -A okOnSecondV
typeset -i okOnSecond
declare -A okOnThirdV
typeset -i okOnThird
declare -A okOnPlus3V
typeset -i okOnPlus3
declare -A skipV
typeset -i skips
declare -A disabledTestV
typeset -i disabledTests

export DIFF=$SCRIPT_HOME/testDiff.py
testError=0
okOnSecond=0
okOnThird=0
okOnPlus3=0
skips=0
disabledTests=0


# -----------------------------------------------------------------------------
#
# Default value of skipList taken from an env var, to make things a little
# easier in distros with constantly failing tests
#
skipList="$CB_SKIP_LIST"



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
  echo "$empty [--fromIx <index of test where to start (inclusive)>]"
  echo "$empty [--toIx <index of test where to end (inclusive)>]"
  echo "$empty [--ixList <list of test indexes>]"
  echo "$empty [--skipList <list of indexes of test cases to be skipped>]"
  echo "$empty [--stopOnError (stop at first error encountered)]"
  echo "$empty [--no-duration (removes duration mark on successful tests)]"
  echo "$empty [--noCache (force broker to be started with the option --noCache)]"
  echo "$empty [--cache (force broker to be started without the option --noCache)]"
  echo "$empty [--noThreadpool (do not use a threadpool, unless specified by a test case. If not set, a thread pool of 200:20 is used by default in test cases which do not set notificationMode options)]"
  echo "$empty [--xbroker (use external brokers, i.e. this script will *not* start any brokers, including context providers)]"
  echo "$empty [ <directory or file> ]*"
  echo
  echo "* Please note that if a directory is passed as parameter, its entire path must be given, not only the directory-name"
  echo "* If a file is passed as parameter, its entire file-name must be given, including '.test'"
  echo ""
  echo "Env Vars:"
  echo "CB_MAX_TRIES:             The number of tries before giving up on a failing test case"
  echo "CB_SKIP_LIST:             Default value for option --skipList"
  echo "CB_SKIP_FUNC_TESTS:       List of names of func tests to skip"
  echo "CB_NO_CACHE:              Start the broker without subscription cache (if set to 'ON')"
  echo "CB_THREADPOOL:            Start the broker without thread pool (if set to 'OFF')"
  echo "CB_DIFF_TOOL:             To view diff of failing tests with diff/tkdiff/meld/... (e.g. export CB_DIFF_TOOL=tkdiff)"
  echo "CB_WITH_EXTERNAL_BROKER:  The broker is started externally - not 'automatically' by the test harness (if set to 'ON')"
  echo ""
  echo "FT_FROM_IX: alternative to commandline parameter  'fromIx', index of test where to start (inclusive) "
  echo "FT_TO_IX: alternative to commandline parameter  'toIx', index of test where to end (inclusive)"
  echo
  echo "Please note that, if using CB_WITH_EXTERNAL_BROKER (or --xbroker, which is the same), only a single test case should be run."
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

  echo -n "(FAIL $exitCode - $errorText) "

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
typeset -i toIx
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
toIx=0
ixList=""
noCache=""
threadpool=ON
xbroker=off

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];             then usage 0;
  elif [ "$1" == "-v" ];             then verbose=on;
  elif [ "$1" == "--dryrun" ];       then dryrun=on;
  elif [ "$1" == "--keep" ];         then keep=on;
  elif [ "$1" == "--stopOnError" ];  then stopOnError=on;
  elif [ "$1" == "--filter" ];       then testFilter="$2"; filterGiven=yes; shift;
  elif [ "$1" == "--match" ];        then match="$2"; shift;
  elif [ "$1" == "--dir" ];          then dir="$2"; dirGiven=yes; shift;
  elif [ "$1" == "--fromIx" ];       then fromIx=$2; shift;
  elif [ "$1" == "--toIx" ];         then toIx=$2; shift;
  elif [ "$1" == "--ixList" ];       then ixList=$2; shift;
  elif [ "$1" == "--skipList" ];     then skipList=$2; shift;
  elif [ "$1" == "--no-duration" ];  then showDuration=off;
  elif [ "$1" == "--noCache" ];      then noCache=ON;
  elif [ "$1" == "--cache" ];        then noCache=OFF;
  elif [ "$1" == "--noThreadpool" ]; then threadpool=OFF;
  elif [ "$1" == "--xbroker" ];      then xbroker=ON;
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

# -----------------------------------------------------------------------------
#
# The function brokerStart looks at the env var CB_NO_CACHE to decide
# whether to start the broker with the --noCache option or not
#
if [ "$noCache" != "" ]
then
  export CB_NO_CACHE=$noCache
fi

# -----------------------------------------------------------------------------
#
# The function brokerStart looks at the env var CB_THREADPOOL to decide
# whether to start the broker with pool of threads or not.
# Do not overwrite if a value is passed from environment
#
if [ "$CB_THREADPOOL" == "" ]
then
  export CB_THREADPOOL=$threadpool
fi

# -----------------------------------------------------------------------------
#
# Check if fromIx is set through an env var and use if nothing
# else is set through commandline parameter
#
if [ "$FT_FROM_IX" != "" ] && [ $fromIx == 0 ]
then
  fromIx=$FT_FROM_IX
fi

# -----------------------------------------------------------------------------
#
# Check if toIx is set through an env var and use if nothing
# else is set through commandline parameter
#
if [ "$FT_TO_IX" != "" ] && [ $toIx == 0 ]
then
  toIx=$FT_TO_IX
fi

echo "Run tests $fromIx to $toIx"

# ------------------------------------------------------------------------------
#
# Check unmatching --dir and 'parameter that is a directory' AND
#       unmatching --filter and 'parameter that is a file'
#
# 1. If it is a directory - just change the 'dir' variable and continue
# 2. Else, it must be a file, or a filter.
#    If the 
#
singleFile=No
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

    singleFile=Yes
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

      # Last dir + test file ?
      if [ -d test/functionalTest/cases/$dirPart ]
      then
          dirOrFile=test/functionalTest/cases/$dirPart
      fi
    else
      testFilter=$(basename $dirOrFile)
    fi
  fi
fi

#
# The option of running against an external broker "--xbroker" only works (for now, at least) with a single test case.
# Strange things may happen (due to the state inside the broker) if more that one test case are launched.
# This check avoid this situation.
#
# If in the future we want to be able to run more than one test case against an external broker, we'd need to make sure
# that each test case undoes all internal state inside the external broker. E.g. delete subscriptions, entities, etc.
#
if [ "$singleFile" == "No" ] && [ "$xbroker" == "ON" ]
then
    echo "External broker can only be used with individual test cases"
    exit 1
fi

vMsg directory: $dir
vMsg testFilter: $testFilter
vMsg "Script in $SCRIPT_HOME"



# -----------------------------------------------------------------------------
#
# Other global variables
#
toBeStopped=false



# ------------------------------------------------------------------------------
#
# xbroker - if this CLI is set, then the broker is not to be started as part of
#           the test suite - another broker is assumed to be running already
#
if [ "$xbroker" == "ON" ]
then
    export CB_WITH_EXTERNAL_BROKER=ON
fi



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
    # Note that the script home in this case is test/functionalTest
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



# -----------------------------------------------------------------------------
#
# testDisabled
#
function testDisabled
{
  testcase=$1
  typeset -i dIx
  dIx=0
  while [ $dIx -lt  ${#DISABLED[@]} ]
  do
    if [ test/functionalTest/cases/$testcase == ${DISABLED[$dIx]} ]
    then
      echo "Disabled"

      #
      # NOTE: In a non-disabled test, running inside the valgrind test suite, the function 'localBrokerStart()' (from harnessFunctions.sh)
      #       redirects the output of "valgrind contextBroker" to the file /tmp/valgrind.out.
      #       Later, the valgrind test suite uses the existence of this file (/tmp/valgrind.out) to detect errors in the valgrind execution.
      #       But, in the case of a disabled func test, we will not start the test case. and thus we will not reach 'localBrokerStart()', so the
      #       file will not be created and an error will be flagged by the valgrind test suite.
      #       The simplest solution is to simply create the file here, in the case of a disabled test.
      #
      echo "Disabled" > /tmp/valgrind.out
      return
    fi
    dIx=$dIx+1
  done
  echo NOT Disabled
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

  if [ $toIx != 0 ] && [ $testNo -gt $toIx ]
  then
    continue;
  fi

  #
  # Disabled test?
  #
  disabled=$(testDisabled $testFile)
  if [ "$disabled" == "Disabled" ]
  then
    disabledTestV[$disabledTests]=$testNo': '$testFile
    disabledTests=$disabledTests+1
    continue
  fi

  if [ "$ixList" != "" ]
  then
    hit=$(echo ' '$ixList' ' | grep ' '$testNo' ')
    if [ "$hit" == "" ]
    then
      # Test case not found in ix-list, so it is not executed
      continue
    fi
  fi

  if [ "$CB_SKIP_FUNC_TESTS" != "" ]
  then
    hit=$(echo ' '$CB_SKIP_FUNC_TESTS' ' | grep ' '$testFile' ')
    if [ "$hit" != "" ]
    then
      # Test case found in skip-list, so it is skipped
      skipV[$skips]=$testNo': '$testFile
      skips=$skips+1
      continue
    fi
  fi

  if [ "$skipList" != "" ]
  then
    hit=$(echo ' '$skipList' ' | grep ' '$testNo' ')
    if [ "$hit" != "" ]
    then
      # Test case found in skip-list, so it is skipped
      skipV[$skips]=$testNo': '$testFile
      skips=$skips+1
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
        printf "%04d/%d: %s %s " "$testNo" "$noOfTests" "$init" "$tryNoInfo"
      else
        printf "Running test %04d/%d: %s\n" "$testNo" "$noOfTests" "$testFile"
      fi

      runTest $testFile $tryNo
      if [ "$shellResult" == "0" ]
      then
        if [ $tryNo != 1 ]
        then
          if [ $tryNo == 2 ]
          then
            okOnSecondV[$okOnSecond]=$testFile
            okOnSecond=$okOnSecond+1
          elif [ $tryNo == 3 ]
          then
            okOnThirdV[$okOnThird]=$testFile
            okOnThird=$okOnThird+1
          else
            okOnPlus3V[$okOnPlus3]=$testFile
            okOnPlus3=$okOnPlus3+1
          fi
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
      printf "%04d/%d: %s " "$testNo" "$noOfTests" "$init"
    else
      printf "Running test %04d/%d: %s\n" "$testNo" "$noOfTests" "$testFile"
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


typeset -i ix
exitCode=0
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

  ix=0
  while [ $ix -lt $testError ]
  do
    echo "  o " ${testErrorV[$ix]}
    ix=$ix+1
  done
  exitCode=1
fi



# ------------------------------------------------------------------------------
#
# Check for reintents
#
if [ "$okOnSecond" != "0" ]
then
  echo
  echo "$okOnSecond test cases OK in the second attempt:"

  ix=0
  while [ $ix -lt $okOnSecond ]
  do
    echo "  o " ${okOnSecondV[$ix]}
    ix=$ix+1
  done
fi

if [ "$okOnThird" != "0" ]
then
  echo
  echo "$okOnThird test cases OK in the third attempt:"

  ix=0
  while [ $ix -lt $okOnThird ]
  do
    echo "  o " ${okOnThirdV[$ix]}
    ix=$ix+1
  done
fi

if [ "$okOnPlus3" != "0" ]
then
  echo
  echo "$okOnPlus3 test cases OK after three or more failed attempts:"

  ix=0
  while [ $ix -lt $okOnPlus3 ]
  do
    echo "  o " ${okOnPlus3V[$ix]}
    ix=$ix+1
  done
fi

if [ $skips != 0 ]
then
  echo
  echo WARNING: $skips test cases skipped:
  ix=0
  while [ $ix -lt $skips ]
  do
    echo "  o " ${skipV[$ix]}
    ix=$ix+1
  done
fi

if [ $disabledTests != 0 ]
then
  echo
  echo WARNING: $disabledTests test cases disabled:
  ix=0
  while [ $ix -lt $disabledTests ]
  do
    echo "  o " ${disabledTestV[$ix]}
    ix=$ix+1
  done
fi

exit $exitCode
