#!/usr/bin/env python3
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

import requests
import sys

requests.packages.urllib3.disable_warnings()

__author__ = 'Md Arqum Farooqui'

##############################################################
# BEGIN of the configuration part (don't touch above this line ;)

cb_endpoint = 'http://localhost:1026/v2/subscriptions'

headers = {
    'Accept': 'application/json',
    'fiware-service': 'service',
    'fiware-servicepath': '/subservice'
}

page_size = 500

# WARNING! use this filter or you will remove *all* subscriptions
# FIXME: Note that filtering is not supported in the GET /v2/subscriptions. If, at some point it gets
# implemented (https://github.com/telefonicaid/fiware-orion/issues/1145 or additional issues),
# we should adjust this point of the script with a valid example filter
#filter = '&type=device'
# For now we can keep filter variable empty.
filter = ''

# END of the configuration part (don't touch below this line ;)
##############################################################

def delete_subscriptions():
    """
    delete subscriptions in current page.
    :return: True if removal was ok, False otherwise
    """
    try:
        # Fetching subscriptions with pagination
        response = requests.get(f"{cb_endpoint}?limit={page_size}{filter}", headers=headers, verify=False)

        if response.status_code == 200:
            subscriptions = response.json()

            # Check if subscriptions list is not empty
            if not subscriptions:
                print("No more subscriptions found.")
                return False
                
            # Delete each subscription
            for subscription in subscriptions:
                subscription_id = subscription['id']
                delete_url = f"{cb_endpoint}/{subscription_id}"
                delete_response = requests.delete(delete_url, headers=headers)

                if delete_response.status_code != 204:
                    print(f"Failed to delete subscription with ID {subscription_id}. Status code: {delete_response.status_code}")
                    return False

        else:
            print(f"Failed to fetch subscriptions. Status code: {response.status_code}")
            return False

    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")


def get_subscriptions_count():
    """
    Return the total number of subscriptions.

    :return: number of subscriptions (or -1 if there was some error)
    """

    res = requests.get(cb_endpoint + '?limit=1&options=count', headers=headers, verify=False)
    if res.status_code != 200:
        print('Error getting subscriptions count (%d): %s' % (res.status_code, res.json()))
        return -1
    else:
        return int(res.headers['fiware-total-count'])


def initial_statistics():
    """
    Print some initial data statistics.
    """
    total = get_subscriptions_count()
    print(f'There are {total} subscriptions in total.')
    pages = total // page_size
    rest = total % page_size
    print(f'This includes {pages} full pages of {page_size} subscriptions and one final page of {rest} subscriptions.')


def remove_all_subscriptions():
    """
    Remove all subscriptions, page after page.
    """
    i = 1
    while get_subscriptions_count() > 0:
        print(f'- Removing page {i}')
        delete_subscriptions()
        i += 1


### Main program starts here ###

# Warn user
print(f"WARNING!!!! This script will delete all the subscriptions matching the filter '{filter}'")
print("This action cannot be undone. If you are sure you want to continue type 'yes' and press Enter")

confirm = input()

if (confirm != 'yes'):
    sys.exit()


# Get statistics
initial_statistics()

# Remove all subscriptions
remove_all_subscriptions()

print('All subscriptions deleted!')
