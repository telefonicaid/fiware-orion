#!/bin/bash
# -*- coding: latin-1 -*-
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
#
# Author: Ken Zangelin



# -----------------------------------------------------------------------------
#
# usage
#
function usage()
{
  echo $0 "[-u (usage)] [-v (verbose)] [--filter <test filter>] [--keep (don't remove output files)] [--dryrun (don't execute any tests)] [--dir <directory>]"
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
    echo $YO: $*
  fi
}



# -----------------------------------------------------------------------------
#
# errorMsg
#
function errorMsg()
{
  echo ERROR
  echo $YO: $*
  exit 1
}



# ------------------------------------------------------------------------------
#
# YO - name of script, to be used in error and verbose messages 
#
YO=$(basename $0)



# ------------------------------------------------------------------------------
#
# Argument parsing
#
verbose=off
dryrun=off
keep=off
testFilter=${TEST_FILTER:-"*.test"}
dir=test/testharness

vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];         then usage 0;
  elif [ "$1" == "-v" ];         then verbose=on;
  elif [ "$1" == "--dryrun" ];   then dryrun=on;
  elif [ "$1" == "--keep" ];     then keep=on;
  elif [ "$1" == "--filter" ];   then testFilter=$2; shift;
  elif [ "$1" == "--dir" ];      then dir=$2; shift;
  else
    echo $0: bad parameter/option: "'"${1}"'";
    usage 1
  fi
  shift
done



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
  if [ -f test/testharness/harnessFunctions.sh ]
  then
    vMsg Sourcing test/testharness/harnessFunctions.sh
    source test/testharness/harnessFunctions.sh
  else
    echo "--------------------------------------------------------------------------------------------"
    echo "Please source test/testharness/harnessFunctions.sh before running the functional test suite."
    echo "--------------------------------------------------------------------------------------------"
    exit 1
  fi
fi



# ------------------------------------------------------------------------------
#
# Preparations
#
if [ ! -d $dir ]
then
  errorMsg "$dir is not a directory"
fi
cd $dir



# ------------------------------------------------------------------------------
#
# Number of test cases
#
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
  toKeep=$2

  rm $filename.name                 2> /dev/null
  rm $filename.shellInit.*          2> /dev/null
  rm $filename.shell.*              2> /dev/null
  rm $filename.teardown.*           2> /dev/null
  rm $filename.valgrind.out         2> /dev/null
  rm $filename.valgrind.stop.out    2> /dev/null

  if [ "$toKeep" != "yes" ]
  then
    rm $filename.out                2> /dev/null
    rm $filename.expect             2> /dev/null
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

  vMsg Creating test files for $filename

  
  #
  # Extract the NAME
  #
  NAME=$(sed -n '/--NAME--/,/^--/p' $path | grep -v "^--")
  if [ "$NAME" == "" ]
  then
    errorMsg "($path): --NAME-- part is missing"
  fi

  #
  # Extract the shell init script
  #
  if [ $(grep "\-\-SHELL\-INIT\-\-" $path | wc -l) -eq 1 ]
  then
    TEST_SHELL_INIT=${filename}.shellInit
    vMsg "Creating $TEST_SHELL_INIT at $PWD"
    sed -n '/--SHELL-INIT--/,/^--/p' $path  | grep -v "^--" > $TEST_SHELL_INIT
  else
    errorMsg "($path): --SHELL-INIT-- part is missing"
  fi
}



# ------------------------------------------------------------------------------
#
# runTest - the function that runs ONE test case
#
# 1.    Remove old output files
# 2.1.  Create the various test files from '$path'
# 2.2.  Anything missing?  ERROR
# 3.1.  Get the NAME from the TEST case
# 3.2.  No NAME?  ERROR    
# 4.1.  Run the SHELL-INIT part
# 4.2.  If [ $? != 0 ] || [ STDERR != empty ]   ERROR
# 5.1.  Run the SHELL part
# 5.2.  If [ $? != 0 ] || [ STDERR != empty ]   ERROR
# 6.1.  Run the TEARDOWN part
# 6.2.  If [ $? != 0 ] || [ STDERR != empty ]   ERROR
# 7.1.  Compare output with regexpect (or expect)
# 7.2.  Not EQUAL: ERROR
# 8.    If [ "$keep" != "yes" ]  Remove all output files
#
#
function runTest()
{
  path=$1
  filename=$(basename $path .test)
  vMsg running test $path
  # 1. Remove old output files
  fileCleanup $filename removeAll

  # 2.1 Create the various test files from '$path'
  fileCreation $path $filename
}


# ------------------------------------------------------------------------------
#
# Main loop
#
vMsg Total number of tests: $noOfTests
for testFile in $fileList
do
  if [ "$verbose" == "off" ]
  then
    printf "%03d/%d: %s ... " "$testNo" "$noOfTests" "$testFile"
  else
    printf "Running test %03d/%d: %s\n" "$testNo" "$noOfTests" "$testFile"
  fi

  testNo=$testNo+1
  runTest $testFile
  exit 12
done

exit 0

function setup_test_vars
{
    log $FUNCNAME "Entering setup_test_vars"
    HAVE_SHELL_INIT=0
    HAVE_COMMAND=0
    HAVE_SCRIPT=0
    HAVE_SHELL=0
    INIT_SHELL_ERROR=0
    MULTIPLE_TESTS=0
    TEST_FAILED=0
    HAVE_EXPECT=0
    HAVE_REGEXPECT=0
    HAVE_TEARDOWN=0
    TEST_SHELL_ERROR=0

    TEST_BASENAME=$(echo $TEST_FILE | sed 's/\.test//')
    TEST_NAME=$(sed -n '/--NAME--/,/^--/p' $TEST_FILE  | grep -v "^--")

    #Extract the shell init script
    if [ $(grep "\-\-SHELL\-INIT\-\-" $TEST_FILE | wc -l) -eq 1 ]; then
        TEST_SHELL_INIT=${TEST_BASENAME}.sh_init
        sed -n '/--SHELL-INIT--/,/^--/p' $TEST_FILE  | grep -v "^--" > $TEST_SHELL_INIT
        HAVE_SHELL_INIT=1
    fi

    #Extract the shell script
    if [ $(grep "\-\-SHELL\-\-" $TEST_FILE | wc -l) -eq 1 ]; then
        TEST_SHELL_SCRIPT=${TEST_BASENAME}.sh
        sed -n '/--SHELL--/,/^--/p' $TEST_FILE  | grep -v "^--" > $TEST_SHELL_SCRIPT
        HAVE_SHELL=1
    fi

    #Generate the static canon file
    if [ $(grep "\-\-EXPECT\-\-" $TEST_FILE | wc -l) -eq 1 ]; then
        TEST_EXPECT=${TEST_BASENAME}.expect
        sed -n '/--EXPECT--/,/^--/p' $TEST_FILE  | grep -v "^--" > $TEST_EXPECT
        HAVE_EXPECT=1
    fi

    #Generate the dynamic canon file
    if [ $(grep "\-\-REGEXPECT\-\-" $TEST_FILE | wc -l) -eq 1 ]; then
        TEST_REGEXPECT=${TEST_BASENAME}.regexpect
        sed -n '/--REGEXPECT--/,/^--/p' $TEST_FILE  | grep -v "^--" > $TEST_REGEXPECT
        HAVE_REGEXPECT=1
    fi

    #Clean up shell script
    if [ $(grep "\-\-TEARDOWN\-\-" $TEST_FILE | wc -l) -eq 1 ]; then
        TEST_TEARDOWN=${TEST_BASENAME}.teardown.sh
        sed -n '/--TEARDOWN--/,/^--/p' $TEST_FILE  | grep -v "^--" > $TEST_TEARDOWN
        HAVE_TEARDOWN=1
    fi

    TEST_DIFF=${TEST_BASENAME}.diff
    TEST_OUTPUT=${TEST_BASENAME}.out
    TEST_OUTPUT_COMPLETE=${TEST_BASENAME}.out.complete

    if [ $HAVE_COMMAND -eq 0 -a $HAVE_SHELL -eq 0 -a $HAVE_SCRIPT -eq 0 ]; then
        echo "ERROR: $TEST_FILE is missing a test"
        exit 1
    fi

    if [ $HAVE_COMMAND -eq 1 ]; then
        if [ $HAVE_SHELL -eq 1 -o $HAVE_SCRIPT -eq 1 ]; then
            MULTIPLE_TESTS=1
        fi
    fi
    if [ $HAVE_SHELL -eq 1 ]; then
        if [ $HAVE_COMMAND -eq 1 -o $HAVE_SCRIPT -eq 1 ]; then
            MULTIPLE_TESTS=1
        fi
    fi
    if [ $HAVE_SCRIPT -eq 1 ]; then
        if [ $HAVE_COMMAND -eq 1 -o $HAVE_SHELL -eq 1 ]; then
            MULTIPLE_TESTS=1
        fi
    fi
    if [ $MULTIPLE_TESTS -eq 1 ]; then
        echo "Only one test type of --COMMAND--, --SCRIPT-- or --SHELL-- allowed"
        exit 1
    fi

    if [ $HAVE_EXPECT -eq 1 -a $HAVE_REGEXPECT -eq 1 ]; then
        echo "ERROR: Multiple expect sections detected, only one of EXPECT or REGEXPECT should be specified"
        exit 1
    fi

    log $FUNCNAME "Leaving"
}

function init_test
{
    log $FUNCNAME "Entering"

    # ------------------------------------------------ 
    # Initialize the test
    # ------------------------------------------------ 

    if [ $HAVE_SHELL_INIT -eq 1 ]; then
        log $FUNCNAME "Initializing test using bash"
        log $FUNCNAME "$(cat $TEST_SHELL_INIT)"
        TEST_BASENAME=$TEST_BASENAME bash -x $TEST_SHELL_INIT 2> $TEST_SHELL_INIT.stderr > $TEST_SHELL_INIT.stdout
        if [ $? -ne 0 ]; then
            log $FUNCNAME "Error initializing test using $TEST_SHELL_INIT"
            echo "Errors with the shell initialization script $TEST_SHELL_INIT:"
            echo "STDOUT:"
            cat $TEST_SHELL_INIT.stdout
            echo "================================================================================"
            echo
            echo
            echo "STDERROR:"
            cat $TEST_SHELL_INIT.stderr
            echo "================================================================================"
            echo
            echo
            exit 2
        fi
    fi

    log $FUNCNAME "Leaving"
}

function execute_test
{
    log $FUNCNAME "Entering"

    # ------------------------------------------------
    # Execute test shell script
    # ------------------------------------------------
    if [ $HAVE_SHELL -eq 1 ]; then
        log $FUNCNAME "Executing test using bash"
        log $FUNCNAME "$(cat $TEST_SHELL_SCRIPT)"
        if [ $TEST_VERBOSE -eq 1 ]; then
            TEST_BASENAME=$TEST_BASENAME bash -x $TEST_SHELL_SCRIPT > $TEST_OUTPUT_COMPLETE 2>/dev/null
            if [ $? -ne 0 ]; then
                TEST_SHELL_ERROR=1
            fi
        else
            TEST_BASENAME=$TEST_BASENAME bash $TEST_SHELL_SCRIPT > $TEST_OUTPUT_COMPLETE 2>/dev/null
            if [ $? -ne 0 ]; then
                TEST_SHELL_ERROR=1
            fi
        fi
        grep -v "^[MEWT]:" $TEST_OUTPUT_COMPLETE > $TEST_OUTPUT 2>/dev/null
    fi

    if [ $TEST_SHELL_ERROR -ne 0 ]; then
        log $FUNCNAME "Error executing $TEST_SHELL_SCRIPT" "force"
        log $FUNCNAME "$(cat $TEST_OUTPUT_COMPLETE)" force
        exit 3
    fi

    log $FUNCNAME "Exiting"
}

function diff_test
{
    log $FUNCNAME "Entering"
    log $FUNCNAME "Using $TESTDIFF_SCRIPT for regular expression diffs"

    # Check to see if the output is what we expect
    log $FUNCNAME "Diffing test output"
    rm -f $TEST_DIFF
    if [ $HAVE_EXPECT -eq 1 ]; then
        diff -wbB -u $TEST_EXPECT $TEST_OUTPUT > $TEST_DIFF
    else
        $TESTDIFF_SCRIPT -i $TEST_OUTPUT -r $TEST_REGEXPECT >  $TEST_DIFF
    fi 

    if [ $? -ne 0 ]; then
        TEST_FAILED=1
        log $FUNCNAME "----------------------------------------------------------------"
        log $FUNCNAME "Diff for $TEST_FILE failed"
        log $FUNCNAME "Expected"
        log $FUNCNAME "----------------------------------------------------------------"
        if  [ $HAVE_EXPECT -eq 1 ]; then
            log $FUNCNAME "$(cat $TEST_EXPECT)"
        else
            log $FUNCNAME "$(cat $TEST_REGEXPECT)"
        fi
        log $FUNCNAME "----------------------------------------------------------------"

        log $FUNCNAME "================================================================"

        log $FUNCNAME "Got"
        log $FUNCNAME "----------------------------------------------------------------"
        log $FUNCNAME "$(cat $TEST_OUTPUT)"
        log $FUNCNAME "----------------------------------------------------------------"
    else
        # Clean up files for succesful test runs
        log $FUNCNAME "File clean up"
        rm -f $TEST_EXPECT $TEST_OUTPUT_COMPLETE $TEST_SHELL_SCRIPT
        #rm -f $TEST_BASENAME.diff $TEST_REGEXPECT $TEST_OUTPUT
        if [ ! -z $TEST_SHELL_INIT ]; then
            rm -f  $TEST_SHELL_INIT $TEST_SHELL_INIT.stderr $TEST_SHELL_INIT.stdout
        fi
    fi

    log $FUNCNAME "Exiting"
}

function teardown_test
{
    log $FUNCNAME "Entering"

    if [ $HAVE_TEARDOWN -eq 1 ]; then
        log $FUNCNAME "$(cat $TEST_TEARDOWN)"
        if [ $TEST_VERBOSE -eq 1 ]; then
            TEST_BASENAME=$TEST_BASENAME bash -x $TEST_TEARDOWN
        else
            TEST_BASENAME=$TEST_BASENAME bash $TEST_TEARDOWN > /dev/null 2>&1
        fi
        rm $TEST_TEARDOWN
    fi

    log $FUNCNAME "Exiting"
}

function run_test
{

    log $FUNCNAME "Entering"

    setup_test_vars
    init_test
    execute_test
    diff_test
    teardown_test

    log $FUNCNAME "Leaving"
}

function test_header
{
    echo "Orion Context Broker functional unit test harness"
    echo "Machine : $(hostname)"
    echo -n "OS      : "
    if [ -f /etc/lsb-release ]; then
        source /etc/lsb-release 
        echo $DISTRIB_DESCRIPTION
    fi
    echo "Date    : $(date)"
}

function usage
{
    echo "Usage: $0 [test_file|test_directory]"
    exit 1
}

if [ -z $1 ]
then
    usage
else
    test_header
fi

# Locate the testDiff script
BASE_PATH=$(pwd)
SCRIPT_PATH=$(dirname $(readlink -f $0))
unset TESTDIFF_SCRIPT

if [ -x "${SCRIPT_PATH}/testDiff.py" ]; then
    TESTDIFF_SCRIPT=${SCRIPT_PATH}/testDiff.py
else 
    if [ -x "${BASE_PATH}/script/testDiff.py" ]; then
        TESTDIFF_SCRIPT=${BASE_PATH}/script/testDiff.py
    fi
fi

# locate accumulator-server.py
ACC_SERVER=$(which accumulator-server.py 2>/dev/null)
if [ $? -ne 0 ]; then
    PATH=$PATH:$SCRIPT_PATH
fi

if [ -z ${TESTDIFF_SCRIPT} ]; then
    cat <<EOF
Unable to locate testDiff.py make sure that you execute $(baseame $0) using
the full path and that testDiff.py can be found at the same location.
EOF
    exit 1
fi

# Set TEST_VERBOSE=1 to show the diffs for failed tests
if [ -z $TEST_VERBOSE ]; then
    TEST_VERBOSE=0
fi

# If we have a directory we'll run a suite of tests
if [ -d $1 ];
then
    TEST_DIR=$1
fi

# Init success and failure counters
TEST_COUNT_SUCCESS=0
TEST_COUNT_FAIL=0

# Execute an individual test
if [ -f $1 ];
then
    TEST_DIR=$(dirname $1)
    TEST_FILE=$(basename $1)
fi

if [ -z "${TEST_DIR}" ]
then
    echo "ERROR: $1 does not exist"
    exit 1
fi

pushd $TEST_DIR > /dev/null
if [ -z ${TEST_FILE} ];
then
    for test in  `find . -name \*.test | sort`
    do
        TEST_FILE=$test
        run_test
        if [ $TEST_FAILED -eq 1 ]; then
            echo "FAILED : $TEST_NAME ($(basename $TEST_FILE))"
            TEST_COUNT_FAIL=$(expr $TEST_COUNT_FAIL + 1)
            if [ -z "$TEST_FAIL_LIST" ]; then
                TEST_FAIL_LIST=$(echo $TEST_NAME - $(basename $TEST_FILE))
            else
                TEST_FAIL_LIST=$(echo $TEST_FAIL_LIST:$TEST_NAME - $(basename $TEST_FILE))
            fi

        else
            echo "SUCCESS: $TEST_NAME"
            TEST_COUNT_SUCCESS=$(expr $TEST_COUNT_SUCCESS + 1)
        fi

    done
    # Print summary
    echo "===================================================================="
    echo "Test Summary"
    echo "$TEST_COUNT_SUCCESS test(s) passed"
    echo "$TEST_COUNT_FAIL test(s) failed:"
    IFS=":"
    for test in $TEST_FAIL_LIST
    do
        echo $test
    done
    echo "===================================================================="
else
    TEST_VERBOSE=1
    run_test
    if [ $TEST_FAILED -eq 1 ]; then
        echo "FAILED : $TEST_NAME ($(basename $TEST_FILE))"
        TEST_COUNT_FAIL=$(expr $TEST_COUNT_FAIL + 1)
        if [ -z "$TEST_FAIL_LIST" ]; then
            TEST_FAIL_LIST=$(echo $TEST_NAME - $(basename $TEST_FILE))
        else
            TEST_FAIL_LIST=$(echo $TEST_FAIL_LIST:$TEST_NAME - $(basename $TEST_FILE))
        fi

    else
        echo "SUCCESS: $TEST_NAME"
        TEST_COUNT_SUCCESS=$(expr $TEST_COUNT_SUCCESS + 1)
    fi
fi
popd > /dev/null
if [ $TEST_FAILED -ne 0 ]; then
    exit 255
else
    exit 0
fi
