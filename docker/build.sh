#!/bin/bash

# Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
# Author: Dmitrii Demin <mail@demin.co>
#

set -e

export ROOT='/opt'
export BROKER='orionld'
REV_DEFAULT='develop'
REPOSITORY_DEFAULT='https://github.com/fiware/context.Orion-LD'
STAGE_DEFAULT='release'
OS_DEFAULT='debian'

function _fix_speed()
{
    echo "Builder: fix mongo ttl"

    mongo --eval "db.adminCommand({setParameter:1, ttlMonitorSleepSecs: 3});" --quiet
}

function _unfix_speed()
{
    echo "Builder: revert mongo ttl"
    mongo --eval "db.adminCommand({setParameter:1, ttlMonitorSleepSecs: 60});" --quiet
}

function _usage()
{
  echo -n "Usage: build [options]
  Options:
    -h   --help          show help

    -s   --stage         specify stage (release/deps/rpm/compliance/unit/functional) to use
    -t   --test          run test (compliance/unit/functional)
    -d   --db            start mongo database
    -q   --speed         execute performance fix
    -p   --path          specify path to source code (default - ${ROOT}/orion)
    -o   --os            specify os (centos/debian)

    -b   --build         cmake & make & make install
    -c   --clone         clone repository
    -r   --rev           specify branch/tag/commit to build (default - ${REV_DEFAULT})
    -R   --repository    specify repository to clone (default - ${REPOSITORY_DEFAULT})

    -P   --rpm           specify rpm (nightly, release) to build
    -U   --upload        upload rpm, REPO_USER and REPO_PASSWORD ENV variables should be provided

    -T   --token         specify token to clone k* tools
"
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
       --stage) set -- "$@" -s ;;
       --test) set -- "$@" -t ;;
       --db) set -- "$@" -d ;;
       --speed) set -- "$@" -q ;;
       --path) set -- "$@" -p ;;
       --os) set -- "$@" -o ;;
       --build) set -- "$@" -b ;;
       --clone) set -- "$@" -c ;;
       --rev) set -- "$@" -r ;;
       --repository) set -- "$@" -R ;;
       --rpm) set -- "$@" -P ;;
       --upload) set - "$@" -U ;;
       --token) set -- "$@" -T ;;
       *) set -- "$@" "$arg" ;;
    esac
done

while getopts "hs:tdqp:o:bcr:R:P:UT:" opt; do
    case ${opt} in
        h)  _usage; exit 0 ;;
        s)  STAGE=$OPTARG ;;
        t)  TEST=true ;;
        d)  DATABASE=true ;;
        q)  SPEED=true ;;
        p)  PATH_TO_SRC=${OPTARG} ;;
        o)  OS=${OPTARG} ;;
        b)  BUILD=true ;;
        c)  CLONE=true ;;
        r)  REV=$OPTARG ;;
        R)  REPOSITORY=$OPTARG ;;
        P)  RPM=$OPTARG ;;
        U)  UPLOAD=true ;;
        T)  TOKEN=$OPTARG ;;
        *) _usage && exit 0;;
    esac
done
shift $((OPTIND-1))

echo "Builder: started"

# ==================================== CHECKS =========================================================================
echo "Builder: check user"
if [[ -z "${ORION_USER}" ]]; then
    export ORION_USER=orion
fi

echo "Builder: set path"
if [[ -z "${PATH_TO_SRC}" ]]; then
    PATH_TO_SRC="${ROOT}/orion"
fi
export PATH_TO_SRC

echo "Builder: set revision"
if [[ -z "${REV}" ]]; then
    REV=${REV_DEFAULT}
fi
export REV

echo "Builder: set repository"
if [[ -z "${REPOSITORY}" ]]; then
    REPOSITORY=${REPOSITORY_DEFAULT}
fi
export REPOSITORY

echo "Builder: set stage"
if [[ -z "${STAGE}" ]]; then
    STAGE='release'
fi
export STAGE

echo "Builder: set os"
if [[ -z "${OS}" ]];then
    OS=${OS_DEFAULT}
fi
export OS

if [[ -n "${UPLOAD}" ]]; then
    echo "Builder: checking credentials"

    if [[ -z "${REPO_USER}" ||  -z "${REPO_PASSWORD}" ]]; then
        echo "Builder: failed, REPO_USER or REPO_PASSWORD env variables are not set"
        exit 1
    fi
    if [[ -z "${RPM}" ]]; then
        echo "Builder: failed, RPM type is not set"
        exit 1
    fi
fi

FUNC_STATUS=$(echo ${STAGE} | cut -d '_' -f 1)
if [[ ${FUNC_STATUS} == 'functional' ]]; then

    echo "Builder: checking start & end"
    START=$(echo ${STAGE} | cut -d '_' -f 2 -s)
    END=$(echo ${STAGE} | cut -d '_' -f 3 -s)

    if [[ -n "${END}" && -n "${START}" ]]; then
        FUNC_STATUS=true
        echo "Builder: Start=${START}"
        echo "Builder: End=${END}"
    else FUNC_STATUS=false; fi

    STAGE='functional'
fi

echo "Builder: checking stage"
if [[ -n "${TEST}" && -z "${STAGE}" ]]; then
    echo "Builder: failed, STAGE is not set"
    exit 1
fi

echo "Builder: checking source code"
if [[ -z "${CLONE}" && ${STAGE} != 'deps' && ${STAGE} != 'release' && ! -f ${PATH_TO_SRC}/LICENSE ]]; then
    echo "Builder: failed, source code not found"
    exit 1
fi

echo -n "Builder: status:
    PATH_TO_SRC=${PATH_TO_SRC}
    REV=${REV}
    REPOSITORY=${REPOSITORY}
    STAGE=${STAGE}
    OS=${OS}
    BROKER=${BROKER}
    USER=${ORION_USER}
"

if [[ "${STAGE}" == 'release' || "${STAGE}" == 'deps' ]]; then
    echo "Builder: checking token"
    if [[ -z "${TOKEN}" ]]; then
        echo "Builder: failed, TOKEN is not set"
        exit 1
    fi

    echo "Builder: building starts for OS $OS"
    build-${OS}.sh
    echo "Builder: building ended"
    exit 0
fi

# ===================================== MONGO ==========================================================================
if [[ -n "${DATABASE}" && ! "$(pidof mongod)" ]]; then

    echo "Builder: creating Mongo temp folder"
    rm -Rf /tmp/mongodb || true && mkdir -p /tmp/mongodb

    echo "Builder: starting Mongo"
    mongod --dbpath /tmp/mongodb  --nojournal --quiet > /dev/null 2>&1 &
    sleep 3
    export MONGO_HOST=localhost
fi

# ===================================== CLONE ==========================================================================
if [[ -n "${CLONE}" ]]; then

    echo "Builder: cloning branch ${REV}, from ${REPOSITORY}"

    rm -Rf ${PATH_TO_SRC} || true
    git clone -b ${REV} ${REPOSITORY} ${PATH_TO_SRC}
    chown -R root:root ${PATH_TO_SRC}
fi

# ===================================== BUILDING =======================================================================
if [[ -n "${BUILD}" ]]; then

    echo "Builder: orion installation started"

    cd ${PATH_TO_SRC}

    make install

    if [[ $? -ne 0 ]]; then echo "Builder: installation failed"; exit 1; fi

    strip /usr/bin/${BROKER}

    echo "Builder: orion installation ended"
fi

# ===================================== COMPLIANCE TESTS ===============================================================

if [[ -n "${TEST}" && ${STAGE} == "compliance" ]]; then

    echo "Builder: compliance test started"

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

    echo "Builder: compliance test ended";

fi

# ===================================== UNIT TESTS =====================================================================

if [[ -n "${TEST}" && "${STAGE}" = "unit" ]]; then

    echo "Builder: unit test started"

    cd ${PATH_TO_SRC}

    make unit_test
    if [[ $? -ne 0 ]]; then echo "Builder: unit test failed"; exit 1; fi

    echo "Builder: unit test ended"
fi

# ===================================== FUNCTIONAL TESTS ===============================================================

if [[ -n "${TEST}" && "${STAGE}" = "functional" ]]; then

    echo "Builder: functional test started"

    cd ${PATH_TO_SRC}
    STATUS=true

    if [[ -n "${SPEED}" ]]; then _fix_speed; fi

    make install_scripts
    make install

    . scripts/testEnv.sh

    if ${FUNC_STATUS}; then
        CB_DIFF_TOOL="diff -u" ${PATH_TO_SRC}/test/functionalTest/testHarness.sh --fromIx ${START}  --toIx ${END}
    else
        CB_DIFF_TOOL="diff -u" ${PATH_TO_SRC}/test/functionalTest/testHarness.sh
    fi

    if [[ $? -ne 0 ]]; then STATUS=false; else STATUS=true; fi

    if [[ -n "${SPEED}" ]]; then _unfix_speed; fi

    if ! ${STATUS}; then echo "Builder: functional test failed"; exit 1; fi

    echo "Builder: functional test ended"
fi

# ===================================== BUILDING RPM ===================================================================
if [[ -n "${RPM}" ]]; then
    echo "Builder: ${RPM} rpm building started"
    ch ${PATH_TO_SRC}
    git reset --hard && git clean -qfdx

    if [[ ${RPM} = "nightly" ]]; then

        export BROKER_RELEASE=$(date "+%Y%m%d")

        cd ${PATH_TO_SRC}/src/app/contextBroker
        version=$(cat version.h | grep ORION_VERSION | awk '{ print $3}' | sed 's/"//g' | sed 's/-next//g')
        sed -i "s/ORION_VERSION .*/ORION_VERSION \"$version\"/g" version.h
        cd ${PATH_TO_SRC}

    fi

    if [[ "${RPM}" = "release" ]]; then
        export BROKER_RELEASE=1
    fi

    make rpm

    echo "Builder: ${RPM} rpm building ended"

fi

# ===================================== UPLOADING RPM ==================================================================

if [[ -n "${UPLOAD}" ]]; then

    NEXUS='https://nexus.lab.fiware.org/repository/el'
    VER=7
    ARCH='x86_64'

   echo "Builder: uploading RPM started"
    cd ${PATH_TO_SRC}/rpm/RPMS/x86_64
    for FILE in $(ls); do
      echo "Builder: uploading ${FILE}"
      curl -v -u ${REPO_USER}:${REPO_PASSWORD} --upload-file ${FILE} ${NEXUS}/${VER}/${ARCH}/${RPM}/${FILE};
      if !(curl --output /dev/null --silent --head --fail ${NEXUS}/${VER}/${ARCH}/${RPM}/${FILE}); then
          echo "UPLOAD FAILED!";
          exit 1;
      fi
    done

    echo "Builder: uploading RPM ended"
fi

echo "Builder: ended"
