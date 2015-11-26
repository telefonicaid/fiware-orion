# -*- coding: utf-8 -*-
"""
 Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U

 This file is part of Orion Context Broker.

 Orion Context Broker is free software: you can redistribute it and/or
 modify it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 Orion Context Broker is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.

 For those usages not covered by this license please contact with
 iot_support at tid dot es
"""
__author__ = 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

import sys
import requests


class Subscriptions:
    """
    Subscriptions manager (create, update and delete)
    """
    # variables
    operation = u'create'
    duration = u'1000S'
    throttling = u''
    subId = u'NotDefined'
    service = u'entities_append_long_time'
    service_path = u'/test'
    refer_url = 'http://localhost:9999/notify'

    @staticmethod
    def __usage():
        """
        usage message
        """
        print " *****************************************************************************************************************"
        print " * This script manage subscriptions with these operation:                                                        *"
        print " *    - create, update and delete.                                                                               *"
        print " *  usage: python subscription_manager.py <host> [optional params] [-u]                                          *"
        print " *                                                                                                               *"
        print " *  Parameters:                                                                                                  *"
        print " *     <host>               : CB or balancer host (MANDATORY).                                                   *"
        print " *     -u                   : show this usage (Optional).                                                        *"
        print " *     -operation=<value>   : operations allowed: create | update | delete (default: create) (Optional).         *"
        print " *     -duration=<value>    : used only in create and update operations (default: 1000S) (Optional)              *"
        print " *     -throttling=<value>  : used only in create and update operations (default: it is not used) (Optional)     *"
        print " *     -subs_id=<value>     : used only in delete and update operation (default: NotDefined) (Optional)          *"
        print " *     -service=<value>     : service header (default: entities_append_long_time) (Optional)                     *"
        print " *     -service_path=<value>: service path header (default: /test) (Optional)                                    *"
        print " *     -refer_url=<value>   : reference url used to notifications in create operation (Optional)                 *"
        print " *                                                                                                               *"
        print " *  Examples:                                                                                                    *"
        print " *    python subscription_manager.py localhost -operation=create -duration=1000S -throttling=20S                 *"
        print " *    python subscription_manager.py localhost -operation=update -duration=1000S -subs_id=51c04a21d714fb3b37d7d5 *"
        print " *    python subscription_manager.py localhost -operation=delete -subs_id=51c04a21d714fb3b37d7d5a7               *"
        print " *                                                                                                               *"
        print " *****************************************************************************************************************"
        exit(0)

    def __init__(self, arguments):
        """
        Contructor
        """
        if len(arguments) <= 1:
            print "ERROR - No host defined (Mandatory)"
            self.__usage()
            sys.exit(0)
        self.host = arguments[1]
        for i in range(len(arguments)):
            if arguments[i].find('-u') >= 0:
                self.__usage()
            if arguments[i].find('-operation') >= 0:
                self.operation = str(arguments[i]).split("=")[1]
            if arguments[i].find('-duration') >= 0:
                self.duration = str(arguments[i]).split("=")[1]
            if arguments[i].find('-throttling') >= 0:
                self.throttling = u', "throttling": "PT%s"' % str(arguments[i]).split("=")[1]
            if arguments[i].find('-subs_id') >= 0:
                self.subId = str(arguments[i]).split("=")[1]
            if arguments[i].find('-service') >= 0:
                self.service = str(arguments[i]).split("=")[1]
            if arguments[i].find('-service_path') >= 0:
                self.service_path = str(arguments[i]).split("=")[1]
            if arguments[i].find('-refer_url') >= 0:
                self.refer_url = str(arguments[i]).split("=")[1]

        self.headers = {u'Content-Type': u'application/json',
                        u'Accept': u'application/json',
                        u'Fiware-Service': self.service,
                        u'Fiware-ServicePath': self.service_path}
        if self.operation.lower() == "create":
            self.__create()
        elif self.operation.lower() == "delete":
            self.__delete()
        elif self.operation.lower() == "update":
            self.__update()
        else:
            print "ERROR - operation unknown: %s\n - operations allowed: create, delete or update" % self.operation

    @staticmethod
    def __print_by_console(resp):
        """
        print by console http response
        resp: http response
        """
        print "HTTP Code: %s" % str(resp.status_code)
        print "Body:\n %s" % resp.text

    def __create(self):
        """
        create a subscription
        """
        payload = u'{                                                          '\
                   '   "entities": [                                           '\
                   '    {                                                      '\
                   '      "type": "house",                                     '\
                   '      "isPattern": "true",                                 '\
                   '      "id": "room_*"                                       '\
                   '    }                                                      '\
                   '   ],                                                      '\
                   '   "attributes": [                                         '\
                   '     "temperature"                                         '\
                   '   ],                                                      '\
                   '    "reference": "%s",                                     '\
                   '    "duration": "PT%s",                                    '\
                   '    "notifyConditions": [                                  '\
                   '      {                                                    '\
                   '        "type": "ONCHANGE",                                '\
                   '        "condValues": [                                    '\
                   '           "temperature"                                   '\
                   '        ]                                                  '\
                   '      }                                                    '\
                   '   ]                                                       '\
                   '   %s                                                      '\
                   ' }                                                         ' % \
                  (self.refer_url, self.duration, self.throttling)
        resp = requests.post("http://%s:1026/NGSI10/subscribeContext" % self.host, headers=self.headers, data=payload)
        self.__print_by_console(resp)

    def __delete(self):
        """
        delete a subscription
        """

        payload = u'{                           '\
                  u'    "subscriptionId": "%s"  '\
                  u'}                           ' % self.subId
        if self.subId != "NotDefined":
            resp = requests.post("http://%s:1026/v1/unsubscribeContext" % self.host, headers=self.headers, data=payload)
            self.__print_by_console(resp)
        else:
            print "ERROR - the subscriptionId is neccesary.\n   Use -subs_id=????"

    def __update(self):
        """
        update a subscription
        """
        payload = u'{                           '\
                  u'    "subscriptionId": "%s", '\
                  u'    "duration": "PT%s"      '\
                  u'    %s                      '\
                  u'}                           ' % (self.subId, self.duration, self.throttling)
        if self.subId != "NotDefined":
            resp = requests.post("http://%s:1026/v1/updateContextSubscription" % self.host, headers=self.headers, data=payload)
            self.__print_by_console(resp)
        else:
            print "ERROR - the subscriptionId is neccesary.\n   Use -subs_id=????"


if __name__ == '__main__':
    sub = Subscriptions(sys.argv)
