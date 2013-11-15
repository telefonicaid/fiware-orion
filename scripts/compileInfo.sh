#!/bin/bash
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
# fermin at tid dot es

if [ "$1" == "--release" ]; then
    date=$(LANG=C LC_TIME=C date)
else
    date=nodate
fi
git_hash=$(git log | grep commit | head -1 | awk '{ print $2 }')
user=$(whoami)
host=$(hostname)

if [ -n "$git_hash" ]; then
    hash=$git_hash
else
    hash=nogitversion
fi

TMP_FILE=$(mktemp /tmp/compileInfo.h.XXXX)
echo '#ifndef COMPILE_INFO_H'                >  $TMP_FILE
echo '#define COMPILE_INFO_H'                >> $TMP_FILE
echo                                         >> $TMP_FILE
echo '#define GIT_HASH         "'${hash}'"'  >> $TMP_FILE
echo '#define COMPILE_TIME     "'${date}'"'  >> $TMP_FILE
echo '#define COMPILED_BY      "'${user}'"'  >> $TMP_FILE
echo '#define COMPILED_IN      "'${host}'"'  >> $TMP_FILE
echo                                         >> $TMP_FILE
echo '#endif'                                >> $TMP_FILE

# We only update compileInfo.h if the file previously doesn't exist or is
# different to avoid trigger make build in unneded cases
diff $TMP_FILE src/lib/common/compileInfo.h
if [ "$?" == "0" ]; then
   rm -f $TMP_FILE
else
   mv $TMP_FILE src/lib/common/compileInfo.h
fi
