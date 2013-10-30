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

date=$(LANG=C date)
hash=$(git log | grep commit | head -1 | awk '{ print $2 }')
user=$(whoami)
host=$(hostname)

echo '#ifndef CB_VERSION_H'                   >  src/lib/common/compileInfo.h
echo '#define CB_VERSION_H'                  >> src/lib/common/compileInfo.h
echo                                         >> src/lib/common/compileInfo.h
echo '#define GIT_HASH         "'${hash}'"'  >> src/lib/common/compileInfo.h
echo '#define COMPILE_TIME     "'${date}'"'  >> src/lib/common/compileInfo.h
echo '#define COMPILED_BY      "'${user}'"'  >> src/lib/common/compileInfo.h
echo '#define COMPILED_IN      "'${host}'"'  >> src/lib/common/compileInfo.h
echo                                         >> src/lib/common/compileInfo.h
echo '#endif'                                >> src/lib/common/compileInfo.h
