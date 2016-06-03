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
#   scripts/build/release.sh 0.4.0 4 2.3.3 4 changelog
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
# Chewcking command line parameters
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
sed "s/$currentVersion/$NEW_VERSION/" test/functionalTest/cases/0000_version_operation/version_via_rest.test > /tmp/version_via_rest.test
sed "s/$currentVersion/$NEW_VERSION/" test/functionalTest/cases/0000_cli/version.test                        > /tmp/version.test
sed "s/$currentVersion/$NEW_VERSION/" src/app/contextBroker/version.h        > /tmp/version.h

mv /tmp/version_via_rest.test  test/functionalTest/cases/0000_version_operation/version_via_rest.test
mv /tmp/version.test           test/functionalTest/cases/0000_cli/version.test
mv /tmp/version.h              src/app/contextBroker/version.h


# Clean the inter-release changes file
rm -rf CHANGES_NEXT_RELEASE
touch CHANGES_NEXT_RELEASE

# Adjust Readthedocs documentation badge. Note that the procedure is not symmetric (like in version.h), as
# dev release sets 'latest' and not 'X.Y.Z-next"
if [ "$BROKER_RELEASE" != "dev" ]
then
  sed "s/(https:\/\/readthedocs.org\/projects\/fiware-orion\/badge\/?version=latest)](http:\/\/fiware-orion.readthedocs.org\/en\/latest\/?badge=latest)/(https:\/\/readthedocs.org\/projects\/fiware-orion\/badge\/?version=$NEW_VERSION)](http:\/\/fiware-orion.readthedocs.org\/en\/$NEW_VERSION\/?badge=$NEW_VERSION)/" README.md > /tmp/README.md
else
  sed "s/(https:\/\/readthedocs.org\/projects\/fiware-orion\/badge\/?version=$currentVersion)](http:\/\/fiware-orion.readthedocs.org\/en\/$currentVersion\/?badge=$currentVersion)/(https:\/\/readthedocs.org\/projects\/fiware-orion\/badge\/?version=latest)](http:\/\/fiware-orion.readthedocs.org\/en\/latest\/?badge=latest)/" README.md > /tmp/README.md
fi
mv /tmp/README.md README.md

# Adjust Dockerfile GIT_REV_ORION. Note that the procedure is not symmetric (like in version.h), as
# dev release sets 'develop' and not 'X.Y.Z-next"
if [ "$BROKER_RELEASE" != "dev" ]
then
  sed "s/ENV GIT_REV_ORION develop/ENV GIT_REV_ORION $NEW_VERSION/" docker/Dockerfile > /tmp/Dockerfile
else
  sed "s/ENV GIT_REV_ORION $currentVersion/ENV GIT_REV_ORION develop/" docker/Dockerfile > /tmp/Dockerfile
fi
mv /tmp/Dockerfile docker/Dockerfile

#
# Do the git stuff only if we are in develop branch
#
CURRENT_BRANCH=$(git branch | grep '^*' | cut -c 3-10)
if [ "$CURRENT_BRANCH" == "develop" ]
then
    git add rpm/SPECS/contextBroker.spec
    git add src/app/contextBroker/version.h
    git add test/functionalTest/cases/0000_cli/version.test
    git add test/functionalTest/cases/0000_version_operation/version_via_rest.test
    git add CHANGES_NEXT_RELEASE
    git add README.md
    git add docker/Dockerfile
    git commit -m "Step: $currentVersion -> $NEW_VERSION"
    git push origin develop
    # We do the tag only and merge to master only in the case of  non "dev" release
    if [ "$BROKER_RELEASE" != "dev" ]
    then
       git checkout master
       git pull
       git merge develop
       git push origin master
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
    echo "Your current branch is $CURRENT_BRANCH. You need to be at develop branch to do the final part of the process"
fi
