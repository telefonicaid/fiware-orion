#!/usr/bin/env python3
# -*- coding: latin-1 -*-
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

# MQTT functionality based in old accumulator-mqtt.py script, by Burak

# Requires following dependencies:
#
# Flask==2.0.2
# Werkzeug==2.0.2
# paho-mqtt==1.6.1
# amqtt==0.11.0b1
# setuptools==80.9.0
# confluent-kafka==2.11.0
#

__author__ = 'fermin'

# This program stores everything it receives by HTTP in a given URL (pased as argument),
# Then return the accumulated data upon receiving 'GET <host>:<port>/dump'. It is aimet
# at harness test for subscription scenarios (so accumulator-server.py plays the role
# of a subscribed application)
#
# Known issues:
#
# * Curl users: use -H "Expect:" (default "Expect: 100-continue" used by curl has been problematic
#   in the past)
# * Curl users: use -H "Content-Type: application/xml"  for XML payload (the default:
#   "Content-Type: application/x-www-form-urlencoded" has been problematic in the pass)
# * This script requires at least Flask 2.0.2, which comes with Werkzeug 2.0.2.

from flask import Flask, request, Response
from getopt import getopt, GetoptError
from datetime import datetime
from math import trunc
from time import sleep
import sys
import os
import atexit
import string
import signal
import json
import re
import paho.mqtt.client as mqtt
import threading
from confluent_kafka import Consumer, KafkaException, KafkaError
from confluent_kafka.admin import AdminClient, NewTopic


def usage_and_exit(msg):
    """
    Print usage message and exit"

    :param msg: optional error message to print
    """

    if msg != '':
        print(msg + "\n")

    usage()
    sys.exit(1)


def usage():
    """
    Print usage message
    """

    print(f"Usage: {os.path.basename(__file__)} --host <host> --port <port> " +
          "--mqttHost <host> --mqttPort <port> --mqttTopic <topic> " +
          "--bootstrapServers <endpoint> --kafkaTopic <topic> " +
          "--kafkaGroupId <group_id> --url <server url> --pretty-print "
          "--https --key <key_file> --cert <cert_file> -v -u")
    print("""\nParameters:
          --host <host>: host to use database to use (default is '0.0.0.0')"
          --port <port>: port to use (default is 1028)"
          --mqttHost <host>: mqtt broker host to use (if not defined MQTT listening is not enabled)"
          --mqttPort <port>: mqtt broker port to use (default is 1883)"
          --mqttTopic <topic>: mqtt topic to use (default is accumulate_mqtt)"
          --url <server url>: server URL to use (default is /accumulate)"
          --bootstrapServers <endpoint>: Kafka url example: brokerA:9092,brokerB:9094
          --kafkaTopic <topic>: Kafka topic (default 'accumulate_kafka')
          --kafkaGroupId <group_id>: Kafka consumer group ID (default 'accumulator_group')
          --kafka_security_protocol
          --kafka_sasl_mechanism
          --kafka_sasl_user
          --kafka_sasl_passwd
          --pretty-print: pretty print mode"
          --https: start in https"
          --key <key file>: (only used if https is enabled)"
          --cert <cert file>: (only used if https is enabled)"
          -v: verbose mode"
          -u: print this usage message
          """
          )


# This function is registered to be called upon termination
def all_done():
    os.unlink(pidfile)


# Default arguments
port = 1028
host = '0.0.0.0'
mqtt_host = None
mqtt_port = 1883
mqtt_topic = 'accumulate_mqtt'
bootstrap_servers = None
kafka_topic = 'accumulate_kafka'
kafka_group_id = 'accumulator_group'
kafka_security_protocol = None
kafka_sasl_mechanism = None
kafka_sasl_user = None
kafka_sasl_passwd = None
server_url = '/accumulate'
verbose = 0
pretty = False
https = False
key_file = None
cert_file = None

# TODO: Improve cli argument parsing with click
# https://click.palletsprojects.com/en/8.0.x/
# This will avoid the 'complex' for statement at L137
try:
    opts, args = getopt(
        sys.argv[1:],
        'vu', [
            'host=', 'port=',
            'mqttHost=',
            'mqttPort=',
            'mqttTopic=',
            'bootstrapServers=',
            'kafkaTopic=',
            'kafkaSecurityProtocol=',
            'kafkaSaslMechanism=',
            'kafkaSaslUser=',
            'kafkaSaslPasswd=',
            'kafkaGroupId=',
            'url=',
            'pretty-print',
            'https',
            'key=',
            'cert='
        ]
    )
except GetoptError:
    usage_and_exit('wrong parameter')

for opt, arg in opts:
    if opt == '-u':
        usage()
        sys.exit(0)
    elif opt == '--mqttHost':
        mqtt_host = arg
    elif opt == '--bootstrapServers':
        bootstrap_servers = arg
    elif opt == '--host':
        host = arg
    elif opt == '--url':
        server_url = arg
    elif opt == '--mqttTopic':
        mqtt_topic = arg
    elif opt == '--kafkaTopic':
        kafka_topic = arg
    elif opt == '--kafkaGroupId':
        kafka_group_id = arg
    elif opt == '--kafkaSecurityProtocol':
        kafka_security_protocol = arg
    elif opt == '--kafkaSaslMechanism':
        kafka_sasl_mechanism = arg
    elif opt == '--kafkaSaslUser':
        kafka_sasl_user = arg
    elif opt == '--kafkaSaslPasswd':
        kafka_sasl_passwd = arg
    elif opt == '--port':
        try:
            port = int(arg)
        except ValueError:
            usage_and_exit('port parameter must be an integer')
    elif opt == '--mqttPort':
        try:
            mqtt_port = int(arg)
        except ValueError:
            usage_and_exit('mqttPort parameter must be an integer')
    elif opt == '-v':
        verbose = 1
    elif opt == '--pretty-print':
        pretty = True
    elif opt == '--https':
        https = True
    elif opt == '--key':
        key_file = arg
    elif opt == '--cert':
        cert_file = arg
    else:
        usage_and_exit()

if https:
    if key_file is None or cert_file is None:
        print("if --https is used then you have to provide --key and --cert")
        sys.exit(1)

if verbose:
    print("verbose mode is on")
    print("port: " + str(port))
    print("host: " + str(host))
    if mqtt_host:
        print("mqtt_port: " + str(mqtt_port))
        print("mqtt_host: " + str(mqtt_host))
        print("mqtt_topic: " + str(mqtt_topic))
    if bootstrap_servers:
        print("bootstrap_servers: " + str(bootstrap_servers))
        print("kafka_topic: " + str(kafka_topic))
        print(f"kafka_group_id: " + str(kafka_group_id))
    print("server_url: " + str(server_url))
    print("pretty: " + str(pretty))
    print("https: " + str(https))
    if https:
        print("key file: " + key_file)
        print("cert file: " + cert_file)

pid = str(os.getpid())
pidfile = "/tmp/accumulator." + str(port) + ".pid"

#
# If an accumulator process is already running, it is killed.
# First using SIGTERM, then SIGINT and finally SIGKILL
# The exception handling is needed as this process dies in case
# a kill is issued on a non-running process ...
#
if os.path.isfile(pidfile):
    oldpid = open(pidfile, 'r').read()
    opid = int(oldpid)
    print(f"PID file {pidfile} already exists, killing the process {oldpid}")

    try:
        oldstderr = sys.stderr
        sys.stderr = open("/dev/null", "w")
        os.kill(opid, signal.SIGTERM)
        sleep(0.1)
        os.kill(opid, signal.SIGINT)
        sleep(0.1)
        os.kill(opid, signal.SIGKILL)
        sys.stderr = oldstderr
    except Exception:
        print(f"Process {opid} killed")


#
# Creating the pidfile of the currently running process
#
open(pidfile, 'w').write(pid)

#
# Making the function all_done being executed on exit of this process.
# all_done removes the pidfile
#
atexit.register(all_done)


app = Flask(__name__)


@app.route("/noresponse", methods=['POST'])
def noresponse():
    sleep(10)
    return Response(status=200)


@app.route("/noresponse/updateContext", methods=['POST'])
def unoresponse():
    sleep(10)
    return Response(status=200)


@app.route("/noresponse/queryContext", methods=['POST'])
def qnoresponse():
    sleep(10)
    return Response(status=200)


# This response has been designed to test the #2360 case, but is general enough to be
# used in other future cases (e.e. #3363)
@app.route("/badresponse/queryContext", methods=['POST'])
def bad_response():
    r = Response(status=404)
    r.data = '{"name":"ENTITY_NOT_FOUND","message":"The entity with the requested id [qa_name_01] was not found."}'
    return r


# This response has been designed for 3068_ngsi_v2_based_forwarding/query_cpr_fail_but_local_results.test,
# but is general enough to be used in other future cases
@app.route("/badresponse/op/query", methods=['POST'])
def bad_response_device_not_found():
    r = Response(status=404)
    r.data = '{"name":"DEVICE_NOT_FOUND","message":"No device was found with id:E."}'
    return r


# From:
# https://stackoverflow.com/questions/14902299/json-loads-allows-duplicate-keys-in-a-dictionary-overwriting-the-first-value
def dict_raise_on_duplicates(ordered_pairs):
    """Reject duplicate keys."""
    d = {}
    for k, v in ordered_pairs:
        if k in d:
            raise ValueError(f"duplicate key: {(k,)}")
        else:
            d[k] = v
    return d


def sort_headers(headers):
    """
    Sort headers in a predefined order. It seems that from the Python2 version of this
    script (which used Flask==1.0.2) and the Python3 version (which uses Flask==2.0.2)
    the order of the headers has changed. We sort to avoid change a lot of .test
    expectations with no gain.

    :param headers: the headers list to sort
    """

    sorted = []
    headers_order = [
        'Fiware-Servicepath',
        'Content-Length',
        'X-Auth-Token',
        'User-Agent',
        'Ngsiv2-Attrsformat',
        'Host',
        'Accept',
        'Fiware-Service',
        'Content-Type',
        'Fiware-Correlator',
    ]

    # headers is a generator object, not exactly a list (i.e. it doesn't have remove method)
    headers_list = list(headers)

    for h in headers_order:
        if h in headers_list:
            sorted.append(h)
            headers_list.remove(h)

    # Remaining headers are added at the end of sorted array in the same order
    for h in headers_list:
        sorted.append(h)

    return sorted


def record_request(request):
    """
    Common function used by several route methods to save request content

    :param request: the request to save
    """

    global ac, t0, times
    s = ''

    # First request? Then, set reference datetime. Otherwise, add the
    # timedelta to the list
    if (t0 == ''):
        t0 = datetime.now()
        times.append(0)
    else:
        delta = datetime.now() - t0
        t = delta.total_seconds()
        times.append(trunc(round(t)))
        # times.append(t)

    # Store verb and URL
    #
    # We have found that request.url can be problematic in some distributions (e.g. Debian 8.2)
    # when used with IPv6, so we use request.scheme, request.host and request.path to compose
    # the URL "manually"
    #
    #  request.url = request.scheme + '://' + request.host + request.path
    #
    s += request.method + ' ' + request.scheme + '://' + request.host + request.path

    # Check for query params
    params = ''
    for k in request.args:
        if (params == ''):
            params = k + '=' + request.args[k]
        else:
            params += '&' + k + '=' + request.args[k]

    if (params == ''):
        s += '\n'
    else:
        s += '?' + params + '\n'

    # Store headers (according to pre-defined order)
    for h in sort_headers(request.headers.keys()):
        s += h + ': ' + request.headers[h] + '\n'

    # Store payload
    if ((request.data is not None) and (len(request.data) != 0)):
        s += '\n'
        if pretty:
            try:
                raw = json.loads(request.data, object_pairs_hook=dict_raise_on_duplicates)
                s += json.dumps(raw, indent=4, sort_keys=True)
                s += '\n'
            except ValueError as e:
                s += str(e)
        else:
            s += request.data.decode("utf-8")

    # Separator
    s += '=======================================\n'

    # Accumulate
    ac += s

    if verbose:
        print(s)


def send_continue(request):
    """
    Inspect request header in order to look if we have to continue or not

    :param request: the request to look
    :return: true if we  have to continue, false otherwise
    """

    for h in request.headers.keys():
        if ((h == 'Expect') and (request.headers[h] == '100-continue')):
            return True

    return False


@app.route("/v1/updateContext", methods=['POST'])
@app.route("/v1/queryContext", methods=['POST'])
@app.route("/v2/op/query", methods=['POST'])
@app.route("/v2/op/update", methods=['POST'])
@app.route("/v2/entities", methods=['GET'])
@app.route(server_url, methods=['GET', 'POST', 'PUT', 'DELETE', 'PATCH'])
def record():

    # Store request
    record_request(request)

    if send_continue(request):
        return Response(status=100)
    else:
        return Response(status=200)


@app.route("/bug2871/updateContext", methods=['POST'])
def record_2871():

    # Store request
    record_request(request)

    if send_continue(request):
        return Response(status=100)
    else:
        # Ad hoc response related with issue #2871, see https://github.com/telefonicaid/fiware-orion/issues/2871
        r = Response(status=200)
        r.data = '''
            {
              "contextResponses": [
                {
                  "contextElement": {
                    "attributes": [
                      {
                        "name": "turn",
                        "type": "string",
                        "value": ""
                      }
                    ],
                    "id": "entity1",
                    "isPattern": false,
                    "type": "device"
                  },
                  "statusCode": {
                    "code": 200,
                    "reasonPhrase": "OK"
                  }
                }
              ]
            }
        '''
        return r


# In NGSIv1 the response for queryContext and updateContext has the same structure, so we reuse
# this function for boths
@app.route("/cpr/queryContext", methods=['POST'])
@app.route("/cpr/updateContext", methods=['POST'])
def cpr_simulation():

    # Store request
    record_request(request)

    if send_continue(request):
        return Response(status=100)
    else:
        # Ad hoc response to test new NGSIv1 parsing logic in PR #4603
        r = Response(status=200)
        r.data = '''
            {
                "contextResponses": [
                    {
                        "contextElement": {
                            "attributes": [
                                {
                                    "name": "lightstatus",
                                    "type": "light",
                                    "value": {
                                      "x": 1,
                                      "y": 2
                                    }
                                },
                                {
                                    "name": "pressure",
                                    "type": "clima",
                                    "value": [ "a", "b", "c" ]
                                },
                                {
                                    "name": "temperature",
                                    "type": "degree",
                                    "value": "14",
                                    "metadatas": [
                                        {
                                            "name": "ID1",
                                            "type": "Text",
                                            "value": {
                                                "x": 1,
                                                "y": 2
                                            }
                                        },
                                        {
                                            "name": "ID2",
                                            "type": "Text",
                                            "value": [ "a", "b", "c" ]
                                        },
                                        {
                                            "name": "ID3",
                                            "type": "Text",
                                            "value": "ThisIsID"
                                        }
                                    ]
                                }
                            ],
                            "id": "ConferenceRoom",
                            "isPattern": "false",
                            "type": "Room"
                        },
                        "statusCode": {
                            "code": "200",
                            "reasonPhrase": "OK"
                        }
                    },
                    {
                        "contextElement": {
                            "attributes": [
                                {
                                    "name": "temperature",
                                    "type": "degree",
                                    "value": "14"
                                }
                            ],
                            "id": "ConferenceRoom2",
                            "isPattern": "false",
                            "type": "Room"
                        },
                        "statusCode": {
                            "code": "200",
                            "reasonPhrase": "OK"
                        }
                    },
                    {
                        "contextElement": {
                            "attributes": [
                                {
                                    "name": "pressure",
                                    "type": "degree",
                                    "value": "14"
                                }
                            ],
                            "id": "ConferenceRoom3",
                            "isPattern": "false",
                            "type": "Room"
                        },
                        "statusCode": {
                            "code": "200",
                            "reasonPhrase": "OK"
                        }
                    }
                ]
            }
        '''
        return r



@app.route("/cprfail/updateContext", methods=['POST'])
def cpr_simulation_fail():

    # Store request
    record_request(request)

    if send_continue(request):
        return Response(status=100)
    else:
        # Ad hoc response to test new NGSIv1 parsing logic in PR #4603
        r = Response(status=200)
        r.data = '''
            {
                "contextResponses": [
                    {
                        "contextElement": {
                            "attributes": [
                                {
                                    "name": "lightstatus",
                                    "type": "light",
                                    "value": {
                                      "x": 1,
                                      "y": 2
                                    }
                                },
                                {
                                    "name": "pressure",
                                    "type": "clima",
                                    "value": [ "a", "b", "c" ]
                                },
                                {
                                    "name": "temperature",
                                    "type": "degree",
                                    "value": "14",
                                    "metadatas": [
                                        {
                                            "name": "ID1",
                                            "type": "Text",
                                            "value": {
                                                "x": 1,
                                                "y": 2
                                            }
                                        },
                                        {
                                            "name": "ID2",
                                            "type": "Text",
                                            "value": [ "a", "b", "c" ]
                                        },
                                        {
                                            "name": "ID3",
                                            "type": "Text",
                                            "value": "ThisIsID"
                                        }
                                    ]
                                }
                            ],
                            "id": "ConferenceRoom",
                            "isPattern": "false",
                            "type": "Room"
                        },
                        "statusCode": {
                            "code": "200",
                            "reasonPhrase": "OK"
                        }
                    },
                    {
                        "contextElement": {
                            "attributes": [
                                {
                                    "name": "temperature",
                                    "type": "degree",
                                    "value": "14"
                                }
                            ],
                            "id": "ConferenceRoom2",
                            "isPattern": "false",
                            "type": "Room"
                        },
                        "statusCode": {
                            "code": "422",
                            "reasonPhrase": "this is a fail"
                        }
                    }
                ]
            }
        '''
        return r


# Next 6 ones are for testing subscription status and failure logic. They are used by test
# 1126_GET_v2_subscriptions/lastsuccesscode_and_lastfailurereason.test


@app.route("/giveme200", methods=['POST'])
def giveme200():
    return Response(status=200)


@app.route("/giveme400", methods=['POST'])
def giveme400():
    return Response(status=400)


@app.route("/giveme404", methods=['POST'])
def giveme404():
    return Response(status=404)


@app.route("/giveme500", methods=['POST'])
def giveme500():
    return Response(status=500)


@app.route("/givemeDelay", methods=['POST'])
def givemeDelay():
    sleep(60)
    return Response(status=200)


@app.route("/waitForever", methods=['POST'])
def waitForever():
    sleep(6000000000)    # Arround 20 years.. close enough to "forever" :)
    return Response(status=200)


@app.route('/dump', methods=['GET'])
def dump():
    return ac


@app.route('/times', methods=['GET'])
def giveTimes():
    return ', '.join(map(str, times)) + '\n'


@app.route('/number', methods=['GET'])
def number():
    return str(len(times)) + '\n'


@app.route('/reset', methods=['POST'])
def reset():
    global ac, t0, times
    ac = ''
    t0 = ''
    times = []
    return Response(status=200)


@app.route('/pid', methods=['GET'])
def getPid():
    return str(os.getpid())


# This is the accumulation string
ac = ''
t0 = ''
times = []
lock = threading.Lock()


# The callback for when the client receives a CONNACK response from the MQTT broker
def on_connect(client, userdata, flags, rc):
    print("MQTT broker connected with result code " + str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(mqtt_topic)


# The callback for when a PUBLISH message is received from the MQTT broker
def on_message(client, userdata, msg):

    global ac, t0, times, lock

    lock.acquire()

    s = ''

    # FIXME: this is common code in record_request(). Unify.
    # First notification? Then, set reference datetime. Otherwise, add the
    # timedelta to the list
    if (t0 == ''):
        t0 = datetime.now()
        times.append(0)
    else:
        delta = datetime.now() - t0
        t = delta.total_seconds()
        times.append(trunc(round(t)))

    s += 'MQTT message at topic ' + msg.topic + ':'
    s += '\n'

    message = msg.payload.decode("utf-8")
    if pretty:
        raw = json.loads(message)
        s += json.dumps(raw, indent=4, sort_keys=True)
        s += '\n'
    else:
        s += str(message)

    # Separator
    s += '=======================================\n'

    # Accumulate
    ac += s

    if verbose:
        print(s)

    lock.release()

# Kafka Consumer
class KafkaConsumerThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.daemon = True
        self.running = True

        # Essential configuration for Kafka
        conf = {
            'bootstrap.servers': bootstrap_servers,
            'group.id': kafka_group_id,
            'auto.offset.reset': 'earliest',
            'enable.auto.commit': True,
            'allow.auto.create.topics': True,
            'topic.metadata.refresh.interval.ms': 200,
            'session.timeout.ms': 6000,
            'max.poll.interval.ms': 10000
        }
        # SASL (optional)
        if kafka_security_protocol:
            conf['security.protocol'] = kafka_security_protocol
        if kafka_sasl_mechanism:
            conf['sasl.mechanism'] = kafka_sasl_mechanism
        if kafka_sasl_user is not None:
            conf['sasl.username'] = kafka_sasl_user
        if kafka_sasl_passwd is not None:
            conf['sasl.password'] = kafka_sasl_passwd
        self.consumer = Consumer(conf)
        self.kafka_topic = kafka_topic
        self.is_regex = self.kafka_topic.startswith("^") or self.kafka_topic.startswith("regex:")
        if self.is_regex:
            self.pattern_str = self.kafka_topic.replace("regex:", "") if self.kafka_topic.startswith("regex:") else self.kafka_topic
        else:
            self.pattern_str = None

    def run(self):
        # Subscription using regular expressions or direct topic

        try:
            topics_metadata = self.consumer.list_topics(timeout=1).topics.keys()
            if self.is_regex:
                print(f"Subscribing to Kafka topics with regex: {self.kafka_topic}")
                topics_match = [t for t in topics_metadata if re.match(self.pattern_str, t)]
                self.consumer.subscribe(topics_match, on_assign=self.on_assign)
            else:
                print(f"Subscribing to a Kafka topic: {self.kafka_topic}")
                self.consumer.subscribe([self.kafka_topic], on_assign=self.on_assign)
        except KafkaException as e:
            print(f"Subscription error: {e}")
            self.running = False
            return

        # Main consumption loop
        while self.running:
            try:
                while self.running:
                    msg = self.consumer.poll(1.0)
                    if msg is None:
                        continue

                    # ? MUY IMPORTANTE: manejar errores entregados por poll()
                    if msg.error():
                        # EOF es normal si habilitas enable.partition.eof; lo puedes ignorar
                        if msg.error().code() == KafkaError._PARTITION_EOF:
                            continue
                        print(f"Consumer error from poll(): {msg.error()}")
                        continue

                    self.process_message(msg)

            finally:
                self.consumer.close()

    def on_assign(self, consumer, partitions):
        print(f"Assigned partitions: {partitions}")

    def process_message(self, msg):
        global ac, t0, times
        with lock:
            # Record time
            if not t0:
                t0 = datetime.now()
                times.append(0)
            else:
                delta = datetime.now() - t0
                times.append(trunc(delta.total_seconds()))

            # Build record with key and headers
            s = f"Kafka message at topic {msg.topic()}\n"

            # 1. Print the message KEY
            if msg.key() is not None:
                try:
                    key_str = msg.key().decode('utf-8')
                except UnicodeDecodeError:
                    key_str = msg.key().hex()
                s += f"Key: {key_str}\n"
            else:
                s += "Key: (null)\n"

            # 2. Print the HEADERS of the message
            if msg.headers():
                s += "Headers:\n"
                for header_key, header_value in msg.headers():
                    try:
                        value_str = header_value.decode('utf-8') if header_value else "(empty)"
                    except UnicodeDecodeError:
                        value_str = header_value.hex()
                    s += f"  {header_key}: {value_str}\n"

            # 3. Print the payload
            s += "Payload:\n"
            try:
                payload = json.loads(msg.value().decode('utf-8'))
                s += json.dumps(payload, indent=4) if pretty else json.dumps(payload)
            except:
                s += msg.value().decode('utf-8', errors='replace')

            s += '\n=======================================\n'
            ac += s

            if verbose:
                print(s)

ac = ''
t0 = ''
times = []

if __name__ == '__main__':

    if mqtt_host is not None:
        # Initialize the MQTT Client and set callback methods
        client = mqtt.Client()
        client.on_connect = on_connect
        client.on_message = on_message

        client.connect(mqtt_host, mqtt_port, 60)

        # Enable the processing of MQTT network traffic, dispatches callbacks and
        # handles reconnecting.
        # Other loop*() functions are available that give a threaded interface and a
        # manual interface.
        client.loop_start()

    # Start Kafka consumer if configured
    kafka_thread = None
    if bootstrap_servers:
        kafka_thread = KafkaConsumerThread()
        kafka_thread.start()

    # Note that using debug=True breaks the procedure to write the PID into a file. In particular
    # makes the calle os.path.isfile(pidfile) return True, even if the file doesn't exist. Thus,
    # use debug=True below with care :)
    if (https):
        context = (cert_file, key_file)
        app.run(host=host, port=port, debug=False, ssl_context=context)
    else:
        app.run(host=host, port=port)
