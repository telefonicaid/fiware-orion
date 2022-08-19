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

repeat.sh 1000 rcr  ngsi9.registerContextRequest.ok.valid.xml
repeat.sh 1000 dcar ngsi9.discoverContextAvailabilityRequest.ok.valid.xml
repeat.sh 1000 upcr ngsi10.updateContextRequest.append.valid.xml
repeat.sh 1000 qcr  ngsi10.queryContextRequest.ok.valid.xml
repeat.sh 1000 scr  ngsi10.subscribeContextRequest.onchange.valid.xml
repeat.sh 1000 scr  ngsi10.subscribeContextRequest.ontimeinterval.valid.xml
repeat.sh 1000 ucsr ngsi10.updateContextSubscriptionRequest.onchange-SUBSCRIPTIONID.invalid.xml
repeat.sh 1000 uncr ngsi9.unsubscribeContextRequest.SUBSCRIPTION_ID.invalid.xml
repeat.sh 1000 upcr ngsi10.updateContextRequest.append.valid.xml
repeat.sh 1000 upcr ngsi10.updateContextRequest.update.valid.xml
repeat.sh 1000 upcr ngsi10.updateContextRequest.delete.valid.xml
repeat.sh 1000 ncr  ngsi10.notifyContextRequest.ok.valid.xml

#repeat.sh 100 conv -cov ce XXX
#repeat.sh 100 conv -cov cea XXX
#repeat.sh 100 conv -cov ceaa XXX
