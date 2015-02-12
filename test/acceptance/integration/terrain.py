# -*- coding: utf-8 -*-
"""
Copyright 2014 Telefonica Investigación y Desarrollo, S.A.U

This file is part of fiware-orion-pep

fiware-orion-pep is free software: you can redistribute it and/or
modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

fiware-orion-pep is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public
License along with fiware-orion-pep.
If not, seehttp://www.gnu.org/licenses/.

For those usages not covered by the GNU Affero General Public License
please contact with::[iot_support@tid.es]
"""

__author__ = 'Jon'

from lettuce import before, world, after
from integration.tools.general_utils import stop_mock

@before.all
def before_all():
    world.entities = None
    world.attributes_consult = None
    world.attributes_creation = None
    world.context_elements = None
    world.notify_conditions = None
    world.context_registrations = None
    world.service = None
    world.subservice = None
    world.mock_pid = None
    world.mock_data = None
    world.payloads_count = -1
    world.response_count = -1
    world.cb_count = -1
    world.cb = {}
    world.payloads = {}
    world.responses = {}


@after.each_scenario
def after_each_scenario(scenario):
    stop_mock()