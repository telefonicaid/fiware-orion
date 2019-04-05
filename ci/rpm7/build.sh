#!/bin/bash
# Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
# Author: Dmitrii Demin

url_src='https://github.com/telefonicaid/fiware-orion'
url_dst='https://nexus.lab.fiware.org/repository/el'
releasever=7
basearch='x86_64'

function _usage()
{
  echo -n "Usage: build [options]
  Options:
    -h   --help          show help
    -b   --branch        specify branch/tag to build, if not specified - the source from /opt/fiware-orion will be used
    -S   --source        specify repository to clone, if not specified - default repo (https://github.com/telefonicaid/fiware-orion) will be used
    -p   --path          specify path to use as home, if not specified - /opt/fiware-orion will be used
    -s   --stage         specify stage (unit/functional/compliance) to use
    -m   --make          cmake/make (works with unit/functional stage only)
    -i   --install       make install (works with unit/functional stage only)
    -d   --db            start mongodb
    -t   --test          run unit/functional/compliance test (depends on the defined stage)
    -r   --rpm           specify rpm (nightly, release, testing) to build
    -u   --upload        upload rpm, REPO_USER and REPO_PASSWORD ENV variables should be provided
    -j   --jenkins       execute fix for jenkins during functional testing (disable ipv6 test)
    -J                   execute fix for jenkins
    -q   --speed         execute fix for functional tests during testing (improve speed)
    -Q                   execute fix for functional tests
    -a   --attempts      attempts to execute functional test
    -e   --execute       run (rerun) test stand with 2 orions
    -H   --show          show the list of necessary commands that should be executed before starting functional tests manually

  Examples:
    build -s unit -midt -b master    clone from master, make (for unit testing), make install, run mongo, execute unit tests
    build -s functional -midtq       get source from mounted folder, make (for functional testing), make install, run mongo, execute speed FIX, functional test
"
}

function _fix_tests()
{
    # This function adjust functional tests to reduce time, that is spent on the tests.
    # TODO: this function created to reduce the time that is spent on functional tests because of travis time limits. Should be fixed.
    echo "Builder: fix tests"

    # This part adjust TTL mongo parameter and 3000_allow_creation_transient_entities sleep parameter.
    test='3000_allow_creation_transient_entities'
    echo "Builder: ${test}"

    list=('create_transient_entity.test' \
          'replace_to_transient_entity.test' \
          'update_to_regular_entity.test' \
          'update_to_transient_entity.test')

    for i in ${list[@]}; do
        cp ${path}/test/functionalTest/cases/${test}/${i} /tmp/builder/bu/${test}-${i}
        sed 's/sleep [0-9]*/sleep 3/g' ${path}/test/functionalTest/cases/${test}/${i} > /tmp/test.tmp && mv -f /tmp/test.tmp ${path}/test/functionalTest/cases/${test}/${i}
    done

    mongo --eval "db.adminCommand({setParameter:1, ttlMonitorSleepSecs: 3});"
}

function _unfix_tests()
{
    echo "Builder: revert tests"
    test='3000_allow_creation_transient_entities'
    echo "Builder: ${test}"

    list=('create_transient_entity.test' \
          'replace_to_transient_entity.test' \
          'update_to_regular_entity.test' \
          'update_to_transient_entity.test')

    for i in ${list[@]}; do
        mv -f /tmp/builder/bu/${test}-${i} ${path}/test/functionalTest/cases/${test}/${i}
    done

    mongo --eval "db.adminCommand({setParameter:1, ttlMonitorSleepSecs: 60});"
}

function _fix_jenkins()
{
    # FIXME: ipv4_ipv6_both.test is disabled as IPV6 is disabled in CI environment. This should be fixed.
    echo "Builder: fix jenkins"
    cp ${path}/test/functionalTest/testHarness.sh /tmp/builder/bu/testHarness.sh
    sed $'/DISABLED=(/s/$/\\\n          \'test\/functionalTest\/cases\/0000_ipv6_support\/ipv4_ipv6_both.test\' \\\/1' ${path}/test/functionalTest/testHarness.sh > /tmp/ft.tmp && mv -f /tmp/ft.tmp ${path}/test/functionalTest/testHarness.sh
    chmod +x ${path}/test/functionalTest/testHarness.sh
}

function _unfix_jenkins()
{
    echo "Builder: revert jenkins"
    mv -f /tmp/builder/bu/testHarness.sh ${path}/test/functionalTest/testHarness.sh
}

function _fix()
{
    echo "Builder: fix makefile"
    mv -f ${path}/makefile /tmp/builder/bu/
    if [ ! -f "${path}/ci/rpm7/makefile" ]; then tp="/opt/archive/makefile"; else tp="${path}/ci/rpm7/makefile"; fi
    cp ${tp} ${path}/
}

function _unfix()
{
    echo "Builder: revert makefile"
    mv -f /tmp/builder/bu/makefile ${path}/makefile
}

function _show()
{
    echo "Builder: run this commands"
    echo ". scripts/testEnv.sh"
}

function _execute()
{
    killall contextBroker
    killall mongod

    sleep 3

    mongod --dbpath /data/db1 --port 20001 --quiet --nojournal&
    mongod --dbpath /data/db2 --port 20002 --quiet --nojournal&

    sleep 3

    contextBroker -port 30001 -dbhost localhost:20001 -pidpath cb1.pid &
    contextBroker -port 30002 -dbhost localhost:20002 -pidpath cb2.pid &
}

[ $# = 0 ] && _usage

reset=true

for arg in "$@"
do
    if [ -n "$reset" ]; then
      unset reset
      set --
    fi
    case "$arg" in
       --help) set -- "$@" -h ;;
       --branch) set -- "$@" -b ;;
       --source) set -- "$@" -S ;;
       --path) set -- "$@" -p ;;
       --stage) set -- "$@" -s ;;
       --make) set -- "$@" -m ;;
       --install) set -- "$@" -i ;;
       --db) set -- "$@" -d ;;
       --test) set -- "$@" -t ;;
       --rpm) set -- "$@" -r ;;
       --upload) set - "$@" -u ;;
       --jenkins) set -- "$@" -j ;;
       --speed) set -- "$@" -q ;;
       --execute) set -- "$@" -e ;;
       --show) set -- "$@" -H ;;
       --attempts) set -- "$@" -a ;;
       *) set -- "$@" "$arg" ;;
    esac
done

while getopts ":hb:S:p:s:midtr:ujJqQeHa:" opt; do
    case ${opt} in
        h)  _usage; exit 0 ;;
        b)  branch=$OPTARG ;;
        S)  source=$OPTARG ;;
        p)  path=$OPTARG ;;
        s)  stage=$OPTARG ;;
        m)  make=true;;
        i)  install=true;;
        d)  database=true ;;
        t)  test=true ;;
        r)  rpm=$OPTARG ;;
        u)  upload=true ;;
        j)  fix_j=true ;;
        J)  fix_J=true ;;
        q)  fix_q=true ;;
        Q)  fix_Q=true ;;
        e)  execute=true ;;
        H)  show=true ;;
        a)  attempts=$OPTARG ;;
        *) _usage ;;
        :)
        echo "option -$OPTARG requires an argument"
        _usage; exit 1
        ;;
    esac
done
shift $((OPTIND-1))

echo "===================================== CHECKS ==========================================="

echo "Builder: if upload is on, check if credentials exists"
if [ -n "${upload}" ]; then
    if [ -z "${REPO_USER}" ] || [ -z "${REPO_PASSWORD}" ]; then
        echo "Builder: failed, REPO_USER or REPO_PASSWORD env variables not set"; exit 1;
    fi
fi

echo "Builder: check if stage/rpm defined"
if [[ -z "${stage}" && -z "${rpm}" ]]; then echo "Builder: failed, stage/rpm not defined"; fi

echo "Builder: check if target folder can be created"
if [ -z "${path}" ]; then path='/opt/fiware-orion'; fi
mkdir -p ${path} > /dev/null 2>&1
if [ ! -d "${path}" ]; then echo "Builder: failed, not enough permissions"; exit 1; fi
cd ${path}

echo "Builder: if branch is empty, check if source exists"
if [ -z "${branch}" ] && [ ! -f ${path}/LICENSE ]; then echo "Builder: failed, source code not found"; exit 1; fi

echo "===================================== PREPARE =========================================="

echo "Builder: create temp folders"
rm -Rf /tmp/builder || true && mkdir -p /tmp/builder/{db1,db2,db,bu}

if [ -n "${database}" ]; then
    echo "Builder: starting Mongo"
    mongod --dbpath /tmp/builder/db  --nojournal --quiet > /dev/null 2>&1 &
    sleep 3
    export MONGO_HOST=localhost
fi

if [ -n "${branch}" ]; then
    echo "===================================== CLONE ============================================"

    echo "Builder: cloning branch: "${branch}
    if [ -n "${source}" ]; then url_src=${source}; fi
    echo "Builder: cloning from: "${url_src}
    git clone -b ${branch} ${url_src} ${path}
    chown -R root:root ${path}

fi

if [ -n "${make}" ]; then
    echo "===================================== MAKE ============================================="

    _fix
    echo "Builder: build ${stage}"
    make build_${stage}
    if [ $? -ne 0 ]; then echo "Builder: make failed"; exit 1; fi
    _unfix

fi

if [ -n "${install}" ]; then
    echo "===================================== INSTALL =========================================="

    _fix
    echo "Builder: install ${stage}"
    make install_${stage}
    if [ $? -ne 0 ]; then echo "Builder: installation failed"; exit 1; fi
    _unfix

fi

if [ -n "${execute}" ]; then
    echo "===================================== EXECUTE =========================================="

    _execute
    exit 0

fi

if [ -n "${test}" ] && [ "${stage}" = "compliance" ]; then
    echo "===================================== COMPLIANCE TESTS ================================="

    status=true
    _fix

    make files_compliance
    if [ $? -ne 0 ]; then status=false; fi

    make payload_check
    if [ $? -ne 0 ]; then status=false; fi

    make style
    if [ $? -ne 0 ]; then status=false; fi
    rm -Rf LINT*

    _unfix

    if ! ${status}; then echo "Builder: compliance test failed"; exit 1; fi

fi

if [ -n "${test}" ] && [ "${stage}" = "unit" ]; then
    echo "===================================== UNIT TESTS ======================================="

    _fix
    make unit
    if [ $? -ne 0 ]; then status=false; else status=true; fi
    _unfix

    if ! ${status}; then echo "Builder: unit test failed"; exit 1; fi
fi

if [ -n "${test}" ] && [ "${stage}" = "functional" ]; then
    echo "===================================== FUNCTIONAL TESTS ================================="

    if [ -n "${fix_j}" ]; then _fix_jenkins; fi
    if [ -n "${fix_q}" ]; then _fix_tests; fi

    _fix

    if [ -n "${attempts}" ]; then export CB_MAX_TRIES=${attempts}; fi

    CB_DIFF_TOOL=diff make functional INSTALL_DIR=~
    if [ $? -ne 0 ]; then status=false; else status=true; fi

    _unfix

    if [ -n "${fix_j}" ]; then _unfix_jenkins; fi
    if [ -n "${fix_q}" ]; then _unfix_tests; fi

    if ! ${status}; then echo "Builder: functional test failed"; exit 1; fi

fi

if [ -n "${rpm}" ]; then
    echo "===================================== BUILDING RPM ====================================="
    echo "Builder: ${rpm} rpm"

    if [ "$rpm" = "nightly" ]; then

        export BROKER_RELEASE=$(date "+%Y%m%d")
        git reset --hard && git clean -qfdx

        cd ${path}/src/app/contextBroker
        version=$(cat version.h | grep ORION_VERSION | awk '{ print $3}' | sed 's/"//g' | sed 's/-next//g')
        sed -i "s/ORION_VERSION .*/ORION_VERSION \"$version\"/g" version.h
        cd ${path}

        make rpm

        pack='nightly'

    fi

    if [ "${rpm}" = "release" ]; then

        export BROKER_RELEASE=1
        git reset --hard && git clean -qfdx

        make rpm

        pack='release'

    fi

    if [ "${rpm}" = "testing" ]; then

        export BROKER_RELEASE=$(date "+%Y%m%d%H%M")
        git reset --hard && git clean -qfdx

        cd ${path}/src/app/contextBroker
        version=$(cat version.h | grep ORION_VERSION | awk '{ print $3}' | sed 's/"//g' | sed 's/-next//g')
        sed -i "s/ORION_VERSION .*/ORION_VERSION \"$version\"/g" version.h
        cd ${path}

        make rpm

        pack='testing'

    fi
fi

if [ -n "${upload}" ]; then
    echo "===================================== UPLOADING RPMS ==================================="

    cd ${path}/rpm/RPMS/x86_64
    for file in $(ls); do
      echo "Builder: uploading ${file}"
      curl -v -u ${REPO_USER}:${REPO_PASSWORD} --upload-file ${file} ${url_dst}/${releasever}/${basearch}/${pack}/${file};
      if !(curl --output /dev/null --silent --head --fail ${url_dst}/${releasever}/${basearch}/${pack}/${file}); then echo "UPLOAD FAILED!"; exit 1; fi
   done

fi


if [ -n "${show}" ]; then _show; fi
if [ -n "${fix_J}" ]; then _fix_jenkins; fi
if [ -n "${fix_Q}" ]; then _fix_tests; fi
