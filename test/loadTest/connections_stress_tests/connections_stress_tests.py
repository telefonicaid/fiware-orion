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
import rpyc
import json
import logging

logging.basicConfig(level=logging.DEBUG,
                    format='%(asctime)s | %(levelname)s | %(message)s',
                    filename='connections_stress_tests.log',
                    filemode='w')
# disable log messages from the Requests library
logging.getLogger("requests").setLevel(logging.WARNING)


class Stablished_Connections:
    """
    Verify that ContextBroker works properly with a large number of stablished connections
    Tests procedure:
    - drop the db in mongo
    - create 5000 subscription with subject.entities.idPattern: .*
    - verify/modify the notification listener with a delay of 10 minutes before answering.
    - modify the ContextBroker config with: -httpTimeout 600000 -notificationMode threadpool:60000:5000 and restart it.
    - launch an entity update, that it triggers all subscriptions.
    - launch indefinitely a "/version" request per second and:
         - report that its response is correct.
         - report the number of established connections (if `-noEstablished` param is not used this column is ignored)
         - report the notification queue size into ContextBroker (if `-noQueueSize` param is used this column is ignored)
    """
    # variables
    max_subscription_created = 5000
    service = u'stablished_connections'
    service_path = u'/test'
    host = u'localhost'
    port = u'1026'
    cb_endpoint = u'http://%s:%s' % (host, port)
    notif_url = 'http://localhost:8090/notify'
    mongo_host = "localhost"
    mongo_port = u'27017'
    verbose = False
    no_established_connections_flag = False
    no_queue_size_flag = False
    duration = 60       # time in minutes
    version_delay = 1   # time in seconds

    @staticmethod
    def __usage():
        """
        usage message
        """
        print " *****************************************************************************************************************"
        print " * This script verifies that CB works properly with a large number of stablished connections:                    *"
        print " *                                                                                                               *"
        print " *  Parameters:                                                                                                  *"
        print " *     -host=<host>         : CB host (OPTIONAL) (default: localhost).                                           *"
        print " *     -u                   : show this usage (OPTIONAL).                                                        *"
        print " *     -v                   : verbose with all responses (OPTIONAL) (default: False).                            *"
        print " *     -noEstablished       : is used to ignore the established connections (OPTIONAL) (default: False).         *"
        print " *     -noQueueSize         : is used to ignore the Notification Queue Size (OPTIONAL) (default: False).         *"
        print " *     -service=<value>     : service header (OPTIONAL) (default: stablished_connections).                       *"
        print " *     -service_path=<value>: service path header (OPTIONAL) (default: /test)                                    *"
        print " *     -notif_url=<value>   : url used to notifications (OPTIONAL) (default: http://localhost:8090/notify)       *"
        print " *     -mongo=<value>       : mongo host used to clean de bd (OPTIONAL) (default: localhost)                     *"
        print " *     -duration=<value>    : test duration, value is in minutes (OPTIONAL) (default: 60 minutes)                *"
        print " *                                                                                                               *"
        print " *  Examples:                                                                                                    *"
        print " *   python connections_stress_tests.py -host=10.10.10.10 -notif_url=http://10.0.0.1:8090/notify duration=100 -v *"
        print " *                                                                                                               *"
        print " *  Note:                                                                                                        *"
        print " *    - the version delay is a second                                                                            *"
        print " *    - the number of subscriptions is 5000                                                                      *"
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
            logging.debug("HTTP Code: %s" % str(resp.status_code))
            logging.debug("Headers:\n %s" % str(resp.headers))
            logging.debug("Body:\n %s" % resp.text)

    def __get_queue_size(self):
        """
        return the notification queue size
        :return: string (size)
        """
        resp = requests.get("%s/statistics" % self.cb_endpoint)
        self.__print_by_console(resp)
        assert resp.status_code == 200, " ERROR - the statistics request is failed: \n - status code: %s \n - response: %s" % (str(resp.status_code), resp.text)
        resp_dict = json.loads(resp.text)
        return str(resp_dict["notifQueue"]["size"])

    def __get_established_connections(self):
        """
        return the number of established connections per ContextBroker
        using "prpyc" and "psutil" libraries.
        :return string (connections)
        """
        connections = """def get_number_established_conn():
               import psutil
               process_name = "contextBroker"
               pid = 0
               e_c = 0
               for proc in psutil.process_iter():
                      if proc.name == process_name:
                             pid = proc.pid
                             break
               p = psutil.Process(pid)
               connections = p.get_connections()
               for c in connections:
                      if c.status == "ESTABLISHED":
                             e_c += 1
               return e_c"""

        conn = rpyc.classic.connect(self.host)
        conn.execute(connections)
        remote_exec = conn.namespace['get_number_established_conn']
        return str(remote_exec())

    def __init__(self, arguments):
        """
        Contructor
        """
        for i in range(len(arguments)):
            if arguments[i].find('-u') >= 0:
                self.__usage()
            if arguments[i].find('-v') >= 0:
                self.verbose = True
            if arguments[i].find('-noQueueSize') >= 0:
                self.no_queue_size_flag = True
            if arguments[i].find('-noEstablished') >= 0:
                self.no_established_connections_flag = True
            if arguments[i].find('-host') >= 0:
                self.host = str(arguments[i]).split("=")[1]
                self.cb_endpoint = "http://%s:%s" % (self.host, self.port)
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
                        u'Content-Type': u'application/json',
                        u'Fiware-Service': self.service,
                        u'Fiware-ServicePath': self.service_path}

        logging.info("Test configuration:")
        logging.info("   service: %s" % self.service)
        logging.info("   servicePath: %s" % self.service_path)
        logging.info("   CB endpoint: %s" % self.cb_endpoint)
        logging.info("   notification URL: %s" % self.notif_url)
        logging.info("   mongo host: %s" % self.mongo_host)
        logging.info("   test duration: %s minutes (%s seconds)" % (str(self.duration), str(self.duration * 60)))
        logging.info("   version requests delay: %d seconds" % self.version_delay)
        logging.info("   max subcription: %d" % self.max_subscription_created)
        logging.info("   noEstablished flag: %s" % str(self.no_established_connections_flag))
        logging.info("   noQueueSize flag: %s" % str(self.no_queue_size_flag))
        logging.warn(" ***************************************************************************************")
        logging.warn(" *  verify if the listener has a delay in the response (10 minutes recommended)        *")
        logging.warn(" *  verify if these parameters are used in CB config:                                  *")
        logging.warn(" *           -httpTimeout 600000 -notificationMode threadpool:60000:5000               *")
        logging.warn(" ***************************************************************************************")

    def drop_database(self):
        """
        drop database in mongo
        """
        mongo_database = "orion-%s" % self.service
        mongo_uri = "mongodb://%s:%s/%s" % (self.mongo_host, self.mongo_port, mongo_database)
        try:
            client = pymongo.MongoClient(mongo_uri)
            client.drop_database(mongo_database)
            logging.info(" The database %s has been erased" % mongo_database)
        except Exception, e:
            raise Exception(" ERROR - Deleting a database %s in MongoDB...\n %s" % (mongo_database, str(e)))

    def create_subscriptions(self):
        """
        create N subscription until de max subscription created
        """
        payload = u'{"description": "subscription used to epoll test",  "subject": {"entities": [{"idPattern": ".*"}], "condition": {"attrs": ["temperature"]}}, "notification": {"http": {"url": "%s"}, "attrs": ["temperature"]}}' % self.notif_url
        logging.info(" creating %d subscriptions..." % self.max_subscription_created)
        for i in range(self.max_subscription_created):
            resp = requests.post("%s/v2/subscriptions" % self.cb_endpoint, headers=self.headers, data=payload)
            self.__print_by_console(resp)
            assert resp.status_code == 201, " ERROR - the subcriptions request is failed: \n - status code: %s \n - response: %s" % (str(resp.status_code), resp.text)
        logging.info(" %d subscriptions have been created" % self.max_subscription_created)

    def update_and_version(self):
        """
        launch an entity update, that it triggers all subscriptions and
        launch indefinitely a "/version" request and verify that its response is correct
        """
        init_date = time.time()
        duration_in_secs = self.duration * 60
        queue_size = u'N/A'
        established_connections = u'N/A'
        counter = 0

        # update request
        logging.info("Test init: %s " % self.__convert_timestamp_to_zulu(init_date))
        random_value = random.randint(1, 100)  # random value between 1..100 for attribute value. See the payload below.
        payload = u'{"actionType": "APPEND", "entities": [{"id": "Bcn-Welt", "temperature": {"value": %s}}]}' % random_value
        resp = requests.post("%s/v2/op/update" % self.cb_endpoint, headers=self.headers, data=payload)
        assert resp.status_code == 204, " ERROR - the update batch op request is failed: \n - status code: %s \n - response: %s" % (str(resp.status_code), resp.text)
        self.__print_by_console(resp)

        logging.info(" Reports each second:")
        logging.info(" counter       version      queue   established")
        logging.info("               request      size    connections")
        logging.info(" --------------------------------------------------------")
        while (init_date+duration_in_secs) > time.time():
            # version request
            counter += 1
            resp = requests.get("%s/version" % self.cb_endpoint)
            self.__print_by_console(resp)
            assert resp.status_code == 200, " ERROR - the version http code is not 200 - OK\n     - http code received: %s - %s" % (str(resp.status_code), resp. reason)
            # report
            if not self.no_queue_size_flag:
                queue_size = self.__get_queue_size()
            if not self.no_established_connections_flag:
                established_connections = self.__get_established_connections()
            logging.info(" -------- %d -------- %s -------- %s -------- %s ----------" % (counter, resp.reason, queue_size, established_connections))
            time.sleep(self.version_delay)

        logging.info(" ALL (%d) \"/version\" requests responded correctly...Bye." % counter)
        logging.info("Test end: %s" % self.__convert_timestamp_to_zulu(time.time()))


if __name__ == '__main__':
    conn = Stablished_Connections(sys.argv)

    # drop database in mongo
    conn.drop_database()

    # create N subscription with subject.entities.idPattern: .*
    conn.create_subscriptions()

    # modify a entity that it triggers all subscriptions and...
    # ...launch indefinitely a "/version" request and verify that its response is correct.
    conn.update_and_version()
