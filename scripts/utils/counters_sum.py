#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright 2023 Telefonica Investigacion y Desarrollo, S.A.U
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

# Usage:
#   counters_sum.py file1.json file2.json ... fileN.json
#
# (Maybe you need to use 'PYTHONIOENCODING=utf8 python counter_sum.py ...' if you are going to redirect the output of this
# script to a file)

# This script is designed to aggregate information coming of several CB instances running in parallel in HA or
# horizontal scaling scenarios. The files passed to the script as arguments are the response of the
# operation GET /v2/statistics?options=fullCounters
#
# Some notes to take into account:
#
# - All .json files have to have a regular structure (i.e. all with the same structural keys in the JSON object)
#   Thus, it is a must to use ?option=fullCounter in the GET operation
# - At the present moment this script only aggregates information in the "counters" section, but it would be
#   easily extended to take into account other sections

# Configuration
# PRUNE: if true remove entries with 0
PRUNE = True

__author__ = 'fermin'

import sys
import json

# Grab all files into JSON dicts
files = []
for f in sys.argv[1:]:
    files.append(json.load(open(f)))

# Initialize accumulator with first file
accum = files[0]['counters']

# Accum the rest of the files
for f in files[1:]:
    counters = f['counters']

    # Request types
    request_types = ['requests', 'requestsLegacy']

    # Update summaries
    summary_fields = [
        'jsonRequests',
        'textRequests',
        'noPayloadRequests',
        'missedVerb',
        'invalidRequests',
        'registrationUpdateErrors',
        'discoveryErrors',
        'notificationsSent'
    ]

    for field in summary_fields:
        accum[field] += counters[field]

    # Process request blocks
    for field in request_types:
        for url in accum[field]:
            for verb in accum[field][url]:
                accum[field][url][verb] += counters[field][url][verb]

if PRUNE:
    for field in request_types:
        # First pass: prune verbs with 0
        for url in accum[field].keys():
            to_delete = []
            for verb in accum[field][url].keys():
                if accum[field][url][verb] == 0:
                    to_delete.append(verb)
            for verb in to_delete:
                accum[field][url].pop(verb)
        # Second pass: prune URL without content
        to_delete = []
        for url in accum[field].keys():
            if len(accum[field][url].keys()) == 0:
                to_delete.append(url)
        for url in to_delete:
            accum[field].pop(url)

print(json.dumps(accum))
