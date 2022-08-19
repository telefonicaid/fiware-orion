#!/usr/bin/env python
# Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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

import requests
from time import sleep
from random import randint
from datetime import datetime
requests.packages.urllib3.disable_warnings()

__author__ = 'fermin'

# This script updates periodically a given attribute of a given entity.
# It is though to be used in combination with STH in demos, but of
# course it may be used in any other context

##############################################################
# BEGIN of the configuration part (don't touch above this line ;)

cb_endpoint = 'http://localhost:1026'

headers = {
    'fiware-service': 'service',
    'fiware-servicepath': '/subservice',
    'x-auth-token': 'token'
}

# entity and attribute to update
entity = 'thermometer'
attribute = 'temperature'

# attribute range (must be integers)
from_value = 30
to_value = 50

# update interval in seconds
interval = 60*5

# END of the configuration part (don't touch below this line ;)
##############################################################

def update_entity():
    """
    Update entity based on parametrization.
    :return: True if update went ok, False otherwise
    """

    value = randint(from_value, to_value)

    body = {
        'type': 'Number',
        'value': value
    }

    print '%s: Updating attribute %s in entity %s with value %d' % (datetime.now().isoformat(), entity, attribute, value)

    res = requests.put(cb_endpoint + '/v2/entities/' + entity + '/attrs/' + attribute, json=body, headers=headers, verify=False)
    if res.status_code != 204:
        print '*** Error updating entity (%d): %s' % (res.status_code, res.json())
        return False

    return True


### Main program starts here ###

while True:
    if not update_entity():
       break;
    sleep(interval)
