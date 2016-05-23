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

import os

from termcolor import colored
from behave.model import ScenarioOutline

from components.common_steps.initial_steps import *
from components.common_steps.general_steps import *

__logger__ = logging.getLogger("environment")


# constants
# pre-actions in feature files
ACTIONS_BEFORE_FEATURE = u'actions before the feature'
ACTIONS_AFTER_FEATURE = u'actions after the feature'
ACTIONS_BEFORE_SCENARIO = u'actions before each scenario'
ACTIONS_AFTER_SCENARIO = u'actions after each scenario'
KEYWORDS = ["Setup", "Check"]  # prefix in steps to pre-actions
GIVEN_PREFIX = u'Given'
AND_PREFIX = u'And'
SEPARATOR = u':'
SHOW_SUMMARY = True  # determine whether the summary is displayed or not (boolean).
scenarios_status = {"untested": 0,  # The scenario was has not been completely tested yet.
                    "skipped": 0,   # One or more steps of this scenario was passed over during testing.
                    "passed": 0,    # The scenario was tested successfully.
                    "failed": 0}    # One or more steps of this scenario failed.

features_data = {"scenario_status": None,  # A summary status of the scenario’s run
                 "file_name": None,        # The file name where the feature was found
                 "duration": None,         # The time, in seconds, that it took to test this feature
                 "total": None}            # total of scenarios into the feature
features_list = []  # list of features executed with its name and scenarios status


def __create_log_folder(name):
    """
    verify if the folder exists and it does not exists, it is created
    :param name: log folder name
    """
    try:
        if not os.path.exists(name):
            os.makedirs(name)
            __logger__.info("log folder has been created with name: %s" % name)
    except Exception, e:
        assert False, "ERROR  - creating logs folder \n       - %s" % str(e)


def get_steps_from_feature_description(label, feature):
    """
    return all steps defined in feature description associated to label
    :param label: labels allowed:
       - Actions Before the Feature
       - Actions Before each Scenario
       - Actions After each Scenario
       - Actions After the Feature
    :param feature: feature values
    :return: list
    Hint: must be ":" in the step prefix (used as separator). ex: "Setup: "
    """
    steps_list = []
    label_exists = False
    for description in feature.description:
        if label_exists:
            if description.split(SEPARATOR)[0] in KEYWORDS:
                steps_list.append(description.replace(description.split(SEPARATOR)[0] + SEPARATOR, GIVEN_PREFIX))
            else:
                break
        if description.lower().find(label) >= 0:
            label_exists = True
    return steps_list


def replace_given_to_add(value):
    """
    replacing Given prefix to Add prefix
    :param value: text to verify
    :return: string
    """
    return value.replace(GIVEN_PREFIX, AND_PREFIX)


def execute_one_step(context, name, **kwargs):
    """
    execute a step manually
    :param name: step name
    :param show: determine if the steps is displayed or not (the "Given" label is replaced to "And" label)
    """
    show = kwargs.get("show", False)
    __logger__.debug("step defined in pre-actions: %s" % name)
    if show:
        print colored('    %s' % replace_given_to_add(name), 'green')
    context.execute_steps(name)


def init_feature():
    """
    features_data, scenarios_status dicts are initialized
    """
    global features_data, scenarios_status
    features_data = {"scenario_status": None, "file_name": ""}
    scenarios_status = {"untested": 0, "skipped": 0, "passed": 0, "failed": 0}


def process_scenario(scenario):
    """
    determine the status in each scenario
    :param scenario: scenario unique
    """
    scenarios_status[scenario.status or 'skipped'] += 1


def process_scenario_outline(scenario_outline):
    """
    determine the scenario status in each outline example
    :param scenario_outline: is executed for each row in the Examples section
    """
    for scenario in scenario_outline.scenarios:
        process_scenario(scenario)


def before_all(context):
    """
    actions before all
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __create_log_folder("logs")
    context.config.setup_logging(configfile="logging.ini")


def after_all(context):
    """
    actions after all
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    if SHOW_SUMMARY:
        summary_color = u'yellow'
        print colored("                    SUMMARY:", summary_color)
        print colored("-------------------------------------------------", summary_color)
        for item in features_list:
            status = item["scenario_status"]
            print colored("  - %s >> passed: %s, failed: %s, skipped: %s and total: %s with duration: %.3f seconds."
                          % (item["file_name"], status["passed"], status["failed"], status["skipped"], str(item["total"]),
                             item["duration"]), summary_color)
        print colored("-------------------------------------------------", summary_color)
        print colored("  Date: %s" % str(time.strftime("%A, %B %d, %Y %H:%M:%S")), summary_color)
        print colored("-------------------------------------------------", summary_color)


def before_feature(context, feature):
    """
    actions before each feature
    in case of backgroundFeature, re-throw steps defined in the feature descriptions
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    """
    __logger__.info("\n\n\n\n")
    __logger__.info(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
    __logger__.info(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
    __logger__.info("FEATURE name: %s" % feature)
    __logger__.info("FEATURE description: %s" % feature.description)
    # ---- actions before the feature ----
    steps_before_feature = get_steps_from_feature_description(ACTIONS_BEFORE_FEATURE, feature)
    print colored('')
    if len(steps_before_feature) > 0:
        print colored('')
        print colored('  Actions executed before the feature:')
    for item in steps_before_feature:
        execute_one_step(context, item, show=True)

    context.steps_after_feature = get_steps_from_feature_description(ACTIONS_AFTER_FEATURE, feature)
    context.steps_before_scenario = get_steps_from_feature_description(ACTIONS_BEFORE_SCENARIO, feature)
    context.steps_after_scenario = get_steps_from_feature_description(ACTIONS_AFTER_SCENARIO, feature)


def after_feature(context, feature):
    """
    actions executed after each feature
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param feature: feature properties
    """
    # ---- actions after the feature ----
    print colored('')
    if len(context.steps_after_feature) > 0:
        print colored('  Actions executed after the feature:')
    for item in context.steps_after_feature:
        execute_one_step(context, item, show=True)
    # ---- Summary by feature ----
    features_data["file_name"] = feature.filename
    features_data["scenario_status"] = scenarios_status
    features_data["duration"] = feature.duration
    for scenario in feature:
        if isinstance(scenario, ScenarioOutline):
            process_scenario_outline(scenario)
        else:
            process_scenario(scenario)
    features_data["total"] = scenarios_status["passed"] + scenarios_status["skipped"] + scenarios_status["failed"] + scenarios_status["untested"]
    features_list.append(features_data)
    init_feature()

    __logger__.info("AFTER FEATURE")
    __logger__.info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")
    __logger__.info("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<")


def before_scenario(context, scenario):
    """
    actions executed before each scenario
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param scenario: scenario properties
    """
    __logger__.info("==>>")
    __logger__.info("BEFORE SCENARIO: %s " % scenario)
    # ---- actions before each scenario ----
    print colored('')
    print colored('------------------------------------------------------------------------------')
    if len(context.steps_before_scenario) > 0:
        print colored('  Actions executed before each scenario(Background):')
    for item in context.steps_before_scenario:
        execute_one_step(context, item, show=True)


def after_scenario(context, scenario):
    """
    actions executed after each scenario
       - database used is deleted
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param scenario: scenario properties
    """
    # ---- actions after each scenario ----
    for item in context.steps_after_scenario:
        execute_one_step(context, item, show=True)
    __logger__.info("AFTER SCENARIO")
    __logger__.info("<<==")


def before_step(context, step):
    """
   actions executed before each step
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param step: step properties
    """
    __logger__.info("BEFORE STEP: %s" % step)


def after_step(context, step):
    """
   actions executed after each step
    :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
    :param step: step properties
    """
    __logger__.info("AFTER STEP")
