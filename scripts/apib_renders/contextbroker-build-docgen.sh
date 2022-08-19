# Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
# Mostly a verbatim copy of ContextBroker-Build-Docgen job at jenkins
# It would be great to do an overall cleanup, but not a priority at the present moment ;)
#
# NOTES:
# - It is assumed that required commands (e.g. rvm, apiary, etc.) are already installed in the system where this scripts runs; otherwise it is going to fail
# - It is assumed that required env vars (e.g. WORKSPACE) are already set; otherwise it is going to fail


# Preparing specifications repo clone
cd /tmp
rm -rf specifications  # to avoid problems if this job previously failed without reaching end of script
git clone git@github.com:Fiware/specifications.git
cd specifications
git checkout gh-pages

# The rest of the process is done from WORKSPACE. No place like home... :)
cd ${WORKSPACE}

# Set Ruby 1.9 interpreter
source /etc/profile.d/rvm.sh
rvm use 1.9.3

# Generate apiary renders
apiary preview --path ${WORKSPACE}/doc/apiary/orioncontextbroker.apib --output /tmp/tmp0.html
apiary preview --path ${WORKSPACE}/doc/apiary/v2/fiware-ngsiv2-reference.apib --output /tmp/tmp1.html
apiary preview --path ${WORKSPACE}/doc/apiary/v2/fiware-ngsiv2-cookbook.apib --output /tmp/tmp2.html

# Change now to gh-page branch in fiware-orion repo
git checkout gh-pages

# Generate doc (probably this could be more efficiently, more DRY)
COUNT=$(diff /tmp/tmp0.html api/v1/index.htm | wc -l)
if [ $COUNT -ne 0 ]
then
  mv /tmp/tmp0.html api/v1/index.htm
  git add api/v1/index.htm
  git commit -m "ADD ngsi v1 apiary snapshoot: $BUILD_NUMBER" api/v1/index.htm
  git push origin gh-pages
fi

COUNT=$(diff /tmp/tmp1.html api/v2/latest/index.htm | wc -l)
if [ $COUNT -ne 0 ]
then
  mv /tmp/tmp1.html api/v2/latest/index.htm
  git add api/v2/latest/index.htm
  git commit -m "ADD ngsi v2 apiary reference snapshoot: $BUILD_NUMBER" api/v2/latest/index.htm
  git push origin gh-pages
  # Sync in spec file
  cp api/v2/latest/index.htm /tmp/specifications/ngsiv2/latest/index.htm
  cd /tmp/specifications && git commit -m "ADD ngsi v2 apiary reference snapshoot: $BUILD_NUMBER" ngsiv2/latest/index.htm
  cd /tmp/specifications && git push origin gh-pages
fi

COUNT=$(diff /tmp/tmp2.html api/v2/latest/cookbook/index.htm | wc -l)
if [ $COUNT -ne 0 ]
then
  mv /tmp/tmp2.html api/v2/latest/cookbook/index.htm
  git add api/v2/latest/cookbook/index.htm
  git commit -m "ADD ngsi v2 apiary cookbook snapshoot: $BUILD_NUMBER" api/v2/latest/cookbook/index.htm
  git push origin gh-pages
  cp api/v2/latest/cookbook/index.htm /tmp/specifications/ngsiv2/latest/cookbook/index.htm
  cd /tmp/specifications && git commit -m "ADD ngsi v2 apiary reference snapshoot: $BUILD_NUMBER" ngsiv2/latest/cookbook/index.htm
  cd /tmp/specifications && git push origin gh-pages
fi

# Remove specifications repo clone
rm -rf /tmp/specifications
