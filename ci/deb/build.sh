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
releasever=8
basearch='x86_64'

function _usage()
{
  echo -n "Usage: build [options]
  Options:
    -h   --help          show help
    -b   --branch        specify branch/tag to build, if not specified - the source from /opt/fiware-orion will be used
    -S   --source        specify repository to clone, if not specified - default repo (https://github.com/telefonicaid/fiware-orion) will be used
    -p   --path          specify path to use as home, if not specified - /opt/fiware-orion will be used
    -s   --stage         specify stage (unit/functional/compliance/valgrind) to use
    -m   --make          cmake/make (works with unit/functional/valgrind stage only)
    -i   --install       make install (works with unit/functional/valgrind stage only)
    -t   --test          run unit/functional/compliance/valgrind test (depends on the defined stage)
    -j   --jenkins       execute fix for jenkins during functional/valgrind testing (disable ipv6 test)
    -J                   execute fix for jenkins
    -q   --speed         execute fix for functional/valgrind tests during testing (improve speed)
    -Q                   execute fix for functional/valgrind tests
    -a   --attempts      attempts to execute functional test
    -e   --execute       run (rerun) test stand with 2 orions
    -H   --show          show the list of necessary commands that should be executed before starting functional tests manually

  Examples:
    build -s unit -mit -b master    clone from master, make (for unit testing), make install, execute unit tests
    build -s functional -mitq       get source from mounted folder, make (for functional testing), make install, execute speed FIX, functional test
"
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
    if [ ! -f "${path}/ci/deb/makefile" ]; then tp="/opt/archive/makefile"; else tp="${path}/ci/deb/makefile"; fi
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
       --test) set -- "$@" -t ;;
       --jenkins) set -- "$@" -j ;;
       --speed) set -- "$@" -q ;;
       --execute) set -- "$@" -e ;;
       --show) set -- "$@" -H ;;
       --attempts) set -- "$@" -a ;;
       *) set -- "$@" "$arg" ;;
    esac
done

while getopts ":hb:S:p:s:mitr:ujJeHa:" opt; do
    case ${opt} in
        h)  _usage; exit 0 ;;
        b)  branch=$OPTARG ;;
        S)  source=$OPTARG ;;
        p)  path=$OPTARG ;;
        s)  stage=$OPTARG ;;
        m)  make=true;;
        i)  install=true;;
        t)  test=true ;;
        j)  fix_j=true ;;
        J)  fix_J=true ;;
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

echo "Builder: if upload is on, check if credentials exist"
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

echo "===================================== PREPARE =========================================="

echo "Builder: create temp folders"
rm -Rf /tmp/builder || true && mkdir -p /tmp/builder/{db1,db2,db,bu}

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

    # Build for valgrind is the same as build for functional tests
    if [ "${stage}" = "valgrind" ]; then
      build_stage="functional"
    else
      build_stage=${stage}
    fi

    _fix
    echo "Builder: build ${build_stage}"
    make build_${build_stage}
    if [ $? -ne 0 ]; then echo "Builder: make failed"; exit 1; fi
    _unfix

fi

if [ -n "${install}" ]; then
    echo "===================================== INSTALL =========================================="

    # Install for valgrind is same as install for functional
    if [ "${stage}" = "valgrind" ]; then
      install_stage="functional"
    else
      install_stage=${stage}
    fi

    _fix
    echo "Builder: install ${install_stage}"
    make install_${install_stage}
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

    _fix

    if [ -n "${attempts}" ]; then export CB_MAX_TRIES=${attempts}; fi

    make functional INSTALL_DIR=~
    if [ $? -ne 0 ]; then status=false; else status=true; fi

    _unfix

    if [ -n "${fix_j}" ]; then _unfix_jenkins; fi

    if ! ${status}; then echo "Builder: functional test failed"; exit 1; fi

fi

if [ -n "${test}" ] && [ "${stage}" = "valgrind" ]; then
    echo "===================================== VALGRIND TESTS ================================="

    if [ -n "${fix_j}" ]; then _fix_jenkins; fi

    _fix

    . scripts/testEnv.sh
    . /opt/ft_env/bin/activate
    test/valgrind/valgrindTestSuite.sh
    if [ $? -ne 0 ]; then status=false; else status=true; fi

    _unfix

    if [ -n "${fix_j}" ]; then _unfix_jenkins; fi

    if ! ${status}; then echo "Builder: valgrind test failed"; exit 1; fi

fi


if [ -n "${show}" ]; then _show; fi
if [ -n "${fix_J}" ]; then _fix_jenkins; fi
