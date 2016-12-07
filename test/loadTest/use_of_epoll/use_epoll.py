# -*- coding: utf-8 -*-
"""
 Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U

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
import random
import time
import datetime
import pymongo


class Epoll:
    """
    Verify the epoll() instead of select(). See https://jirapdi.tid.es/browse/DM-2375 issue
    Tests procedure:
    - drop the db in mongo
    - create 5000 subscription with subject.entities.idPattern:. *
    - verify/modify the notification listener with a delay of 10 minutes before answering.
    - modify the ContextBroker config with: -httpTimeout 600000 and restart it.
    - modify a entity each 3 minutes, that it triggers all subscriptions.
    - launch indefinitely a "/version" request per second and verify that its response is correct.
    """
    # variables
    max_subscription_created = 5000
    service = u'epoll'
    service_path = u'/test'
    port = u'1026'
    cb_endpoint = u'http://localhost:%s' % port
    notif_url = 'http://localhost:9999/notify'
    mongo_host = "localhost"
    mongo_port = u'27017'
    verbose = False
    duration = 60       # time in minutes
    update_delay = 180  # time in seconds
    version_delay = 1   # time in seconds

    @staticmethod
    def __usage():
        """
        usage message
        """
        print " *****************************************************************************************************************"
        print " * This script verify the epoll() instead of select():                                                           *"
        print " *                                                                                                               *"
        print " *  Parameters:                                                                                                  *"
        print " *     -host=<host>         : CB host (OPTIONAL) (default: localhost).                                           *"
        print " *     -u                   : show this usage (OPTIONAL).                                                        *"
        print " *     -v                   : verbose with all responses (OPTIONAL) (default: False).                            *"
        print " *     -service=<value>     : service header (OPTIONAL) (default: epoll).                                        *"
        print " *     -service_path=<value>: service path header (OPTIONAL) (default: /test)                                    *"
        print " *     -notif_url=<value>   : url used to notifications (OPTIONAL) (default: http://localhost:9999/notify)       *"
        print " *     -mongo=<value>       : mongo host used to clean de bd (OPTIONAL) (default: localhost)                     *"
        print " *     -duration=<value>    : test duration, value is in minutes (OPTIONAL) (default: 60 minutes)                *"
        print " *                                                                                                               *"
        print " *  Examples:                                                                                                    *"
        print " *    python use_epoll.py -host=10.10.10.10 -notif_url=http://10.0.0.1:1234/notify duration=100 -v               *"
        print " *                                                                                                               *"
        print " *  Note:                                                                                                        *"
        print " *    - the update delay is 3 minutes                                                                            *"
        print " *    - the version delay is a second                                                                            *"
        print " *                                                                                                               *"
        print " *****************************************************************************************************************"
        exit(0)

    @staticmethod
    def __convert_timestamp_to_zulu(value):
        """
        convert a timestamp to a zulu date
        :param value: timestamp to convert
        :return: string (zulu date)
        """
        return str(datetime.datetime.fromtimestamp(value).strftime("%Y-%m-%dT%H:%M:%S.%fZ"))

    def __print_by_console(self, resp):
        """
        print by console http response
        resp: http response
        """
        if self.verbose:
            print("HTTP Code: %s" % str(resp.status_code))
            print("Headers:\n %s" % str(resp.headers))
            print("Body:\n %s" % resp.text)

    def __init__(self, arguments):
        """
        Contructor
        """
        for i in range(len(arguments)):
            if arguments[i].find('-u') >= 0:
                self.__usage()
            if arguments[i].find('-v') >= 0:
                self.verbose = True
            if arguments[i].find('-host') >= 0:
                self.cb_endpoint = "http://%s:%s" % (str(arguments[i]).split("=")[1], self.port)
            if arguments[i].find('-service') >= 0:
                self.service = str(arguments[i]).split("=")[1]
            if arguments[i].find('-service_path') >= 0:
                self.service_path = str(arguments[i]).split("=")[1]
            if arguments[i].find('-notif_url') >= 0:
                self.notif_url = str(arguments[i]).split("=")[1]
            if arguments[i].find('-mongo') >= 0:
                self.mongo_host = str(arguments[i]).split("=")[1]
            if arguments[i].find('-duration') >= 0:
                self.duration = float(arguments[i].split("=")[1])

        self.headers = {u'Accept': u'application/json',
                        u'Fiware-Service': self.service,
                        u'Fiware-ServicePath': self.service_path}

        print("Test configuration:")
        print(" INFO  - service: %s" % self.service)
        print(" INFO  - servicePath: %s" % self.service_path)
        print(" INFO  - CB endpoint: %s" % self.cb_endpoint)
        print(" INFO  - notification URL: %s" % self.notif_url)
        print(" INFO  - mongo host: %s" % self.mongo_host)
        print(" INFO  - test duration: %s minutes (%s seconds)" % (str(self.duration), str(self.duration * 60)))
        print(" INFO  - updates requests delay: %d seconds" % self.update_delay)
        print(" INFO  - version requests delay: %d seconds" % self.version_delay)
        print(" INFO  - max subcription: %d" % self.max_subscription_created)
        print(" ***************************************************************************************")
        print(" * WARN  - verify if the listener has a delay in the response (10 minutes recommended) *")
        print(" * WARN  - verify if -httpTimeout 600000 parameter is used in CB config                *")
        print(" ***************************************************************************************")

    def drop_database(self):
        """
        drop database in mongo
        """
        mongo_database = "orion-%s" % self.service
        mongo_uri = "mongodb://%s:%s/%s" % (self.mongo_host, self.mongo_port, mongo_database)
        try:
            client = pymongo.MongoClient(mongo_uri)
            client.drop_database(mongo_database)
            print(" INFO - The database %s has been erased" % mongo_database)
        except Exception, e:
            raise Exception(" ERROR - Deleting a database %s in MongoDB...\n %s" % (mongo_database, str(e)))

    def create_subscriptions(self):
        """
        create N subscription until de max subscription created
        """
        payload = u'{"description": "subscription used to epoll test",  "subject": {"entities": [{"idPattern": ".*"}], "condition": {"attrs": ["temperature"]}}, "notification": {"http": {"url": "%s"}, "attrs": ["temperature"]}}' % self.notif_url
        self.headers["Content-Type"] = u'application/json'
        for i in range(self.max_subscription_created):
            resp = requests.post("%s/v2/subscriptions" % self.cb_endpoint, headers=self.headers, data=payload)
            self.__print_by_console(resp)
            assert resp.status_code == 201, " ERROR - the subcriptions request is failed: \n - status code: %s \n - response: %s" % (str(resp.status_code), resp.text)
        print(" INFO - %d subscriptions have been created" % self.max_subscription_created)

    def update_and_version(self):
        """
        modify a entity each 3 minutes, that it triggers all subscriptions and
        launch indefinitely a "/version" request and verify that its response is correct
        """
        init_date = time.time()
        duration_in_secs = self.duration * 60
        print("Test init: %s " % self.__convert_timestamp_to_zulu(init_date))
        update_time = time.time()
        while (init_date+duration_in_secs) > time.time():
            if update_time+self.update_delay < time.time():
                # update request
                self.headers["Content-Type"] = u'application/json'
                payload = u'{"actionType": "APPEND", "entities": [{"id": "Bcn-Welt", "temperature": {"value": %s}}]}'
                random_value = random.randint(1, 100)  # random value between 1..100 for attribute value. See the payload above.
                print("payload: %s" % (payload % random_value))
                resp = requests.post("%s/v2/op/update" % self.cb_endpoint, headers=self.headers, data=payload % random_value)
                assert resp.status_code == 204, " ERROR - the update batch op request is failed: \n - status code: %s \n - response: %s" % (str(resp.status_code), resp.text)
                self.__print_by_console(resp)
                update_time = time.time()
            # version request
            resp = requests.get("%s/version" % self.cb_endpoint)
            self.__print_by_console(resp)
            assert resp.status_code == 200, " ERROR - the version http code is not 200 - OK\n     - http code received: %s - %s" % (str(resp.status_code), resp. reason)
            time.sleep(self.version_delay)
        print(" INFO - ALL \"/version\" requests responded correctly...Bye.")
        print("Test end: %s" % self.__convert_timestamp_to_zulu(time.time()))


if __name__ == '__main__':
    epoll = Epoll(sys.argv)

    # drop database in mongo
    epoll.drop_database()

    # create 5000 subscription with subject.entities.idPattern: .*
    epoll.create_subscriptions()

    # modify a entity each N minutes, that it triggers all subscriptions and
    # launch indefinitely a "/version" request and verify that its response is correct.
    epoll.update_and_version()
