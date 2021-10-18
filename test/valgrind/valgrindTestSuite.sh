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
#

# -----------------------------------------------------------------------------
#
# FUNC_TEST_RUNNING_UNDER_VALGRIND - 
#
# This env var is set to "true" so that the harness tests will know whether they are
# running under the valgrund test suite.
# Running under valgrind, everything goes a lotr slower and sleps have to be inserted.
# We don't want these sleeps for regular harness tests, so this variable is used to
# distinguish between the two cases.
#
export FUNC_TEST_RUNNING_UNDER_VALGRIND="true"



# -----------------------------------------------------------------------------
#
# CB_TEST_PORT is used by cbTest.sh as default port
#
export CB_TEST_PORT=9999



# -----------------------------------------------------------------------------
#
# valgrindTestSuite.sh is always executed from the git root directory
#
# To find the executables under 'scripts', we add the directory to the PATH variable
#
export PATH=$PATH:$PWD/scripts



# -----------------------------------------------------------------------------
#
# global variables
#
CASES_DIR=cases
typeset -i lost
typeset -i valgrindErrors



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
  echo "$empty [-filter <test filter>]"
  echo "$empty [-dryrun (don't execute any tests)]"
  echo "$empty [-leakTest (test a memory leak)]"
  echo "$empty [-dryLeaks (simulate leaks and functest errors)]"
  echo "$empty [-fromIx <index of test where to start>]"
  echo "$empty [-toIx <index of test where to end>]"
  echo "$empty [-ixList <list of testNo indexes>]"
  echo "$empty [test case file]"

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
# If any mongo database ftest-ftest exists, strange memory leaks appear ...
# So, before starting, it's important to remove all ftest DBs
#
function dbReset()
{
  test=$1

  echo "Resetting database for $test" >> /tmp/valgrindDbReset.log
  dbResetAllFtest.sh >> /tmp/valgrindDbReset.log 2>&1
}

date
date > /tmp/valgrindDbReset.log
dbReset ALL
totalStartTime=$(date +%s.%2N)



# -----------------------------------------------------------------------------
#
# Init file already sourced?
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



# -----------------------------------------------------------------------------
#
# Parsing parameters
#
typeset -i fromIx
typeset -i toIx
verbose=off
TEST_FILTER=${TEST_FILTER:-"*.*test"}
dryrun=off
leakTest=off
dryLeaks=off
fromIx=0
toIx=0
ixList=""
file=""
vMsg "parsing options"
while [ "$#" != 0 ]
do
  if   [ "$1" == "-u" ];        then usage 0;
  elif [ "$1" == "-v" ];        then verbose=on;
  elif [ "$1" == "-leakTest" ]; then leakTest=on;
  elif [ "$1" == "-filter" ];   then TEST_FILTER=$2; shift;
  elif [ "$1" == "-dryrun" ];   then dryrun=on;
  elif [ "$1" == "-dryLeaks" ]; then dryLeaks=on;
  elif [ "$1" == "-fromIx" ];   then  shift; fromIx=$1;
  elif [ "$1" == "-toIx" ];     then  shift; toIx=$1;
  elif [ "$1" == "-ixList" ];   then  shift; ixList=$1;
  else
    if [ "$file" == "" ]
    then
      file=$1
      TEST_FILTER=$file
    else
      echo $0: bad parameter/option: "'"${1}"'";
      usage 1
    fi
  fi
  shift
done

# -----------------------------------------------------------------------------
#
# Check if fromIx is set through an env var and use if nothing
# else is set through commandline parameter
#
if [ "$FT_FROM_IX" != "" ] && [ $fromIx == 0 ]
then
  fromIx=$FT_FROM_IX
  unset FT_FROM_IX
fi

# -----------------------------------------------------------------------------
#
# Check if toIx is set through an env var and use if nothing
# else is set through commandline parameter
#
if [ "$FT_TO_IX" != "" ] && [ $toIx == 0 ]
then
  toIx=$FT_TO_IX
  unset FT_TO_IX
fi

echo "Run tests $fromIx to $toIx"

if [ "$dryrun" == "on" ]
then
  okString="DryRun"
else
  okString="OK"
fi

vMsg
vMsg "----- Command line options -----"
vMsg "filter: $TEST_FILTER"
vMsg "dryrun: $dryrun"
vMsg "leakTest: $leakTest"
vMsg




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
cd test/valgrind
vMsg In directory $(pwd)



# -----------------------------------------------------------------------------
#
# add - 
#
# We need this function because the regular '+' can not process correctly
# numbers with ',', as the ones generated in the valgrind report (e.g. "1,223")
#
function add()
{
  typeset -i num
  typeset -i sum
  sum=0

  for i in $*
  do
    num=$(echo $i | sed 's/,//g')
    sum=$sum+$num
  done

  echo $sum
}



# -----------------------------------------------------------------------------
#
# Extract leak info from valgrind output file
#
# It uses as argument the .out file to process
#
function leakInfo()
{
    filename=$1
    # Get info from valgrind
    startLine1=$(grep --text -n "LEAK SUMMARY" $filename | awk -F: '{ print $1 }' | head -1)
    startLine2=$(grep --text -n "LEAK SUMMARY" $filename | awk -F: '{ print $1 }' | tail -1)

    if [ "$startLine1" == "" ] || [ "$startLine2" == "" ]
    then
        return
    fi

    typeset -i headEndLine1
    headEndLine1=$startLine1+7

    head -$headEndLine1 $filename | tail -8 > valgrind.leakSummary

    definitelyLost1=$(grep 'definitely lost' valgrind.leakSummary | awk '{ print $4 }')
    indirectlyLost1=$(grep 'indirectly lost' valgrind.leakSummary | awk '{ print $4 }')
    possiblyLost1=$(grep 'possibly lost' valgrind.leakSummary | awk '{ print $4 }')
    stillReachable1=$(grep 'still reachable' valgrind.leakSummary | awk '{ print $4 }')

    headEndLine2=$startLine2+7

    head -$headEndLine2 $1 | tail -8 > valgrind.leakSummary2

    definitelyLost2=$(grep 'definitely lost' valgrind.leakSummary2 | awk '{ print $4 }')
    indirectlyLost2=$(grep 'indirectly lost' valgrind.leakSummary2 | awk '{ print $4 }')
    possiblyLost2=$(grep 'possibly lost' valgrind.leakSummary2 | awk '{ print $4 }')
    stillReachable2=$(grep 'still reachable' valgrind.leakSummary2 | awk '{ print $4 }')

    \rm valgrind.leakSummary valgrind.leakSummary2

    lost=$(add $definitelyLost1 $indirectlyLost1 $definitelyLost2 $indirectlyLost2)
}



# -----------------------------------------------------------------------------
#
# Extract error info from valgrind output file
#
# The first and only argument to 'valgrindErrorInfo' is the valgrind.out file to process
#
function valgrindErrorInfo()
{
  filename=$1

  #
  # Get info from valgrind file
  #
  typeset -i vErrors
  vErrors=0
  for num in $(grep "errors in context" $filename | awk '{ print $2 }')
  do
    typeset -i xNum
    xNum=$num
    if [ $xNum != 0 ]
    then
      vErrors=$vErrors+$xNum
    fi
  done
  valgrindErrors=$vErrors
  vMsg valgrindErrors: $valgrindErrors
}



# -----------------------------------------------------------------------------
#
# Printing related functions
#
function printTestLinePrefix()
{
  if [ "$dryrun" == "off" ]
  then
    name="Test "
  else
    name=""
  fi

  if [ $testNo -lt 10 ]
  then
    testNoString=${name}"00"${testNo}"/"${noOfTests}": "
  elif [ $testNo -lt 100 ]
  then
    testNoString=${name}"0"${testNo}"/"${noOfTests}": "
  else
    testNoString=${name}${testNo}"/"${noOfTests}": "
  fi
}

function printNotImplementedString()
{
  echo "Not apt for valgrind - skipped"
  echo                                    >> /tmp/valgrindTestSuiteLog
  echo "$1 not implemented - skipped"     >> /tmp/valgrindTestSuiteLog
  echo                                    >> /tmp/valgrindTestSuiteLog
}

function printImplementedString()
{
  echo '# ---------------------------------------------' >> /tmp/valgrindTestSuiteLog
  echo '# '                                              >> /tmp/valgrindTestSuiteLog
  echo "# $1"                                            >> /tmp/valgrindTestSuiteLog
  echo '# '                                              >> /tmp/valgrindTestSuiteLog
}



# -----------------------------------------------------------------------------
#
# setNumberOfTests
#
function setNumberOfTests()
{
  noOfTests=0

  if [ "$runHarness" -eq "1" ]
  then
    for file in $(find ../functionalTest/$CASES_DIR -name "$TEST_FILTER")
    do
      noOfTests=$noOfTests+1
    done
  fi

  if [ "$leakTest" == "on" ]
  then
    noOfTests=$noOfTests+1
  fi

  vMsg "Number Of Tests: " $noOfTests
}



# -----------------------------------------------------------------------------
#
# Leak in test?
#
function leakFound()
{
  _valgrindFile=$1
  _file=$2
  _lost=$3
  _dir=$4
  _testNo=$5

  if [ "$_lost" != "0" ]
  then
    echo "FAILED (lost: $_lost). Check $_valgrindFile for clues"
  fi

  testFailures=$testFailures+1

  if [ "$_dir" != "" ]
  then
    failedTests[$testFailures]=$_testNo": "$_dir/$_file".test (lost $_lost bytes, see $_valgrindFile)"
  else
    failedTests[$testFailures]=$_testNo": "$_file".test (lost $_lost bytes, see $_valgrindFile)"
  fi

  failedTests[$testFailures+1]=$okString
}



# -----------------------------------------------------------------------------
#
# valgrindErrorFound
#
function valgrindErrorFound()
{
  _valgrindFile=$1
  _file=$2
  _valgrindErrors=$3
  _testNo=$4

  echo "FAILED (valgrind errors: $_valgrindErrors). Check $_valgrindFile for clues"

  valgrindErrorV[$valgrindErrorNo]="$_testNo: $_file shows $_valgrindErrors valgrind errors"
  valgrindErrorNo=$valgrindErrorNo+1
}



# -----------------------------------------------------------------------------
#
# Main program
#

# Port tests
nc -zv localhost ${CB_TEST_PORT} &>/dev/null </dev/null
if [ "$?" == "0" ]
then
   # Successful nc means that port CB_TEST_PORT is used, thus exit
   echo "Port $CB_TEST_PORT is in use. Aborting"
   exit 1
fi

runHarness=1
vMsg "Running valgrind test suite"
if [ "$leakTest" == "on" ]
then
  runHarness=0
  vMsg "Selecting only the leak test"
fi

date > /tmp/valgrindTestSuiteLog

typeset -i noOfTests
noOfTests=0
setNumberOfTests

declare -A failedTests
declare -A harnessErrorV
declare -A valgrindErrorV
typeset -i harnessErrors
typeset -i testNo
typeset -i testFailures
typeset -i valgrindErrorNo
testNo=0;
testFailures=0
harnessErrors=0
valgrindErrorNo=0


#
# No diff tool for harnessTest when running from valgrind test suite
#
unset CB_DIFF_TOOL
orderedExit=0

if [ "$runHarness" -eq "1" ]
then
  if [ "$CONTEXTBROKER_TESTENV_SOURCED" != "YES" ]
  then
    echo "$0: please source the test script (scripts/testEnv.sh)"
    exit 1
  fi

  cd $SRC_TOP/test/functionalTest/$CASES_DIR
  vMsg TEST_FILTER: $TEST_FILTER
  for file in $(find . -name "$TEST_FILTER" | sort)
  do
    testNo=$testNo+1
    xTestNo=$(printf "%03d" $testNo)

    if [ $fromIx != 0 ] && [ $testNo -lt $fromIx ]
    then
      continue
    fi

    if [ $toIx != 0 ] && [ $testNo -gt $toIx ]
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

    dbReset "$file"

    htest=$(basename $file | awk -F '.' '{print $1}')
    directory=$(dirname $file)
    dir=$(basename $directory)
    file=$htest.test

    vMsg file: $file
    vMsg htest: $htest

    printTestLinePrefix
    init="$testNoString $dir/$htest ..........................................................................................................................................................."
    init=${init:0:150}
    echo -n $init" "

    # In the case of harness test, we check that the test is implemented checking
    # that the word VALGRIND_READY apears in the .test file (usually, in a commented line)
    grep VALGRIND_READY $SRC_TOP/test/functionalTest/$CASES_DIR/$directory/$file > /dev/null 2>&1
    if [ "$?" -ne "0" ]
    then
      printNotImplementedString $htest
      continue
    fi

    cd $SRC_TOP
    printImplementedString $htest
    typeset -i lost
    lost=0
    valgrindErrors=0
    if [ "$dryrun" == "off" ]
    then
      detailForOkString=''

      vMsg "------------------------------------------------"
      vMsg running harnessTest.sh with $file in $(pwd)
      vMsg "------------------------------------------------"

      # Sometimes the Orion pid file is not deleted and this cause all the next tests to fail
      # We ensure it is deleted before launch the test
      # FIXME: this could be improved (using /tmp/orion_9*.pid is a bit hardwired...)
      rm -f /tmp/orion_9*.pid 

      startTime=$(date +%s.%2N)
      VALGRIND=1 test/functionalTest/testHarness.sh --filter $file > /tmp/testHarness 2>&1
      status=$?
      endTime=$(date +%s.%2N)
      diffTime=$(echo $endTime - $startTime | bc)
      vMsg status=$status
      if [ "$status" != "0" ]
      then
        mv /tmp/testHarness         test/functionalTest/$CASES_DIR/$directory/$htest.harness.out
        cp /tmp/contextBroker.log   test/functionalTest/$CASES_DIR/$directory/$htest.contextBroker.log
        detailForOkString=" (no leak but ftest error $status)"
        harnessErrorV[$harnessErrors]="$xTestNo: $file (exit code $status)"
        harnessErrors=$harnessErrors+1
        # No exit here - sometimes harness tests fail under valgrind ...
      fi

      if [ ! -f /tmp/valgrind.out ]
      then
        echo " FAILURE! (no valgrind output for functional test $file)"
        exit 3
      fi

      mv /tmp/valgrind.out test/functionalTest/$CASES_DIR/$directory/$htest.valgrind.out
      
      typeset -i headEndLine1
      typeset -i headEndLine2
      vMsg processing $directory/$htest.valgrind.out in $(pwd)

      lost=0
      leakInfo test/functionalTest/$CASES_DIR/$directory/$htest.valgrind.out

      valgrindErrors=0
      valgrindErrorInfo test/functionalTest/$CASES_DIR/$directory/$htest.valgrind.out
    else
      if [ "$dryLeaks" == "on" ]
      then
        modula3=$(echo $testNo % 30 | bc)
        if [ $modula3 == 0 ]
        then
          leakFound "$htest.valgrind.out" $htest 1024 $dir $xTestNo
        fi

        modula4=$(echo $testNo % 40 | bc)
        if [ $modula4 == 0 ]
        then
          harnessErrorV[$harnessErrors]="$testNo: $file (exit code XXX)"
          harnessErrors=$harnessErrors+1
        fi
      fi
    fi
    cd - > /dev/null

    if [ "$lost" != "0" ]
    then
      leakFound "$htest.valgrind.out" $htest $lost $dir $xTestNo
    fi

    if [ "$valgrindErrors" != "0" ]
    then
      valgrindErrorFound "$htest.valgrind.out" $htest $valgrindErrors $xTestNo
    fi

    if [ "$lost" == "0" ] && [ "$valgrindErrors" == "0" ]
    then
      echo $okString "($diffTime seconds)" $detailForOkString
    fi
  done
  orderedExit=1
fi


#
# If 'orderedExit' is not set to 1 right after the loop, then for some reason (syntax error somewhere in the loop) this
# last line, right after the loop ends, hasn't been executed (this last line sets 'orderedExit' to 1).
# If this is the case, we cannot continue, but must exit here, signaling an error to the caller (exit code 2).
#
if [ "$orderedExit" != 1 ]
then
  echo "Something went wrong"
  exit 2
fi



retval=0
if [ "${failedTests[1]}" != "" ]
then
  echo
  echo
  echo "$testFailures tests leaked memory:"
  typeset -i ix
  ix=0

  while [ $ix -ne $testFailures ]
  do
    ix=$ix+1
    echo "  " ${failedTests[$ix]}
  done

  echo "---------------------------------------"
  echo
  retval=1
fi


if [ $valgrindErrorNo != 0 ]
then
  echo
  echo
  echo "$valgrindErrorNo test cases show valgrind errors:"
  typeset -i ix
  ix=0

  while [ $ix -ne $valgrindErrorNo ]
  do
    echo "  ${valgrindErrorV[$ix]}"
    ix=$ix+1
  done

  echo "---------------------------------------"
  retval=1
  echo
fi


if [ $harnessErrors != 0 ]
then
  echo
  echo
  echo "$harnessErrors functional tests failed (not a leak, just a func-test failure):"
  typeset -i ix
  ix=0

  while [ $ix -ne $harnessErrors ]
  do
    echo "  " ${harnessErrorV[$ix]}
    ix=$ix+1
  done

  echo "---------------------------------------"
  echo
fi

totalEndTime=$(date +%s.%2N)
totalDiffTime=$(echo $totalEndTime - $totalStartTime | bc)
min=$(echo $totalDiffTime / 60 | bc)
echo Total test time: $totalDiffTime" seconds ($min minutes)"

if [ "$dryrun" == "off" ] && [ "$testFailures" == "0" ]
then
  echo "Great, all valgrind tests ran without any memory leakage"
fi

exit $retval
