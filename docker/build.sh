#!/bin/bash

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
# Author: Dmitrii Demin
#

set -e

export DEBIAN_FRONTEND=noninteractive
export REPOSITORY_SRC='https://github.com/fiware/context.Orion-LD'
export BRANCH_SRC='develop'
export ROOT='/opt'

function _usage()
{
  echo -n "Usage: build [options]
  Options:
    -h   --help          show help
    -H   --show          show the list of necessary commands that should be executed before starting functional tests

    -s   --stage         specify stage (release/deps/compliance/unit/functional) to use
    -t   --test          run test (compliance/unit/functional)
    -d   --db            start mongo database
    -q   --speed         execute fix for functional tests during testing (improve speed)
    -p   --platform      specify platform (debian), required if stage is specified
    -t   --token         specify token to clone k* tools, required if stage is specified
    -P   --path          specify path to source code (default - /opt/orion)

    -b   --build         cmake & make & make install
    -B   --branch        specify branch/tag to build (default - ${BRANCH_SRC})
    -R   --repository    specify repository to clone (default - ${REPOSITORY_SRC})

    -r   --rpm           specify rpm (nightly, release, testing) to build - only for centos
    -u   --upload        upload rpm, REPO_USER and REPO_PASSWORD ENV variables should be provided only for centos
"
}

function _fix_speed()
{
    echo "Builder: fix speed"

    echo "Builder: fix mongo sleep parameter"
    mongo --eval "db.adminCommand({setParameter:1, ttlMonitorSleepSecs: 3});"
}

function _unfix_speed()
{
    echo "Builder: revert speed fix"
    mongo --eval "db.adminCommand({setParameter:1, ttlMonitorSleepSecs: 60});"
}

function _show()
{
    echo "Builder: run this commands"
    echo ". scripts/testEnv.sh"
}

[[ $# = 0 ]] && _usage && exit 1
reset=true

for arg in "$@"
do
    if [[ -n "$reset" ]]; then
      unset reset
      set --
    fi
    case "$arg" in
       --help) set -- "$@" -h ;;
       --show) set -- "$@" -H ;;
       --stage) set -- "$@" -s ;;
       --test) set -- "$@" -t ;;
       --db) set -- "$@" -d ;;
       --speed) set -- "$@" -q ;;
       --platform) set -- "$@" -p ;;
       --token) set -- "$@" -t ;;
       --path) set -- "$@" -P ;;
       --build) set -- "$@" -b ;;
       --clone) set -- "$@" -c ;;
       --branch) set -- "$@" -B ;;
       --repository) set -- "$@" -R ;;
       --rpm) set -- "$@" -r ;;
       --upload) set - "$@" -u ;;

       *) set -- "$@" "$arg" ;;
    esac
done

while getopts "hHs:tdqp:T:P:bcB:R:r:u" opt; do
    case ${opt} in
        h)  _usage; exit 0 ;;
        H)  SHOW=true ;;
        s)  STAGE=$OPTARG ;;
        t)  TEST=true ;;
        d)  DATABASE=true ;;
        q)  SPEED=true ;;
        p)  PLATFORM=$OPTARG ;;
        T)  TOKEN=$OPTARG ;;
        P)  PATH_TO_SRC=${OPTARG} ;;
        b)  BUILD=true ;;
        c)  CLONE=true ;;
        B)  BRANCH=$OPTARG ;;
        R)  REPOSITORY=$OPTARG ;;
        r)  RPM=$OPTARG ;;
        u)  UPLOAD=true ;;
        *) _usage && exit 0;;
    esac
done
shift $((OPTIND-1))

# ==================================== CHECKS =========================================================================

echo -n "Builder:
stage - \"${STAGE}\"
platform - \"${PLATFORM}\"
test - \"${TEST}\"
repository - \"${REPOSITORY}\"
branch - \"${BRANCH}\"
"

echo "Builder: check path"
if [[ -z "${PATH_TO_SRC}" ]]; then
    PATH_TO_SRC="${ROOT}/orion"
fi

echo "Builder: checking release/deps"
if [[ "${STAGE}" == 'release' || "${STAGE}" == 'deps' ]]; then
    echo "Builder: checking platform"
    if [[ -z "${PLATFORM}]" ]]; then
        echo "Builder: failed, PLATFORM is not set"
        exit 1
    fi

    echo "Builder: checking token"
    if [[ -z "${TOKEN}" ]]; then
        echo "Builder: failed, TOKEN is not set"
        exit 1
    fi

    build-${PLATFORM}.sh --token ${TOKEN} --stage ${STAGE}
    exit 0
fi

echo "Builder: checking credentials"
if [[ -n "${UPLOAD}" && ${PLATFORM} == 'censos' ]]; then
    if [[ -z "${REPO_USER}" ||  -z "${REPO_PASSWORD}" ]]; then
        echo "Builder: failed, REPO_USER or REPO_PASSWORD env variables are not set"
        exit 1
    fi
    if [[ -z "${RPM}" ]]; then
        echo "Builder: failed, RPM type is not set"
        exit 1
    fi
fi

echo "Builder: checking start & end"
FUNC_STATUS=$(echo ${STAGE} | cut -d '_' -f 1)
START=$(echo ${STAGE} | cut -d '_' -f 2)
END=$(echo ${STAGE} | cut -d '_' -f 3)

if [[ ${FUNC_STATUS} == 'functional' ]]; then
    STAGE='functional'
    if [[ -n "${START}" ]]; then FUNC_STATUS=true; else FUNC_STATUS=false; fi
    if [[ -n "${END}" && -n ${START} ]]; then FUNC_STATUS=true; else FUNC_STATUS=false; fi

fi

echo "Builder: checking stage"
if [[ -n "${TEST}" && -z "${STAGE}" ]]; then
    echo "Builder: failed, STAGE is not set"
    exit 1
fi

echo "Builder: checking source code"
if [[ -z "${CLONE}" && ! -f ${PATH_TO_SRC}/LICENSE ]]; then echo "Builder: failed, source code not found"; exit 1; fi

# ===================================== MONGO ==========================================================================
if [[ -n "${DATABASE}" ]]; then

    echo "Builder: creating Mongo temp folder"
    rm -Rf /tmp/mongodb || true && mkdir -p /tmp/mongodb

    echo "Builder: starting Mongo"
    mongod --dbpath /tmp/mongodb  --nojournal --quiet > /dev/null 2>&1 &
    sleep 3
    export MONGO_HOST=localhost
fi

# ===================================== CLONE ==========================================================================
if [[ -n "${CLONE}" ]]; then

    if [[ -z "${BRANCH}" ]]; then BRANCH=${BRANCH_SRC}; fi
    if [[ -z "${REPOSITORY}" ]]; then REPOSITORY=${REPOSITORY_SRC}; fi

    echo "Builder: cloning branch ${BRANCH}, from ${REPOSITORY}"

    git clone -b ${BRANCH} ${REPOSITORY} ${PATH_TO_SRC}
    chown -R root:root ${PATH_TO_SRC}
fi

# ===================================== BUILDING =======================================================================
if [[ -n "${BUILD}" ]]; then

    echo "Builder: installing orion"

    cd ${PATH_TO_SRC}

    make install

    if [[ $? -ne 0 ]]; then echo "Builder: installation failed"; exit 1; fi

    strip /usr/bin/${BROKER}
fi

# ===================================== COMPLIANCE TESTS ===============================================================

if [[ -n "${TEST}" && ${STAGE} == "compliance" ]]; then

    echo "Builder: executing compliance test"

    cd ${PATH_TO_SRC}

    STATUS=true

    make files_compliance
    if [[ $? -ne 0 ]]; then STATUS=false; fi

    make payload_check
    if [[ $? -ne 0 ]]; then STATUS=false; fi

    make style
    if [[ $? -ne 0 ]]; then STATUS=false; fi

    rm -Rf LINT*

    if ! ${STATUS}; then echo "Builder: compliance test failed"; exit 1; fi

fi

# ===================================== UNIT TESTS =====================================================================

if [[ -n "${TEST}" && "${STAGE}" = "unit" ]]; then

    echo "Builder: executing unit test"

    cd ${PATH_TO_SRC}

    make unit_test
    if [[ $? -ne 0 ]]; then echo "Builder: unit test failed"; exit 1; fi

fi

# ===================================== FUNCTIONAL TESTS ===============================================================

if [[ -n "${TEST}" && "${STAGE}" = "functional" ]]; then

    echo "Builder: executing functional test"

    cd ${PATH_TO_SRC}
    STATUS=true

    if [[ -n "${SPEED}" ]]; then _fix_speed; fi

    make install_scripts
    make install

    . scripts/testEnv.sh

    if ${FUNC_STATUS}; then
        CB_DIFF_TOOL="diff -u" ${PATH_TO_SRC}/test/functionalTest/testHarness.sh --fromIx ${START}  --toIx ${END}
    else:
        CB_DIFF_TOOL="diff -u" ${PATH_TO_SRC}/test/functionalTest/testHarness.sh
    fi

    if [[ $? -ne 0 ]]; then STATUS=false; else STATUS=true; fi

    if [[ -n "${SPEED}" ]]; then _unfix_speed; fi

    if ! ${STATUS}; then echo "Builder: functional test failed"; exit 1; fi

fi

# ===================================== OTHERS =========================================================================
if [[ -n "${SHOW}" ]]; then _show; fi
