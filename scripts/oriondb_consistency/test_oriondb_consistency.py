#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
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

__author__ = 'fermin'

# This is not the usual test that stimulates function and check assertions in the results :)
#
# Before running this test you have to load the entities testing set (validation_data.js) in the 'orion-validation'
# database in the local MongoDB database (check the MongoURI to match the one in your environment). You will get
# the result as log output.
#
# You can run this test under coverage, so you can check the coverage of the different rules in
# oriondb_consistency.py

import unittest
from pymongo import MongoClient
import logging

from oriondb_consistency import process_db

class TestEntitiesConsistency(unittest.TestCase):
    def test_process_db(self):
        # sets the logging configuration
        logging.basicConfig(
            level=logging.getLevelName('INFO'),
            format="time=%(asctime)s | lvl=%(levelname)s | msg=%(message)s",
            handlers=[
                logging.StreamHandler()
            ]
        )
        logger = logging.getLogger()

        # connect to MongoDB and process validation DB
        mongo_client = MongoClient('mongodb://localhost:37017')
        queries = {
            'entities': {},
            'csubs': {}
        }
        process_db(logger, 'orion-validation', mongo_client, False, queries, None, False)
