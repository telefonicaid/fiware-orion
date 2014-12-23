#!/bin/bash
#
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

# Bash lib to know the RPM version and revision from a PDIHub repository
# Call method get_rpm_version_string to obtain them for rpmbuild
#
shopt -s extglob

get_branch()
{
    git rev-parse --abbrev-ref HEAD
}

## PDI specific functions according the pdihub workflow
get_branch_type()
{
    local branch="$(get_branch)"
    case $branch in
        feature/*|bug/*|hotfix/*) echo "unstable";;
        release/*) echo "release";;
        +(+([[:digit:]])\.)+([[:digit:]]) ) echo "release" ;;
        develop) echo "develop";;
        master) echo "stable";;
        *) echo "other";;
    esac
}

get_version_string()
{
    if [[ $(is_pdi_compliant) -eq 0 ]]; then # Not PDI compliant, return a dummy version
        echo "HEAD-0-g$(git log --pretty=format:'%h' -1)"
        return
    fi
    local branch describe_all describe_tags version ancestor
    describe_all="$(git describe --all --long)"
    describe_tags="$(git describe --tags --long 2>/dev/null)"
    [[ "${describe_tags}" == "${describe_all#*/}" ]] && version="${describe_tags%/*}" || version="${version#*/}"
    case $(get_branch_type) in
        stable)
           # If we are on stable branch get last tag as the version, but transform to x.x.x-x-SHA1
           version="${describe_tags%-*-*}"
           echo "${version%.*}-${version#*.*.*.}-g$(git log --pretty=format:'%h' -1)"
        ;;
        develop)
           ## if we are in develop use the total count of commits
           version=$(git describe --tags --long --match */KO)
           echo "${version%/*}-${version#*KO-}"
        ;;
        release)
           version=$(get_branch)
           version=$(git describe --tags --long --match ${version#release/*}/KO)
           echo "${version%/*}-${version#*KO-}"
        ;;
        other)
            ## We are in detached mode, use the last KO tag
            version=$(git describe --tags --long --match */KO)
            echo "${version%/*}-${version#*KO-}"
        ;;
        *)
           # RMs don't stablish any standard here, we use branch name as version
           version=$(get_branch)
           # Using always develop as parent branch does not describe correctly the number of revision
           # for branches not starting there, but works as an incremental rev
           ancestor="$(git merge-base $version develop)"
           version=${version#*/}
           local res="$(git log --oneline ${ancestor}.. --pretty='format:%h')"
           ## wc alone does not get the last line when there's no new line
           [[ -z $res ]] && rel=0 || rel=$(echo "$res" | wc -l | tr -d ' ')
           echo "${version}-${rel}-g$(git log --pretty=format:'%h' -1)"
    esac
}

## Parse the version string and sanitize it
## to use it you can do, for example:
## ># read ver rel < <(get_rpm_version_string)
get_rpm_version_string() {
    local version_string ver rel
    version_string="$(get_version_string)"
    ver="${version_string%-*-*}"
    rel="${version_string:$((${#ver}+1))}"
    echo "${ver//[[:space:]-\/#]}" "${rel//[-]/.}"
}

## DEPRECATED: use get_rpm_version_string instead
get_pdi_version_string()
{
    get_rpm_version_string
}

is_pdi_compliant()
{
    case $(get_branch_type) in
    "other")
       # Maybe we are on detached mode but also are compliant
       # See if there's a tag (annotated or not) describing a Kick Off
        git describe --tags --match */KO >/dev/null 2>/dev/null
        if [ $? -eq 0 ]; then
            echo 1
        else
            echo 0
        fi
    ;;
    "release")
        ver=$(get_branch)
        # remove the leading release/ if necessary
        ver=${ver#release/*}
        # see if there's a tag (annotated or not) describing its Kick Off
        git describe --tags --match ${ver}/KO >/dev/null 2>/dev/null
        if [ $? -eq 0 ]; then
            echo 1
        else
            echo 0
        fi
    ;;
    "develop")
        # see if there's a tag (annotated or not) describing a Kick Off
        git describe --tags --match */KO >/dev/null 2>/dev/null
        if [ $? -eq 0 ]; then
            echo 1
        else
            echo 0
        fi
    ;;
    *)  echo 1 ;;
   esac
}

#cd $1
#get_version_string| cut -d "-" -f $2
