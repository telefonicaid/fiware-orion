# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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

function usage()
{
  sfile="Usage: "$(basename $0)
  empty=$(echo $sfile | tr 'a-zA-z/0-9.:' ' ')
  echo "$sfile [-u (usage)]"
  echo "$empty [-m <files|commit|tag|push>"
  echo "$empty [-c <changelog file to flush into Changelog, default is CHANGES_NEXT_RELEASE>"
  echo
  echo "Modes:"
  echo "* 'files', only modify files (but don't commit changes)"
  echo "* 'commit', modify files and commit changes (but don't tag)"
  echo "* 'tag', modify files, commit and tag (but don't push)"
  echo "* 'push', modify files, commit, tag and push to origin"
  echo
  exit $1
}


# from https://stackoverflow.com/a/21370675/1485926
function daySuffix() {
  case `date +%d` in
    1|21|31) echo "st";;
    2|22)    echo "nd";;
    3|23)    echo "rd";;
    *)       echo "th";;
  esac
}


function checkValidInteger()
{
  if [ "$1" == "" ]; then
    echo $0: at least one of the three tag tokens is empty
    exit 1
  fi

  if ! [[ $1 =~ ^[0-9]+$ ]]; then
    echo $0: tag token '"'${$1}'"' is not a number
    exit 1
  fi
}

#
# Modify Changelog file only when step to a non-dev release
# 
# FIXME: unify this code with the same one in resease.sh into a commmon lib
#
function flushCNRToChangelog()
{
  CHANGELOG_FILE=Changelog
  TEMP_FILE=$(mktemp)

  echo $dateLine       >  $TEMP_FILE
  echo                 >> $TEMP_FILE
  cat $changelog       >> $TEMP_FILE
  echo                 >> $TEMP_FILE
  cat $CHANGELOG_FILE  >> $TEMP_FILE
    
  mv $TEMP_FILE $CHANGELOG_FILE

  rm $changelog; touch $changelog
}



# ------------------------------------------------------------------------------
#
# Argument parsing
#
files=off
commit=off
tag=off
push=off

changelog=CHANGES_NEXT_RELEASE

mode=""

while getopts ":m:c:u" opt
do
  case $opt in
    m)
      mode=${OPTARG}
      ;;
    c)
      changelog=${OPTARG}
      ;;
    u)
      usage
      exit 0
      ;;
    *)
      echo $0: bad parameter/option: "'"${OPTARG}"'"
      echo
      usage
      exit 1
      ;;
  esac
done

if [ "$mode" == "" ]; then
  echo $0: missing mode
  echo
  usage 1
fi

if [ "$mode" == "files" ]; then
  files=on  # not actually used, but included for "symetry"
elif [ "$mode" == "commit" ]; then
  commit=on  
elif [ "$mode" == "tag" ]; then
  commit=on
  tag=on
elif [ "$mode" == "push" ]; then
  commit=on
  tag=on
  push=on
else
  echo $0: wrong mode: "'"${mode}"'";
  echo
  usage 1
fi

# Check is run from the repository home
if ! [ -e ".git/config" ]; then
  echo $0: the script has to run from the repository root
  exit 1
fi

# Check we are in a release/X.Y.Z branch
branch=$(git rev-parse --abbrev-ref HEAD)
if [[ $branch != release/* ]]; then
  echo $0: you are not in a release branch but in "'"${branch}"'"
  exit 1
fi


# Get current tag
currentTag=$(grep "define ORION_VERSION" src/app/contextBroker/version.h  | awk -F\" '{ print $2 }')
echo $0: current tag is: "'"${currentTag}"'"

# Check tag structure
X=$(echo $currentTag | awk -F '.' '{print $1}')
Y=$(echo $currentTag | awk -F '.' '{print $2}')
Z=$(echo $currentTag | awk -F '.' '{print $3}')
checkValidInteger $X
checkValidInteger $Y
checkValidInteger $Z

# Check branch match base version
baseVersion=$X.$Y.0
echo $0: base version is: "'"${baseVersion}"'"
versionFromBranch=$(echo $branch | awk -F '/' '{print $2'})
if [ "$baseVersion" != "$versionFromBranch" ]; then
  echo $0: wrong base version in this release branch: "'"${versionFromBranch}"'"
  exit 1
fi

# Check dockerfile has the right tag
grep "ENV GIT_REV_ORION $currentTag" docker/Dockerfile > /dev/null
if [ "$?" != "0" ]; then
  echo $0: GIT_REV_ORION does not use current tag in Dockerfile
  exit 1
fi

# Calculate next tag
Z=$((Z+1))
nextTag=$X.$Y.$Z
echo $0: next tag is: "'"${nextTag}"'"

# Modify src/app/contextBroker/version.h
sed "s/$currentTag/$nextTag/" src/app/contextBroker/version.h > /tmp/version.h
mv /tmp/version.h src/app/contextBroker/version.h

# Flush CNR into Changelog
DATE=$(LANG=C date +"(%B %-d`daySuffix`, %Y)")
export dateLine="${nextTag} $DATE"
flushCNRToChangelog

# Modify ENV GIT_REV_ORION at docker/Dockerfile
sed "s/ENV GIT_REV_ORION $currentTag/ENV GIT_REV_ORION $nextTag/" docker/Dockerfile > /tmp/Dockerfile
mv /tmp/Dockerfile docker/Dockerfile

# Commit all files
if [ "$commit" == "on" ]; then
  git add src/app/contextBroker/version.h $changelog docker/Dockerfile Changelog
  git commit -m "Step: $currentTag -> $nextTag"
fi

# Create tag
if [ "$tag" == "on" ]; then
  git tag $nextTag
fi

# Push
if [ "$push" == "on" ]; then
  git push --tags origin release/$baseVersion
fi

exit 0
