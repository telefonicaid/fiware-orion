# Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
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

CJEXL_VERSION=$1
TOKEN=$2

res_code=$(curl -I -s -o /dev/null -w "%{http_code}" -H "Authorization: token $TOKEN" https://api.github.com/repos/telefonicasc/cjexl/releases/tags/$CJEXL_VERSION)
if [ "$res_code" -eq 200 ]; then
    echo "get_cjexl: downloading cjexl lib $CJEXL_VERSION"
    ASSET_ID=$(curl -s -S -H "Authorization: token $TOKEN" https://api.github.com/repos/telefonicasc/cjexl/releases/tags/$CJEXL_VERSION | grep -C3 libcjexl.a | grep '"id"' | awk -F ' ' '{print $2}' | awk -F ',' '{print $1}')
    curl -L -s -o /usr/local/lib/libcjexl.a -H "Authorization: token $TOKEN" -H "Accept: application/octet-stream" https://api.github.com/repos/telefonicasc/cjexl/releases/assets/$ASSET_ID
    MD5SUM=$(md5sum /usr/local/lib/libcjexl.a | awk -F ' ' '{print$1}')
    echo "get_cjexl: cjexl lib md5sum is $MD5SUM"
else
    echo "get_cjexl: error $res_code accessing cjexl release. Maybe token is invalid?"
fi