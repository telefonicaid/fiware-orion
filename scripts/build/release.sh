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

# ------------------------------------------------------------------------------
#
# Example execution:
#   scripts/build/release.sh 0.4.0 1 changelog
#
progName=$0


#
# usage
#
function usage
{
  echo "$progName <NEW_VERSION> <BROKER_RELEASE> <changelog-file>"
  exit 1
}



#
# Checking command line parameters
#
if [ "$1" == "-u" ]
then
  usage
fi

if [ $# != 3 ]
then
  usage
fi


#
# Make sure there are no occurrences of LM_TMP in the source code
#
find . -name "*.cpp" -exec grep LM_TMP {} /dev/null \; | grep -v '// LM_TMP'              > /tmp/LM_TMP
find . -name "*.h"   -exec grep LM_TMP {} /dev/null \; | grep -v src/lib/logMsg/logMsg.h >> /tmp/LM_TMP
lines=$(wc -l /tmp/LM_TMP)
\rm -f /tmp/LM_TMP

if [ "$lines" != "0 /tmp/LM_TMP" ]
then
  echo "occurrences of LM_TMP found - release aborted"
  exit 1
fi



#
# Command line parameters
#
export NEW_VERSION=$1
export BROKER_RELEASE=$2
export CHANGELOG_FILE=$3



#
# correct date format
#
DATE=$(LANG=C date +"%a %b %d %Y")
export dateLine="$DATE Fermin Galan <fermin.galanmarquez@telefonica.com> ${NEW_VERSION}-${BROKER_RELEASE}"


# Modify rpm/SPECS/contextBroker.spec only when step to a non-dev release
if [ "$BROKER_RELEASE" != "dev" ]
then
    #
    # Edit rpm/SPECS/contextBroker.spec, adding the new changes from CHANGELOG_FILE
    #
    # 1. Find the line in rpm/SPECS/contextBroker.spec, where to add the content of CHANGELOG_FILE plus the info-line for the changes.
    #    o LINES:       number of lines before the insertion
    # 2. Get the total number of lines in rpm/SPECS/contextBroker.spec
    # 3. Get the number of lines in rpm/SPECS/contextBroker.spec after the insertion
    #    o LAST_LINES:  number of lines after the insertion
    # 4. To a temporal file, add the four 'chunks':
    #    1. LINES
    #    2. the info-line for the changes
    #    3. the content of CHANGELOG_FILE
    #    4. LAST_LINES
    # 5. Replace using the temporal file

    #
    # 1. Find the line in rpm/SPECS/contextBroker.spec, where to add the content of CHANGELOG_FILE
    #    The for is because these is more than one oceuurence of '%changelog'. We are only
    #    interested in the last one.
    #
    for line in $(grep -n '%changelog' rpm/SPECS/contextBroker.spec | awk -F: '{ print $1 }')
    do
      LINE=$line
    done


    #
    # 2. Get the total number of lines in rpm/SPECS/contextBroker.spec
    #
    LINES=$(wc -l rpm/SPECS/contextBroker.spec | awk '{ print $1 }')


    #
    # 3. Get the number of lines in rpm/SPECS/contextBroker.spec after the insertion
    #
    typeset -i LAST_LINES
    LAST_LINES=$LINES-$LINE


    #
    # 4. To a temporal file, add the four 'chunks'
    #
    head -$LINE rpm/SPECS/contextBroker.spec        >  /tmp/contextBroker.spec

    echo -n '* '                                    >> /tmp/contextBroker.spec
    echo $dateLine                                  >> /tmp/contextBroker.spec

    cat $CHANGELOG_FILE                             >> /tmp/contextBroker.spec
    echo                                            >> /tmp/contextBroker.spec

    tail -$LAST_LINES rpm/SPECS/contextBroker.spec  >> /tmp/contextBroker.spec
    
    # 5. Replace using the temporal file
    mv /tmp/contextBroker.spec rpm/SPECS/contextBroker.spec 

fi


#
# Get the current version (maintained in src/app/contextBroker/version.h)
#
currentVersion=$(grep "define ORION_VERSION" src/app/contextBroker/version.h  | awk -F\" '{ print $2 }')

echo "current version: $currentVersion"
echo "new version:     $NEW_VERSION"


#
# Edit files that depend on the current version (which just changed)
#
sed "s/$currentVersion/$NEW_VERSION/" src/app/contextBroker/version.h > /tmp/version.h
mv /tmp/version.h src/app/contextBroker/version.h


# Clean the inter-release changes file
rm -rf CHANGES_NEXT_RELEASE
touch CHANGES_NEXT_RELEASE

# Adjust Readthedocs documentation badge. Note that the procedure is not symmetric (like in version.h), as
# dev release sets 'latest' and not 'X.Y.Z-next"
if [ "$BROKER_RELEASE" != "dev" ]
then
  sed "s/(https:\/\/img.shields.io\/readthedocs\/fiware-orion.svg)](https:\/\/fiware-orion.rtfd.io)/(https:\/\/img.shields.io\/readthedocs\/fiware-orion.$NEW_VERSION.svg)](https:\/\/fiware-orion.rtfd.io\/en\/$NEW_VERSION\/)/" README.md > /tmp/README.md
else
  sed "s/(https:\/\/img.shields.io\/readthedocs\/fiware-orion\/$currentVersion.svg)](https:\/\/fiware-orion.rtfd.io\/en\/$currentVersion\/)/(https:\/\/img.shields.io\/readthedocs\/fiware-orion.svg)](https:\/\/fiware-orion.rtfd.io)/" README.md > /tmp/README.md
fi
mv /tmp/README.md README.md

# Adjust Readthedocs documentation link for GET /version response. Note that the procedure is not symmetric 
# (like in version.h), as dev release sets 'master' and not 'X.Y.Z-next"
if [ "$BROKER_RELEASE" != "dev" ]
then
  sed "s/https:\/\/fiware-orion.rtfd.io\//https:\/\/fiware-orion.rtfd.io\/en\/$NEW_VERSION\//" src/lib/common/defaultValues.h > /tmp/defaultValues.h
else
  sed "s/https:\/\/fiware-orion.rtfd.io\/en\/$currentVersion\//https:\/\/fiware-orion.rtfd.io\//" src/lib/common/defaultValues.h > /tmp/defaultValues.h
fi
mv /tmp/defaultValues.h src/lib/common/defaultValues.h


# Adjust Dockerfile GIT_REV_ORION. Note that the procedure is not symmetric (like in version.h), as
# dev release sets 'master' and not 'X.Y.Z-next"
if [ "$BROKER_RELEASE" != "dev" ]
then
  sed "s/ENV GIT_REV_ORION master/ENV GIT_REV_ORION $NEW_VERSION/" docker/Dockerfile > /tmp/Dockerfile
else
  sed "s/ENV GIT_REV_ORION $currentVersion/ENV GIT_REV_ORION master/" docker/Dockerfile > /tmp/Dockerfile
fi
mv /tmp/Dockerfile docker/Dockerfile

#
# Do the git stuff only if we are in master branch
#
CURRENT_BRANCH=$(git branch | grep '^*' | cut -c 3-10)
if [ "$CURRENT_BRANCH" == "master" ]
then
    git add rpm/SPECS/contextBroker.spec
    git add src/app/contextBroker/version.h
    git add src/lib/common/defaultValues.h
    git add CHANGES_NEXT_RELEASE
    git add README.md
    git add docker/Dockerfile
    git commit -m "Step: $currentVersion -> $NEW_VERSION"
    git push origin master
    # We do the tag only and merge to master only in the case of  non "dev" release
    if [ "$BROKER_RELEASE" != "dev" ]
    then
       git checkout -b release/$NEW_VERSION
       git tag $NEW_VERSION
       git push --tags origin release/$NEW_VERSION
       git push origin release/$NEW_VERSION

       # Build release only when step to a non-dev release. Note that, taking into account
       # how the "make rpm" target works, it has to be done after commit has been done
       make rpm

       git checkout $CURRENT_BRANCH
    fi
else
    echo "Your current branch is $CURRENT_BRANCH. You need to be at master branch to do the final part of the process"
fi
