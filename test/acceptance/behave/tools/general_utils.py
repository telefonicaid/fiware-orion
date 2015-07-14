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

import json
import random
import string
import time
import xmltodict
import datetime
import hashlib
import logging
import math
from decimal import Decimal


# general constants
EMPTY   = u''
XML     = u'xml'
JSON    = u'json'

__logger__ = logging.getLogger("utils")

def string_generator(size=10, chars=string.ascii_letters + string.digits):
    """
    Method to create random strings
    :param size: define the string size
    :param chars: the characters to be use to create the string
    return random string
    """
    return ''.join(random.choice(chars) for x in range(size))

def number_generator (size=5, decimals="%0.1f"):
    """"
    Method to create random number
    :param decimals: decimal account
    :param size: define the number size
    :return: random float
    """
    return float(decimals % (random.random() * (10**size)))

def convert_str_to_dict (body, content):
    """
    Convert string to Dictionary
    :param body: String to convert
    :param content: content type (json or xml)
    :return: dictionary
    """
    try:
        if content == XML:
            return xmltodict.parse(body)
        else:
            return json.loads(body)
    except Exception, e:
        assert False,  " ERROR - converting string to %s dictionary: \n%s \Exception error:\n%s" % (str(content), str(body), str(e))

def convert_dict_to_str (body, content):
    """
    Convert Dictionary to String
    :param body: dictionary to convert
    :param content: content type (json or xml)
    :return: string
    """
    try:
        if content == XML:
            return xmltodict.unparse(body)
        else:
            return json.dumps(body)
    except Exception, e:
        assert False,  " ERROR - converting %s dictionary to string: \n%s \Exception error:\n%s" % (str(content), str(body), str(e))

def convert_str_to_list (text, separator):
    """
    Convert String to list
    :param text: text to convert
    :param separator: separator used
    :return: list []
    """
    try:
        return text.split(separator)
    except Exception, e:
        assert False,  " ERROR - converting %s string to list with separator: %s \nException error:%s" % (str(text), str(separator), str(e))

def convert_list_to_string (list, separator):
    """
    Convert  List to String
    :param text: list to convert
    :param separator: separator used
    :return: string ""
    """
    try:
        return separator.join(list)
    except Exception, e:
        assert False,  " ERROR - converting list to string with separator: %s \nException error:%s" % (str(separator), str(e))

def show_times (init_value):
    """
    shows the time duration of the entire test
    :param initValue: initial time
    """
    print "**************************************************************"
    print "Initial (date & time): " + str(init_value)
    print "Final   (date & time): " + str(time.strftime("%c"))
    print "**************************************************************"

def generate_timestamp(date=EMPTY, format="%Y-%m-%dT%H:%M:%S.%fZ"):
    """
    generate timestamp or convert from a date with a given format
    ex: 1425373697
    :return  timestamp
    """
    if date == EMPTY:
        return time.time()
    return time.mktime(datetime.datetime.strptime(date, format).timetuple())

def generate_date_zulu(timestamp=0):
    """
    convert timestamp or generate to date & time zulu (UTC)
    ex: 2014-05-06T10:39:47.696Z
    :return date-time zulu formatted (UTC)
    """
    if timestamp == 0:
        timestamp = generate_timestamp()
    return str(datetime.datetime.fromtimestamp(timestamp).strftime("%Y-%m-%dT%H:%M:%S.%fZ"))

def get_date_only_one_value(date, value):
    """
    get only one specific value in a date-time
    :param date: date-time to get one specific value
    :param value: value to return ( year | month | day | hour | minute | second)
    :return string
    """
    dupla = {"year": "%Y", "month": "%m", "day": "%d", "hour": "%H", "minute": "%M", "second": "%S"}
    for value_in_dupla in dupla:
        if value_in_dupla == value.lower():
            return datetime.datetime.fromtimestamp(date).strftime(dupla[value_in_dupla])

def is_an_integer_value(value):
    """
    verify if a number is integer or not, verifying if the decimal part is equal to zero
    :param value: value to check
    :return: boolean (True is integer | False is float )
    """
    try:
        temp_value = Decimal(value)  #  Cannot convert float to Decimal.  First convert the float to a string
        dec_v, int_v = math.modf(temp_value)
        if dec_v == 0:
            return True
        return False
    except Exception, e:
       assert False, " Error - %s is not numeric... \n %s" % (str(value), str(e))

def generate_hash_sha512(input, limit=-1):
    """
    generate hash algorithms SHA512
    :param input: text to generate secure hash
    :param limit: number of digit returned (if -1 returns all digits (128))
    :return string
    """
    hash_resp = hashlib.sha512(input).hexdigest()
    if limit == -1:
        limit = len(hash_resp)
    return hash_resp[0:limit]


def mappingQuotes (attr_value):
        """
        limitation in lettuce change ' by " in mysql
        """
        temp = ""
        for i in range (len(attr_value)):
            if attr_value[i] == "'":  temp = temp + "\""
            else:temp = temp + attr_value[i]
        return temp

def read_file_to_json(file_name):
    """
    read a file and return a dictionary
    :param file_name: file to read (path included)
    :return: dict
    """
    try:
        with open(file_name) as config_file:
            return json.load(config_file)
    except Exception, e:
        raise Exception("\n-- ERROR -- parsing %s file\n     msg= %s" % (file_name, str(e)))
