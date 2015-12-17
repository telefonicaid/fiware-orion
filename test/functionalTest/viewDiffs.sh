# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

if [ -z "$CB_DIFF_TOOL" ] 
then
    echo "Need to set env-var CB_DIFF_TOOL"
    exit 1
fi  

if [ "$#" == 1 ]
then
  base=$(basename $1 .test)
  out=$(find . -name ${base}.out)
  exp=$(find . -name ${base}.regexpect)

  if [ "$out" == "" ]
  then
    echo "Sorry, ${base}.out NOT FOUND - using ${base}.shell.stdout instead"
    out=$(find . -name ${base}.shell.stdout)
    if [ "$out" == "" ]
    then
      echo "Sorry, ${base}.shell.stdout ALSO NOT FOUND - no diff is possible"
      exit 1
    fi
  fi

  echo $CB_DIFF_TOOL $out $exp
  $CB_DIFF_TOOL $out $exp
  exit $?
fi


outFiles=$(find . -name '*.out' | grep -v valgrind | sort)
for outFile in $outFiles
do
  # Search for the corresponging .regexpect file
  base=$(basename $outFile)
  dir=$(dirname $outFile)
  baseWithoutExtension="${base%.*}"
  regexpectFile=$dir/$baseWithoutExtension.regexpect
  testFile=$dir/$baseWithoutExtension.test
  if [ -f "$regexpectFile" ]
  then
    echo
    if [ ! -f "$testFile" ]
    then
      echo "WARNING: '$testFile' doesn't exist, this may be a false positive"
    fi
    read -p "View diff for '$outFile'? [Y/n] " yn    
    case $yn in
      [Yy]* ) answer="yes" ;;
      [Nn]* ) answer="no" ;;
      * ) answer="yes" ;;
    esac
    if [ "$answer" = "yes" ]
    then
       $CB_DIFF_TOOL $outFile $regexpectFile
    fi    
  fi
done
