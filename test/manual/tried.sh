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

repeat.sh 1000 rcr firstReg.xml
repeat.sh 1000 dcar simpleDiscovery.xml
repeat.sh 1000 upcr updateContext01.append.xml
repeat.sh 1000 scar subscribeContextAvailabilityRequest.xml
repeat.sh 1000 ucas updateContextAvailabilitySubscriptionRequest.xml
repeat.sh 1000 ucar unsubscribeContextAvailabilityRequest.xml
repeat.sh 1000 ncar notifyContextAvailabilityRequest.xml
repeat.sh 1000 qcr  queryContext.xml
repeat.sh 1000 scr  subscribeContextRequest-ONCHANGE.xml
repeat.sh 1000 scr  subscribeContextRequest-ONTIMEINTERVAL.xml
repeat.sh 1000 ucsr updateContextSubscriptionRequest.xml
repeat.sh 1000 uncr unsubscribeContextRequest.xml
repeat.sh 1000 upcr updateContext01.append.xml
repeat.sh 1000 upcr updateContext01.update.xml
repeat.sh 1000 upcr updateContext01.delete.xml
repeat.sh 1000 ncr  notifyContextRequest.xml

#repeat.sh 100 conv -cov ce XXX
#repeat.sh 100 conv -cov cea XXX
#repeat.sh 100 conv -cov ceaa XXX
