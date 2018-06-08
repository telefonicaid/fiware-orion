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
    -s   --source        specify repository to clone, if not specified - default repo (https://github.com/telefonicaid/fiware-orion) will be used
    -p   --path          specify path to use as home, if not specified - /opt/fiware-orion will be used
    -M   --make          cmake/make, stage (unit/functional) should be specified
    -I   --install       make install
    -c   --compliance    run file_compliance, payload and style checks
    -u   --unit          run unit tests
    -f   --functional    run functional tests
    -n   --nightly       build rpm nightly
    -r   --release       build rpm release
    -t   --testing       build rpm testing
    -U   --upload        upload rpm, REPO_USER and REPO_PASSWORD ENV variables should be provided
    -F   --fix           execute fix for jenkins (disable ipv6 test)
    -E   --execute       run (rerun) test stand with 2 orions
    -D   --db            start mongodb
    -S   --show          show the list of necessary commands that should be executed before starting functional tests manually

  Examples:
    build -M unit -IDu -b master   clone from master, make (for unit testing), make install, run mongo, execute unit tests
    build -M functional -IDFf      get source from mounted folder, make (for functional testing), make install, run mongo, execute FIX, functional test
"
  exit 0
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
    cp /opt/archive/makefile ${path}/
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
       --source) set -- "$@" -s ;;
       --path) set -- "$@" -p ;;
       --make) set -- "$@" -M ;;
       --install) set -- "$@" -I ;;
       --compliance) set -- "$@" -c ;;
       --unit) set -- "$@" -u ;;
       --functional) set -- "$@" -f ;;
       --nightly) set -- "$@" -n ;;
       --release) set - "$@" -r ;;
       --testing) set - "$@" -t ;;
       --upload) set - "$@" -U ;;
       --fix) set -- "$@" -F ;;
       --execute) set -- "$@" -E ;;
       --db) set -- "$@" -D ;;
       --show) set -- "$@" -S ;;
       *) set -- "$@" "$arg" ;;

    esac
done

while getopts ":hb:s:p:M:IcufnrtUFEDH" opt; do
    case ${opt} in
        h)  _usage ;;
        b)  branch=$OPTARG ;;
        s)  source=$OPTARG ;;
        p)  path=$OPTARG ;;
        M)  make=$OPTARG;;
        I)  install=true;;
        c)  compliance=true ;;
        u)  unit=true ;;
        f)  functional=true ;;
        n)  nightly=true ;;
        r)  release=true ;;
        t)  testing=true ;;
        U)  upload=true ;;
        F)  fix=true ;;
        E)  execute=true ;;
        D)  database=true ;;
        H)  show=true ;;
        *) _usage ;;
        :)
        echo "option -$OPTARG requires an argument"
        usage
        ;;
    esac
done
shift $((OPTIND-1))

echo "================================================================="
echo "                             CHECKS                              "
echo "================================================================="

echo "Builder: if upload is on, check if credentials exists"
if [ -n "${upload}" ]; then
    if [ -z "${REPO_USER}" ] || [ -z "${REPO_PASSWORD}" ]; then
        echo "Builder: failed, REPO_USER or REPO_PASSWORD env variables not set"; exit 1;
    fi
fi

echo "Builder: check if target folder can be created"
if [ -z "${path}" ]; then path='/opt/fiware-orion'; fi
mkdir -p ${path} > /dev/null 2>&1
if [ ! -d "${path}" ]; then echo "Builder: failed, not enough permissions"; exit 1; fi
cd ${path}

echo "Builder: if branch is empty, check if source exists"
if [ -z "${branch}" ] && [ ! -f ${path}/LICENSE ]; then echo "Builder: failed, source code not found"; exit 1; fi


echo "================================================================="
echo "                             PREPARE                             "
echo "================================================================="

echo "Builder: create temp folders"
rm -Rf /tmp/builder || true && mkdir -p /tmp/builder/{db1,db2,db,bu}

if [ -n "${database}" ]; then
    echo "Builder: starting Mongo"
    mongod --dbpath /tmp/builder/db  --nojournal --quiet > /dev/null 2>&1 &
    sleep 3
    export MONGO_HOST=localhost
fi

if [ -n "${branch}" ]; then
    echo "================================================================="
    echo "                         CLONE                                   "
    echo "================================================================="

    echo "Builder: cloning branch: "${branch}
    if [ -n "${source}" ]; then url_src=${source}; fi
    echo "Builder: cloning from: "${url_src}
    git clone -b ${branch} ${url_src} ${path}
    chown -R root:root ${path}

fi

if [ -n "${make}" ]; then
    echo "================================================================="
    echo "                        BUILD                                    "
    echo "================================================================="

    _fix
    echo "Builder: build ${make}"
    make build_${make}
    if [ $? -ne 0 ]; then echo "Builder: make failed"; exit 1; fi
    _unfix

fi

if [ -n "${install}" ]; then
    echo "================================================================="
    echo "                        INSTALL                                  "
    echo "================================================================="

    _fix
    make install
    if [ $? -ne 0 ]; then echo "Builder: installation failed"; exit 1; fi
    _unfix

fi

if [ -n "${execute}" ]; then
    echo "================================================================="
    echo "                        EXECUTE                                  "
    echo "================================================================="

    _execute
    exit 0

fi

if [ -n "${compliance}" ]; then
    echo "================================================================="
    echo "                       COMPLIANCE TESTS                          "
    echo "================================================================="

    status=true

    make files_compliance
    if [ $? -ne 0 ]; then status=false; fi

    make payload_check
    if [ $? -ne 0 ]; then status=false; fi

    make style
    if [ $? -ne 0 ]; then status=false; fi
    rm -Rf LINT*

    if ! ${status}; then echo "Builder: compliance test failed"; fi

fi

if [ -n "${unit}" ]; then
    echo "================================================================="
    echo "                       UNIT TESTS                                "
    echo "================================================================="

    _fix
    make unit
    if [ $? -ne 0 ]; then echo "Builder: unit test failed"; exit 1; fi
    _unfix

fi

if [ -n "${functional}" ]; then
    echo "================================================================="
    echo "                    FUNCTIONAL TESTS                             "
    echo "================================================================="

    if [ -n "${fix}" ]; then _fix_jenkins; fi

    _fix

    make functional INSTALL_DIR=~
    if [ $? -ne 0 ]; then status=false; else status=true; fi

    _unfix

    if [ -n "${fix}" ]; then _unfix_jenkins; fi

    if ! ${status}; then echo "Builder: functional test failed"; exit 1; fi

fi

if [ -n "${nightly}" ]; then
    echo "================================================================="
    echo "                   BUILDING NIGHTLY RPM                          "
    echo "================================================================="

    export BROKER_RELEASE=$(date "+%Y%m%d")
    git reset --hard

    cd ${path}/src/app/contextBroker
    version=$(cat version.h | grep ORION_VERSION | awk '{ print $3}' | sed 's/"//g' | sed 's/-next//g')
    sed -i "s/ORION_VERSION .*/ORION_VERSION \"$version\"/g" version.h
    cd ${path}

    make rpm

    pack='nightly'

fi

if [ -n "${release}" ]; then
    echo "================================================================="
    echo "                     BUILDING RELEASE RPM                        "
    echo "================================================================="

    export BROKER_RELEASE=1
    git reset --hard

    make rpm

    pack='release'

fi

if [ -n "${testing}" ]; then
    echo "================================================================="
    echo "                     BUILDING TESTING RPM                        "
    echo "================================================================="

    export BROKER_RELEASE=$(date "+%Y%m%d%H%M")
    git reset --hard

    cd ${path}/src/app/contextBroker
    version=$(cat version.h | grep ORION_VERSION | awk '{ print $3}' | sed 's/"//g' | sed 's/-next//g')
    sed -i "s/ORION_VERSION .*/ORION_VERSION \"$version\"/g" version.h
    cd ${path}

    make rpm

    pack='testing'

fi

if [ -n "${upload}" ]; then
    echo "================================================================="
    echo "                     UPLOADING RPMS                              "
    echo "================================================================="

    cd ${path}/rpm/RPMS/x86_64
    for file in $(ls); do
      echo "Builder: uploading ${file}"
      curl -v -u ${REPO_USER}:${REPO_PASSWORD} --upload-file ${file} ${url_dst}/${releasever}/${basearch}/${pack}/${file};
      if !(curl --output /dev/null --silent --head --fail ${url_dst}/${releasever}/${basearch}/${pack}/${file}); then echo "UPLOAD FAILED!"; exit 1; fi
   done

fi

if [ -n "${show}" ]; then _show; fi
