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

import copy
import re
from bson.objectid import ObjectId

from iotqatools.helpers_utils import *
from iotqatools.remote_log_utils import Remote_Log

# constants
EMPTY = u''
FIWARE_SERVICE_HEADER = u'Fiware-Service'
FIWARE_SERVICE_PATH_HEADER = u'Fiware-ServicePath'
ORION_PREFIX = u'orion'
PARAMETER = u'parameter'
TRACE = u'trace'
VALUE = u'value'

__logger__ = logging.getLogger("utils")


class NGSI:
    """
    manage Context broker operations
    """

    def __init__(self, **kwargs):
        """
        constructor
        """

    # ------------------------------------ validations ------------------------------------------
    def __get_mongo_cursor(self, mongo_driver, headers, **kwargs):
        """
        return a cursor with a entities list
        :param entities_contexts: entities context (attr_name, attr_value, attr_type, id, type, metadatas, etc)
        :param headers: headers dict (service, service_path, etc)
        :param mongo_driver: mongo driver from steps
        :return: list
        """
        entity_type = kwargs.get("type", EMPTY)
        entity_id = kwargs.get("id", EMPTY)
        subs_id = kwargs.get("subs_id", EMPTY)

        # Orion Context Broker interprets the tentant name (service) in lowercase, thus,
        # although you can use tenants such as in updateContext "MyService" it is not advisable
        if FIWARE_SERVICE_HEADER in headers:
            if headers[FIWARE_SERVICE_HEADER] == EMPTY:
                database_name = ORION_PREFIX
            else:
                database_name = "%s-%s" % (ORION_PREFIX, headers[FIWARE_SERVICE_HEADER])
        else:
            database_name = ORION_PREFIX

        if subs_id != EMPTY:
            collection = u'csubs'
            query = {'_id': ObjectId(subs_id)}  # use "from bson.objectid import ObjectId"
        else:
            collection = u'entities'
            if FIWARE_SERVICE_PATH_HEADER in headers:
                if headers[FIWARE_SERVICE_PATH_HEADER] == EMPTY:
                    service_path = "/"
                else:
                    service_path = headers[FIWARE_SERVICE_PATH_HEADER]
            else:
                service_path = "/"

            query = {"_id.id": {'$regex': '%s.*' % entity_id},
                     "_id.servicePath": service_path,
                     "_id.type": entity_type}

        __logger__.info("Database verified: %s" % database_name)
        __logger__.info("collection verified: %s" % collection)
        __logger__.info("query: %s" % str(query))

        curs = None
        curs_list = None

        mongo_driver.connect(database_name.lower())
        mongo_driver.choice_collection(collection)
        curs = mongo_driver.find_data(query)
        curs_list = mongo_driver.get_cursor_value(curs)
        __logger__.debug("documents stored in mongo:  %s" % str(curs_list))
        mongo_driver.disconnect()
        return curs_list

    def __verify_attributes(self, entity, entities_contexts):
        """
        verify attributes from mongo
        """
        # verify attributes
        for a in range(int(entities_contexts["attributes_number"])):  # manages N attributes
            if int(entities_contexts["attributes_number"]) == 1:
                attr_name = entities_contexts["attributes_name"]
            else:
                # prefix plus _<consecutive>. ex: room1_2
                attr_name = "%s_%s" % (entities_contexts["attributes_name"], str(a))

            if entities_contexts["attributes_name"] is not None:
                assert attr_name in entity[u'attrNames'], \
                    " ERROR -- attribute name: %s is not stored in attrNames list" % attr_name

                # '.' char is forbidden in MongoDB keys, so it needs to be replaced in attribute name.
                # (we chose '=' as it is a forbidden character in the API). When returning information to user,
                # the '=' is translated back to '.', thus from user perspective all works fine.
                if attr_name.find(".") >= 0:
                    attr_name = attr_name.replace(".", "=")
                    __logger__.warn("attribute name: %s is changed by \"=\" because \".\" is forbidden in MongoDB keys" % attr_name)

                assert attr_name in entity[u'attrs'], \
                    " ERROR -- attribute: %s is not stored in mongo" % attr_name
                assert entities_contexts["attributes_value"] == entity[u'attrs'][attr_name][u'value'], \
                    " ERROR -- attribute value: %s is not stored successful in mongo" % str(entities_contexts["attributes_value"])
                if entities_contexts["attributes_type"] != "none":
                    assert entities_contexts["attributes_type"] == entity[u'attrs'][attr_name][u'type'], \
                        " ERROR -- attribute type: %s is not stored successful in mongo" % entities_contexts["attributes_type"]
                # verify metadatas
                if entities_contexts["metadatas_number"] > 0:
                    md = entity[u'attrs'][attr_name][u'md']
                    for m in range(int(entities_contexts["metadatas_number"])):  # manages N metadatas
                        if int(entities_contexts["metadatas_number"]) == 1:
                            meta_name = entities_contexts["metadatas_name"]
                        else:
                            meta_name = "%s_%s" % (entities_contexts["metadatas_name"], str(m))
                        # '.' char is forbidden in MongoDB keys, so it needs to be replaced in attribute name.
                        # (we chose '=' as it is a forbidden character in the API). When returning information to user,
                        # the '=' is translated back to '.', thus from user perspective all works fine.
                        meta_name = meta_name.replace(".", "=")
                        assert meta_name in md, \
                            " ERROR - missing the metadata name \"%s\" " % meta_name
                        assert entities_contexts["metadatas_value"] == md[meta_name][u'value'], \
                            " ERROR -- metadata value: %s is not stored successful in mongo" % entities_contexts["metadatas_value"]
                        if entities_contexts["metadatas_type"] != "none":
                            assert entities_contexts["metadatas_type"] == md[meta_name][u'type'], \
                                " ERROR -- metadata type: %s is not stored successful in mongo" % entities_contexts["metadatas_type"]
                assert u'creDate' in entity[u'attrs'][attr_name], \
                    " ERROR -- creDate field into attribute does not exists in document"
                assert u'modDate' in entity[u'attrs'][attr_name], \
                    " ERROR -- modDate field into attribute does not exists in document"

            assert u'creDate' in entity, \
                " ERROR -- creDate field does not exists in document"
            assert u'modDate' in entity, \
                " ERROR -- modDate field does not exists in document"
        __logger__.debug(" Entity id prefix: \"%s\" is successful stored in mongo" % entities_contexts["entities_id"])
        __logger__.debug(" Entity type prefix: \"%s\" is successful stored in mongo" % entities_contexts["entities_type"])

    def verify_entities_stored_in_mongo(self, mongo_driver, entities_contexts, headers, stored=True):
        """
        verify that entities are stored in mongo
        :param entities_contexts:  entities context (see constructor in cb_v2_utils.py)
        :param headers: headers used (see "definition_headers" method in cb_v2_utils.py)
        :param stored: flag to verify if the entities is stored in mongo or not
        :param mongo_driver: mongo driver from steps
            ex: mongo document
                    {
                        "_id": {
                            "id": "room2_0",
                            "type": "room",
                            "servicePath": "/test"
                        },
                        "attrNames": ["timestamp_0"],
                        "attrs": {
                            "timestamp_0": {
                                "value": "017-06-17T07:21:24.238Z",
                                "type": "DateTime",
                                "md": [{
                                            "name": "very_hot_1",
                                            "type": "alarm",
                                            "value": "ScKoMdrQbM"
                                        },
                                        {
                                            "name": "very_hot_0",
                                            "type": "alarm",
                                            "value": "ScKoMdrQbM"
                                        }],
                                "creDate": 1437379476,
                                "modDate": 1437379556
                            }
                        },
                        "creDate": 1437379476,
                        "modDate": 1437379556
                    }
        """

        curs_list = self.__get_mongo_cursor(mongo_driver, headers, type=entities_contexts["entities_type"],
                                            id=entities_contexts["entities_id"])

        # verify entities
        for i in range(int(entities_contexts["entities_number"])):
            # verify if the entity is stored in mongo
            if stored:
                __logger__.debug("Number of doc read from mongo: %s" % str(i + 1))
                assert i < len(curs_list), " ERROR - the entity \"%s\" is not stored" % str(i)
                entity = curs_list[i]  # manages N entities
                # verify attributes
                self.__verify_attributes(entity, entities_contexts)
            # verify if the entity is not stored in mongo
            else:
                assert len(curs_list) == 0, "  ERROR - the entities should not have been saved in mongo"
                __logger__.debug(" Entity id: \"%s\" is not stored in mongo as expected" % entities_contexts["entities_id"])

        __logger__.debug("Total of entities: %s" % str(entities_contexts["entities_number"]))
        __logger__.debug("Total of docs read from mongo: %s" % str(len(curs_list)))

    def verify_entity_updated_in_mongo(self, mongo_driver, entities_contexts, headers, parameters):
        """
        verify that entities are not stored in mongo
        :param entities_contexts:  entities context (see constructor in cb_v2_utils.py)
        :param headers: headers used (see "definition_headers" method in cb_v2_utils.py)
        """
        if "type" in parameters:
            entities_type = parameters["type"]
        else:
            entities_type = entities_contexts["entities_type"]
        entity_list = self.__get_mongo_cursor(mongo_driver, headers, type=entities_type, id=entities_contexts["entities_id"])
        assert len(entity_list) > 0, " ERROR - notihing is returned from mongo, review the query in behave log"
        entity = entity_list[0]
        # verify attributes
        self.__verify_attributes(entity, entities_contexts)

    def verify_error_response(self, context, resp):
        """
        verify error response
        ex: {
              "error": "Bad Request",
              "description": "tenant name not accepted - a tenant string must not be longer than 50 characters and may
                              only contain underscores and alphanumeric characters"
            }
        :param context: parameters and values to evaluate
        :param resp: response from context broker
        """
        error = {}
        __logger__.debug("Response: Code: %s Body: %s" % (resp.status_code, resp.text))

        resp_list = convert_str_to_dict(resp.text, JSON)

        for row in context.table:
            error[row[PARAMETER]] = row[VALUE]

        assert "error" in resp_list, "ERROR - error field does not exists"
        assert error["error"] == resp_list["error"], 'ERROR -  error: "%s" is not the expected: "%s"' % \
                                                     (str(resp_list["error"]), str(error["error"]))
        if "description" in error:
            assert "description" in resp_list, "ERROR - description field does not exists"
            assert error["description"] == resp_list["description"], \
                'ERROR - description error: "%s" is not the expected: "%s"' % (str(resp_list["description"]),
                                                                               str(error["description"]))

    def __get_simple_quote_values(self, value):
        """
        get values with simple quote
        :param value: query parameter (string)
        :return list
        """
        text_to_comma_replace = "<&_i_&>"
        final_list = []
        posi = 0
        posf = 0
        has_quote = True

        while has_quote:
            quote = value[posi:].find("\'")
            if quote < 0:
                has_quote = False
            else:
                posi = posi + quote + 1
                if posi >= 0:
                    posf = value[posi:].find("'")
                    comma = value[posi:posf + posi].find(",")
                    if comma >= 0:
                        comma += posi
                        value = "%s%s%s" % (value[:comma], text_to_comma_replace, value[comma + 1:])
                posi = posi + posf + len(text_to_comma_replace) + 1
        l = value.split(",")
        for e in l:
            e = e.replace("'", "")  # remove the simpl quote
            e = e.replace(text_to_comma_replace, ",")  # replace temporal text to comma
            final_list.append(e)
        return final_list

    def __binary_unary_statements(self, value, entity_context):
        """
        determine if the statement matchs with the entity context
        :param entity_context: entity context to verify
        :return string
        """
        result = None
        unary_operator = ["+", "-"]
        binary_operator = ["==", "!=", ">", ">=", "<", "<="]
        q_list = value.split(";")

        for statement in q_list:
            result = None
            if statement[:1] in unary_operator:
                # unary statement
                op = statement[:1]
                # Unary type
                if statement.find("type") >= 0:
                    if op == "+" and entity_context["entities_type"] != "none":
                        result = "this entity has type"
                    if op == "-" and entity_context["entities_type"] is None:
                        result = "this entity has not type"
                # Unary attribute
                else:
                    if op == "+" and entity_context["attributes_name"] == statement[1:]:
                        result = "this attribute does exist"
                    if op == "-" and entity_context["attributes_name"] != statement[1:]:
                        result = "this attribute does not exist"

            # binary statement
            op = None
            for e in binary_operator:
                if statement.find(e) >= 0:
                    op = e
            if op is not None:
                result = None
                value = statement.split(op)
                assert len(value) == 2, " ERROR - It is necessary attribute and value (in that order)"
                if value[0].find(remove_quote(entity_context["attributes_name"])) >= 0:
                    items = []
                    if value[1].find("..") >= 0:  # value is a range, specified as a minimum and maximum separated by ..
                        items.append("range")
                        min_max = value[1].split("..")
                        items.append(min_max)

                        assert len(min_max) == 2, " WARN - It is necessary two values in range (minimum and maximum)"
                    else:  # The value is a single element
                        items = self.__get_simple_quote_values(value[1])

                    if items[0] == "range":
                        try:
                            if op == "==" and ((float(entity_context["attributes_value"]) >= float(items[1][0])) and
                                                   (float(entity_context["attributes_value"]) <= float(items[1][1]))):
                                result = "this statement does match"  # range q=attr==x..y
                            elif op == "!=" and ((float(entity_context["attributes_value"]) <= float(items[1][0])) or
                                                     (float(entity_context["attributes_value"]) >= float(items[1][1]))):
                                result = "this statement does match"  # range q=attr!=x..y
                        except Exception, e:
                            __logger__.warn("some value is not numeric format, %s" % str(e))
                    else:
                        for it in items:
                            eval = eval_binary_expr(entity_context["attributes_value"], op, it)
                            if op == "!=":  # AND operation
                                if eval:
                                    result = "this statement does match"
                                else:
                                    result = None
                                break
                            else:  # OR operation
                                if eval:
                                    result = "this statement does match"
                                    break

            if result is None:
                break
        return result

    def __mq_binary_unary_statements(self, mq):
        """
        get binary and unary statements with mq condition

        :param mq: mq string
        :return: list of dicts.
             Example of an internal dict:
              {'operator': '==',
               'attribute': 'picture',
               'metadata': 'color',
               'compound': 'secondary_color',  [OPTIONAL] [only in compound metadata]
               'value': 'red'}                 [OPTIONAL] [only in binaries cases]
        """
        mq_split = mq.split(";")

        mq_list = []
        operator = ["==", "!=", ">=", ">", "<=", "<", "!"]
        for item in mq_split:
            op_exist = False
            item_list = []
            for op in operator:
                if item.find(op) >= 0:
                    item_list = item.split(op)
                    item_list.append(op)
                    if op == "!":
                        item_list.pop(0)
                    op_exist = True
                    break
            if not op_exist:  # unary positive case
                item_list.append(item)
                item_list.append("+")

            mq_dict = {}
            name_split = item_list[0].split(".")
            mq_dict["attribute"] = name_split[0]
            mq_dict["metadata"] = name_split[1]
            if len(name_split) > 2:
                compound = u''
                for i in range(2, len(name_split)):
                    compound = compound + name_split[i]
                mq_dict["compound"] = compound
            mq_dict["operator"] = item_list[-1]
            if len(item_list) > 2:
                mq_dict["value"] = item_list[1]
            mq_list.append(mq_dict)
        return mq_list

    def __get_entities_context_group(self, accumulate, elements):
        """
        get entities context groups
        :param accumulate: accumulate of entities context
        :param elements: elements to compare to get entities context (id, type, etc)
        :return: list of dict
        """
        entities_list = []
        for entities_group in accumulate:
            group_exists = False
            for key, value in elements.items():
                result = None
                if key == "idPattern":
                    result = re.search(value, entities_group["entities_id"])
                elif key == "q":
                    result = self.__binary_unary_statements(value, entities_group)
                elif key == "entities_id":
                    result = re.search(remove_quote(entities_group["entities_id"]), value)
                else:
                    result = True if str(remove_quote(entities_group[key])) == str(value) else None

                if result is not None:
                    group_exists = True
                else:
                    group_exists = False
                    break

            __logger__.debug("group_exists: %s" % str(group_exists))
            if group_exists:
                entities_list.append(copy.deepcopy(entities_group))
        return entities_list

    def verify_get_all_entities(self, queries_parameters, accumulate_entities_context, resp, entities_returned):
        """
        verify get all entities
        :param queries_parameters: queries parameters used
        :param entities_context: context values
        :param entities_returned: number of entities returned
        :param resp: http response
        """
        total = 0
        limit = 20
        offset = 0
        ids = []
        types = []
        elements = {}
        group = []
        attrs_qp = []
        options_key_values = False
        for entities_group in accumulate_entities_context:
            total += int(entities_group["entities_number"])
            ids.append(remove_quote(entities_group["entities_id"]))
            types.append(remove_quote(entities_group["entities_type"]))

        if "limit" in queries_parameters:
            limit = int(queries_parameters["limit"])
        if "offset" in queries_parameters:
            offset = int(queries_parameters["offset"])
        if "id" in queries_parameters:
            del ids[:]
            ids = convert_str_to_list(queries_parameters["id"], ",")
            elements["entities_id"] = queries_parameters["id"]
        if "idPattern" in queries_parameters:
            del ids[:]
            ids = [queries_parameters["idPattern"]]
            elements["idPattern"] = queries_parameters["idPattern"]
        if "type" in queries_parameters:
            del types[:]
            types = convert_str_to_list(queries_parameters["type"], ",")
            elements["entities_type"] = queries_parameters["type"]
        if "attrs" in queries_parameters:
            attrs_qp = convert_str_to_list(queries_parameters["attrs"], ",")
        if "q" in queries_parameters:
            elements["q"] = queries_parameters["q"]
        if "options" in queries_parameters:
            options_key_values = True

        if elements != {}:
            total = 0
            group = self.__get_entities_context_group(accumulate_entities_context, elements)
            if group != []:
                for it in group:
                    total = total + it["entities_number"]

        # determinate number of items and position in response
        if offset >= total:
            items = 0
            position = 0
        elif limit + offset > total:
            items = total - offset
            position = offset
        else:
            items = limit
            position = offset

        # info in log
        __logger__.debug("total:  %s" % str(total))
        __logger__.debug("limit:  %s" % str(limit))
        __logger__.debug("offset: %s" % str(offset))
        __logger__.debug("items:  %s" % str(items))
        __logger__.debug("pos:    %s" % str(position))
        __logger__.debug("ids:    %s" % str(ids))
        __logger__.debug("types:  %s" % str(types))
        __logger__.debug("options_key_values:  %s" % str(options_key_values))

        # verify entities number
        items_list = convert_str_to_dict(resp.content, JSON)

        assert len(items_list) == int(entities_returned), "ERROR - in number of items in response\n  received: %s \n  expected: %s" % \
                                                          (str(len(items_list)), entities_returned)
        __logger__.debug("items returned: %d " % len(items_list))
        __logger__.debug("items expected to return: %s" % int(entities_returned))

        if int(entities_returned) != 0:
            sub_elements = {}
            for i in range(int(entities_returned)):
                sub_elements.clear()
                # verify entity id and entity type
                id_exists = False
                type_exists = False
                for id in ids:
                    if re.search(id, items_list[i]["id"]) is not None:
                        id_exists = True
                        sub_elements["entities_id"] = items_list[i]["id"]
                        break
                assert id_exists, 'ERROR - id %s does not exist' % items_list[i]["id"]

                if "type" in items_list[i]:
                    if items_list[i]["type"] != "none":
                        for tp in types:
                            if tp != "none":
                                if items_list[i]["type"].find(tp) >= 0:
                                    type_exists = True
                                    break
                        assert type_exists, 'ERROR - type field with value "%s" does not exist' % items_list[i]["type"]
                    sub_elements["entities_type"] = items_list[i]["type"]

                # verify attributes
                sub_group_list = self.__get_entities_context_group(accumulate_entities_context, sub_elements)
                assert len(sub_group_list) == 1, " ERROR - the id and type do not exist"
                sub_group = sub_group_list[0]
                # remove quotes in values
                for t in sub_group:
                    sub_group[t] = remove_quote(sub_group[t])

                for attr in range(int(sub_group["attributes_number"])):
                    if int(sub_group["attributes_number"]) == 1:
                        attr_name = remove_quote(sub_group["attributes_name"])
                    else:
                        # prefix plus _<consecutive>. ex: room1_2
                        for a in range(int(sub_group["attributes_number"])):
                            attr_name = "%s_%s" % (sub_group["attributes_name"], str(a))
                    if len(attrs_qp) > 0 and attr_name in attrs_qp:
                        # verify attributes name
                        assert attr_name in items_list[i], \
                            'ERROR - attribute name "%s" does not exist in position:%s' % (attr_name, str(i))
                        # verify attributes value and attribute type
                        attribute = items_list[i][attr_name]
                        try:
                            # if attr_value is numeric is convert in float
                            if str(sub_group["attributes_value"]).isdigit():
                                sub_group["attributes_value"] = float(sub_group["attributes_value"])
                            if str(attribute["value"]).isdigit():
                                attribute["value"] = float(attribute["value"])
                            if str(sub_group["attributes_value"]) in ("true", "false"):
                                sub_group["attributes_value"] = convert_str_to_bool(sub_group["attributes_value"])
                        except Exception, e:
                            __logger__.warn(" WARN - the attribute value is not verified if it is digit: \n   - %s" % e)
                        if not options_key_values:
                            if sub_group["attributes_type"] != "none":
                                assert sub_group["attributes_type"] == attribute["type"], \
                                    'ERROR - in attribute type "%s" in position: %s' % (sub_group["attributes_type"], str(i))

                                assert sub_group["attributes_value"] == attribute["value"], \
                                    'ERROR - in attribute value "%s" in position: %s' % (sub_group["attributes_value"], str(i))

                            # verify attribute metadatas
                            if sub_group["metadatas_number"] > 0:
                                for m in range(int(sub_group["metadatas_number"])):
                                    if int(sub_group["metadatas_number"]) > 1:
                                        meta_name = "%s_%s" % (sub_group["metadatas_name"], str(m))
                                    else:
                                        meta_name = sub_group["metadatas_name"]

                                    assert meta_name in attribute["metadata"], \
                                        'ERROR - attribute metadata name "%s" does not exist' % meta_name
                                    metadata = attribute["metadata"][meta_name]
                                    if sub_group["metadatas_type"] != "none":
                                        assert sub_group["metadatas_type"] == metadata["type"], \
                                            'ERROR - in attribute metadata type "%s"' % sub_group["metadatas_type"]
                                    if sub_group["metadatas_value"] != "none":
                                        assert sub_group["metadatas_value"] == metadata["value"], \
                                            'ERROR - in attribute metadata value "%s"' % sub_group["metadatas_value"]

    def verify_headers_response(self, context):
        """
        verify headers in response, it is possible to use regular expressions
        Ex:
          | parameter          | value                |
          | Fiware-Total-Count | 5                    |
          | Location           | /v2/subscriptions/.* |
        :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
        """
        headers = {}  # headers to verify
        for row in context.table:
            headers[row[PARAMETER]] = row[VALUE]
        headers_list = dict(context.resp.headers)  # headers returned in response
        for h in headers:
            assert h in headers_list, " ERROR, %s header does no exist in headers response\n -- %s" % (h, str(headers_list))
            __logger__.info("\"%s\" header does exist in headers response" % h)
            prog = re.compile(headers[h])
            result = prog.match(headers_list[h])
            assert result is not None, " ERROR - the \"%s\" header value expected (%s) not matches with headers response value: \"%s\"" % \
                                       (h, headers[h], headers_list[h])
            __logger__.info("\"%s\" header expected does match with \"%s\" in headers response " % (h, result.group(0)))

    @staticmethod
    def __verify_if_is_int_or_str(value):
        """
        verify if the values is int or not, and returns only int not float
        verify if the values is str or not, and returns only str not unicode
        :param value:
        :return: int
        """
        if type(value) == float and is_an_integer_value(value):
            return int(value)
        elif type(value) == unicode:
            return str(value)
        else:
            return value

    def verify_entity_raw_mode_http_response(self, entities_context, resp, field_type):
        """
        verify an entity in raw mode with type in attribute value from http response
        :param entities_context: entities context
        :param resp: http response
        :param field_type: field type (bool | int | float | list | dict | str | NoneType)
        """
        assert resp.text != "[]", "ERROR - It has not returned any entity"
        entity = convert_str_to_dict(resp.text, JSON)
        if type(entity) is not dict:
            entity = entity[0]  # in raw mode is used only one entity
        assert remove_quote(entities_context["entities_id"]) == entity["id"], \
            'ERROR - in id "%s" in raw mode' % (entity["id"])
        assert remove_quote(entities_context["entities_type"]) == entity["type"], \
            'ERROR - in type "%s" in raw mode' % (entity["type"])

        # verify attribute fields
        assert remove_quote(entities_context["attributes_name"]) in entity, \
            'ERROR - attribute name "%s" does not exist in raw mode' % entities_context["attributes_name"]
        attribute = entity[remove_quote(entities_context["attributes_name"])]
        entities_context["attributes_type"] = self.__change_attribute_type(entities_context["attributes_value"],
                                                                           entities_context["attributes_type"])

        assert remove_quote(entities_context["attributes_type"]) == attribute["type"], \
            'ERROR - in attribute type "%s" in raw mode' % entities_context["attributes_type"]

        if str(type(attribute["value"])) == "<type 'float'>":
            if attribute["value"] % 1 == 0:
                attribute["value"] = int(attribute["value"])
        if str(type(attribute["value"])) == "<type 'unicode'>":
            attribute["value"] = str(attribute["value"])

        assert str(type(attribute["value"])) == "<type '%s'>" % field_type, \
            'ERROR - in attribute value "%s" with type "%s" does not match' % (str(attribute["value"]), field_type)

    def verify_an_entity_by_id(self, queries_parameters, entities_context, resp, entity_id_to_test, **kwargs):
        """
        verify an entity by id
         :param queries_parameters: queries parameters used
        :param entities_context: context values
        :param resp: http response
        :param entity_id_to_test: entity id used to get an entity (to test)
        ex:
             {
                "id":"room_0",
                "type":"house",
                "temperature_0":{
                    "type":"celsius",
                    "value":"34",
                    "very_hot_1":{
                        "type":"alarm",
                        "value":"hot"
                    }
                }
            }
        """
        attrs_request = kwargs.get("attrs", False)
        attrs_list = []
        resp_json = convert_str_to_dict(resp.content, JSON)
        __logger__.debug("query parameter: %s" % str(queries_parameters))
        if not attrs_request:
            assert resp_json["id"] == entity_id_to_test, 'ERROR - in id "%s"' % resp_json["id"]
            if entities_context["entities_type"] != "none":
                assert resp_json["type"] == entities_context["entities_type"], 'ERROR - in type "%s" ' % resp_json["type"]

        # attr query parameter
        if "attrs" in queries_parameters:
            attrs_list = queries_parameters["attrs"].split(",")
        else:
            if int(entities_context["attributes_number"]) == 1:
                attrs_list.append(entities_context["attributes_name"])
            else:
                for e in range(int(entities_context["attributes_number"])):
                    attrs_list.append("%s_%s" % (entities_context["attributes_name"], str(e)))

        # options query parameter
        if "options" in queries_parameters:
            __logger__.debug("options query parameter: %s" % str(queries_parameters["options"]))
            # options = keyValues
            if queries_parameters["options"].find("keyValues") >= 0:
                # verify attributes
                for i in range(len(attrs_list)):
                    attr_name = attrs_list[i]
                    __logger__.info("attribute name: %s" % attr_name)
                    assert attr_name in resp_json, 'ERROR - in attribute name "%s" ' % attr_name
                    attribute = resp_json[attr_name]
                    if entities_context["attributes_value"] != "none":
                        assert entities_context["attributes_value"] == attribute, \
                            'ERROR - in attribute value "%s" with keyValues option' % entities_context["attributes_value"]
            # options = values
            elif queries_parameters["options"].find("values") >= 0:
                # verify attributes value
                c = 0
                for i in range(len(attrs_list)):
                    if entities_context["attributes_value"] != "none":
                        assert entities_context["attributes_value"] == resp_json[c], \
                            'ERROR - in attribute value "%s" with values option' % entities_context["attributes_value"]
                    c = +1
            # options = unique
            elif queries_parameters["options"].find("unique") >= 0:
                # verify attributes value
                for i in range(len(attrs_list)):
                    if entities_context["attributes_value"] != "none":
                        assert entities_context["attributes_value"] in resp_json, \
                            'ERROR - in attribute value "%s" with unique option' % entities_context["attributes_value"]
            # options = normalized
            else:
                # verify attributes
                for i in range(len(attrs_list)):
                    attr_name = attrs_list[i]
                    __logger__.info("attribute name: %s" % attr_name)
                    assert attr_name in resp_json, 'ERROR - in attribute name "%s" ' % attr_name
                    attribute = resp_json[attr_name]
                    # verify attribute type
                    if entities_context["attributes_type"] != "none":
                        assert entities_context["attributes_type"] == attribute["type"], \
                            'ERROR - in attribute type "%s" with normalized option' % entities_context["attributes_type"]
                    # verify attribute value
                    if entities_context["attributes_value"] is not None:
                        assert entities_context["attributes_value"] == attribute["value"], \
                            'ERROR - in attribute value "%s" with normalized option' % entities_context["attributes_value"]
                    # verify attribute metadatas
                    if entities_context["metadatas_number"] > 0:
                        for m in range(int(entities_context["metadatas_number"])):
                            if int(entities_context["metadatas_number"]) == 1:
                                meta_name = entities_context["metadatas_name"]
                            else:
                                meta_name = "%s_%s" % (entities_context["metadatas_name"], str(m))
                            assert meta_name in attribute["metadata"], \
                                'ERROR - attribute metadata name "%s" does not exist with normalized option' \
                                % meta_name
                            metadata = attribute["metadata"][meta_name]
                            if entities_context["metadatas_type"] != "none":
                                assert entities_context["metadatas_type"] == metadata["type"], \
                                    'ERROR - in attribute metadata type "%s" with normalized option' % entities_context["metadatas_type"]
                            if entities_context["metadatas_value"] != "none":
                                assert entities_context["metadatas_value"] == metadata["value"], \
                                    'ERROR - in attribute metadata value "%s" with normalized option' % entities_context["metadatas_value"]

    def verify_an_attribute_by_id(self, entities_context, resp, attribute_name_to_request):
        """
        verify that the attribute by ID is returned
        :param entities_context:
        :param resp:
        :param attribute_name_to_request:
        ex:
            {
                timestamp: {
                    type: "DateTime"
                    value: "017-06-17T07:21:24.238Z"
                    "metadata": {
                          very_hot: {
                              type: "alarm"
                              value: "false"
                          }
                    }
                }
            }
        """
        attribute = convert_str_to_dict(resp.content, JSON)
        if entities_context["attributes_type"] != "none":
            assert entities_context["attributes_type"] == attribute["type"], \
                'ERROR - in attribute type "%s"' % (entities_context["attributes_type"])
        if entities_context["attributes_value"] is not None:
            assert entities_context["attributes_value"] == attribute["value"], \
                'ERROR - in attribute value "%s"' % (entities_context["attributes_value"])

            # verify attribute metadatas
        if entities_context["metadatas_number"] > 0:
            for m in range(int(entities_context["metadatas_number"])):
                if int(entities_context["metadatas_number"]) > 1:
                    meta_name = "%s_%s" % (entities_context["metadatas_name"], str(m))
                else:
                    meta_name = entities_context["metadatas_name"]
                assert meta_name in attribute["metadata"], 'ERROR - attribute metadata name "%s" does not exist' % meta_name
                metadata = attribute["metadata"][meta_name]
                if entities_context["metadatas_type"] != "none":
                    assert entities_context["metadatas_type"] == metadata["type"], \
                        'ERROR - in attribute metadata type "%s"' % (entities_context["metadatas_type"])
                if entities_context["metadatas_value"] is not None:
                    assert entities_context["metadatas_value"] == metadata["value"], \
                        'ERROR - in attribute metadata value "%s"' % (entities_context["metadatas_value"])

    def verify_an_attribute_by_id_in_raw_mode_http_response(self, entities_context, resp, attribute_name_to_request, field_type):
        """
        verify an attribute by ID in raw mode with type in attribute value from http response
        used mainly to compound, boolean, special attributes values
        :param entities_context:
        :param resp:
        :param attribute_name_to_request:
        :param field_type:
        """
        assert resp.text != "[]", "ERROR - It has not returned any entity"
        attribute = convert_str_to_dict(resp.text, JSON)  # in raw mode is used only one entity
        __logger__.debug("attribute: %s" % str(attribute))

        if entities_context["attributes_type"] != "none":
            assert remove_quote(entities_context["attributes_type"]) == attribute["type"], \
                'ERROR - in attribute type "%s" in raw mode' % (remove_quote(entities_context["attributes_type"]))

        attribute["value"] = self.__verify_if_is_int_or_str(attribute["value"])
        assert str(type(attribute["value"])) == "<type '%s'>" % field_type, \
            'ERROR - in attribute value "%s" with type "%s" does not match' % (str(entities_context["attributes_value"]), field_type)

    def verify_an_attribute_value_by_id(self, entities_context, resp):
        """
        verify an attribute value by ID
        :param entities_context:
        :param resp:
        ex:
            2017-06-17T07:21:24.238Z (text/plain)
            or:
            {"a":45, "b":true} (application/json)
        """
        json_object = False
        json_chars = ["{", "["]
        __logger__.debug("Content-Type in response:  %s" % resp.headers["content-type"])
        for o in json_chars:  # determine if the response is an json object or not
            if resp.content.find(o) >= 0:
                json_object = True
        try:
            if json_object:
                resp_json = convert_str_to_dict(resp.content, JSON)
                assert cmp(resp_json, entities_context["attributes_value"]) == 0, "ERROR - the value %s is not the expected: %s" % (
                str(resp_json), str(entities_context["attributes_value"]))
            else:
                assert str(resp.content) == str(entities_context["attributes_value"]), "ERROR - the value %s is not the expected: %s" % (
                str(resp.content), str(entities_context["attributes_value"]))
        except Exception, e:
            __logger__.error(e)

    def verify_attribute_is_deleted(self, mongo_driver, entities_contexts, headers, parameters):
        """
        verify if the attribute has been deleted
        :param entity_context:
        :param headers:
        """
        __logger__.debug("parameters: %s" % str(parameters))
        __logger__.debug("headers: %s" % str(headers))
        __logger__.debug("properties: %s" % str(entities_contexts))
        if "type" in parameters:
            entities_type = parameters["type"]
        else:
            entities_type = entities_contexts["entities_type"]
        curs_list = self.__get_mongo_cursor(mongo_driver, headers, type=entities_type, id=entities_contexts["entities_id"])

        # verify attribute in entity
        __logger__.debug("Number of docs read from mongo: %s" % str(len(curs_list)))
        __logger__.debug("attribute name to verify: %s" % entities_contexts["attributes_name"])
        assert len(curs_list) == 1, " ERROR - should be returned only one entity"
        entity = curs_list[0]
        assert entities_contexts["attributes_name"] not in entity["attrNames"], \
            " ERROR - the attribute: %s exists in the entity " % entities_contexts["attributes_name"]

    def verify_entity_types(self, types, resp):
        """
        verify entity types  -- /v2/types
        :param types: type list to check
        :param resp: http response
        """
        # verify entities types
        type_list = types.split(",")
        items_list = convert_str_to_dict(resp.content, JSON)
        for item in items_list:
            __logger__.debug("checking: %s include in %s" % (item["type"], types))
            assert item["type"] in type_list, " ERROR - \"%s\" type does not match with types to verify" % item["type"]

    def verify_attributes_types_with_entity_types(self, queries_parameters, accumulate_entities_context, prefixes, resp):
        """
        verify attributes types with entity types  -- /v2/types
        :param queries_parameters: queries parameters used in the request
        :param accumulate_entities_context: accumulate of all entities context. See "entity_context" dict in cb_v2_utils.py
        :param resp: http response
        """
        types_count = {}
        total = 0
        limit = 20
        offset = 0

        for entities_group in accumulate_entities_context:
            temp = remove_quote(entities_group["entities_type"])
            if temp not in types_count:  # avoid types duplicated
                if prefixes["type"]:
                    for i in range(int(entities_group["entities_number"])):
                        types_count["%s_%s" % (entities_group["entities_type"], str(i))] = 1
                        total += 1
                else:
                    types_count[temp] = int(entities_group["entities_number"])
                    total += 1
            else:
                types_count[temp] += int(entities_group["entities_number"])  # entities counter

        if "limit" in queries_parameters:
            limit = int(queries_parameters["limit"])
        if "offset" in queries_parameters:
            offset = int(queries_parameters["offset"])
        if "options" in queries_parameters:
            if queries_parameters["options"] == "values":
                pass  # not implemented (develop team) and not tested yet (pending)

        # determinate number of items and position in response
        if offset >= total:
            items = 0
            position = 0
        elif limit + offset > total:
            items = total - offset
            position = offset
        else:
            items = limit
            position = offset

        # info in log
        __logger__.debug("total:  %s" % str(total))
        __logger__.debug("limit:  %s" % str(limit))
        __logger__.debug("offset: %s" % str(offset))
        __logger__.debug("items:  %s" % str(items))
        __logger__.debug("pos:    %s" % str(position))
        __logger__.debug("types count:  ")
        for i in types_count:
            __logger__.debug("   %s = %s" % (i, str(types_count[i])))

        # verify entities number
        items_list = convert_str_to_dict(resp.content, JSON)
        assert items == len(items_list), " ERROR - the entities group total: %d is not the expected:%d" % (len(items_list), items)
        for item in items_list:
            assert item["type"] in types_count, " ERROR - wrong entity type: %s" % item["type"]  # verify that the entities was created

            assert types_count[item["type"]] == int(item["count"]), \
                " ERROR - the entities type counter %s does not match with the expected: %s" % \
                (item["count"], types_count[item["type"]])

            # list of attributes in each entities group
            attr_list = item["attrs"].keys()
            for attr in attr_list:
                for entities_group in accumulate_entities_context:
                    attr_exists = False
                    entities_group["attributes_type"] = self.__change_attribute_type(entities_group["attributes_value"],
                                                                                     entities_group["attributes_type"])
                    if str(item["type"]).find(str(entities_group["entities_type"])) >= 0:
                        if attr.find(entities_group["attributes_name"]) >= 0:
                            if entities_group["attributes_type"] in item["attrs"][attr]["types"]:
                                attr_exists = True
                                break
                assert attr_exists, ' ERROR - the attribute: "%s" with type: "%s" does not exist...' % (
                attr, str(item["attrs"][attr]["types"]))
            __logger__.info(u'"%s" attribute and its types are verified successfully' % attr)
            # count field
            assert "count" in item, "ERROR - the count field does not exist in response"
            assert item["count"] == types_count[
                item["type"]], u' ERROR - "count" field in response (%s) does not match with the expected (%s)' \
                               % (str(item["count"]), types_count[item["type"]])
            __logger__.info(u'"count" field is verified successfully')

    def verify_attributes_types_by_entity_types(self, entity_type, accumulate_entities_context, resp):
        """
        verify attributes types by entity types  -- /v2/types/<entity_type>
        :param accumulate_entities_context: accumulate of all entities context. See "entity_context" dict in cb_v2_utils.py
        :param resp: http response
        """
        entities_counter = 0
        for entities_group in accumulate_entities_context:
            if entities_group["entities_type"] == entity_type:
                entities_counter += int(entities_group["entities_number"])  # entities counter

        attrs_dict = convert_str_to_dict(resp.content, JSON)
        # list of attributes
        attr_list = attrs_dict["attrs"].keys()  # list of attrs names (keys)
        for attr in attr_list:
            for entities_group in accumulate_entities_context:
                attr_exists = False
                if (attr.find(entities_group["attributes_name"]) >= 0):
                    entities_group["attributes_type"] = self.__change_attribute_type(entities_group["attributes_value"],
                                                                                     entities_group["attributes_type"])
                    if entities_group["attributes_type"] in attrs_dict["attrs"][attr]["types"]:
                        attr_exists = True
                        break
            assert attr_exists, ' ERROR - the attribute: "%s" with type: "%s" does not exist...' % (
            attr, str(attrs_dict["attrs"][attr]["types"]))
            __logger__.info(u'"%s" attribute and its types are verified successfully' % attr)
            # count field
            assert "count" in attrs_dict, "ERROR - the count field does not exist in response"
            assert attrs_dict["count"] == entities_counter, u' ERROR - "count" field in response (%s) does not match with the expected (%s)' \
                                                            % (str(attrs_dict["count"]), entities_counter)
            __logger__.info(u'"count" field is verified successfully')

    def verify_log(self, context, line):
        """
        verify if traces match in the log line
        :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
        :param line: log line to seek all traces
        """
        trace = {}
        for row in context.table:
            trace[row[TRACE]] = row[VALUE]

        for key, value in trace.items():
            assert line.find(key) >= 0, u'ERROR - the "%s" trace does not exist in the line: \n   %s' % (key, line)
            __logger__.info(u' the "%s" trace does exist in log line' % key)
            if trace[key] != "ignored":
                remote_log = Remote_Log()
                line_value = remote_log.get_trace(line, key)
                assert line_value.find(trace[key]) >= 0, \
                    u' ERROR - the "%s" value expected in the "%s" trace does not match with "%s" line value...' \
                    % (trace[key], key, line_value)

    def verify_subscription_stored_in_mongo(self, mongo_driver, subscription_context, headers, resp):
        """
        verify that the subscription is stored in mongo

        :param mongo_driver: mongo driver from steps
        :param subscription_contexts: subscription context (see constructor in cb_v2_utils.py)
        :param headers: headers used (see "definition_headers" method in cb_v2_utils.py)
        :param resp: http response
           ex: mongo document (new format after of that BD has been migrated)
            {
                "status": "active",
                "description": "my first subscription",
                "reference": "http://localhost:1234",
                "throttling": 5L,
                "expression": {
                    "q": "temperature>40",
                    "geometry": "point",
                    "georel": "near;minDistance:1000",
                    "coords": "40,6391",
                    "mq": ""
                },
                "format": "normalized",
                "custom": False,
                "blacklist": False,
                "entities": [{
                    "type": "room_0",
                    "id": ".*",
                    "isPattern": "true"
                }, {
                    "type": "room_1",
                    "id": ".*",
                    "isPattern": "true"
                }],
                "expiration": 1459864800L,
                "servicePath": "/test",
                "_id": ObjectId("579b1c000b759403c9464dc4"),
                "conditions": ["temperature_0", "temperature_1", "temperature_2"],
                "attrs": ["temperature_0", "temperature_1", "temperature_2"]
             }
        """
        subs_id = resp.headers["location"].split("subscriptions/")[1]
        curs_list = self.__get_mongo_cursor(mongo_driver, headers, subs_id=subs_id)
        assert len(curs_list) == 1, " ERROR - the subscription docs number in mongo is not the expected: %s" % str(len(curs_list))
        curs = curs_list[0]
        __logger__.debug("subscription to verify: %s" % str(curs))
        for item in subscription_context:  # used to remove quotes when are using raw mode
            if item != "condition_expression":
                subscription_context[item] = remove_quote(subscription_context[item])

        # entities field and its sub-fields
        for e in range(int(subscription_context["subject_entities_number"])):
            # entities - idPattern
            if subscription_context["subject_idPattern"] is not None:
                assert curs["entities"][e]["id"] == subscription_context["subject_idPattern"], \
                    u' ERROR - the id field with idPattern "%s" is not stored correctly' % subscription_context["subject_idPattern"]
                assert curs["entities"][e]["isPattern"] == "true", \
                    u' ERROR - the idPattern field "%s" is not true' % curs["entities"][e]["isPattern"]
            # entities - id
            if subscription_context["subject_id"] is not None:
                if subscription_context["subject_entities_prefix"] == "id" and subscription_context["subject_entities_number"] > 1:
                    assert curs["entities"][e]["id"] == "%s_%s" % (subscription_context["subject_id"], str(e)), \
                        u' ERROR - the id field "%s" is not stored correctly' % subscription_context["subject_id"]
                else:
                    assert curs["entities"][e]["id"] == subscription_context["subject_id"], \
                        u' ERROR - the id field "%s" is not stored correctly' % subscription_context["subject_id"]
            # entities - type
            if subscription_context["subject_type"] == EMPTY: subscription_context["subject_type"] = None
            if subscription_context["subject_type"] is not None:
                if subscription_context["subject_entities_prefix"] == "type" and subscription_context["subject_entities_number"] > 1:
                    assert curs["entities"][e]["type"] == "%s_%s" % (subscription_context["subject_type"], str(e)), \
                        u' ERROR - the type field "%s" is not stored correctly' % subscription_context["subject_type"]
                else:
                    assert curs["entities"][e]["type"] == subscription_context["subject_type"], \
                        u' ERROR - thetype field "%s" is not stored correctly' % subscription_context["subject_type"]
        __logger__.info("entities field an its sub-fields are verified successfully")

        # conditions fields
        if subscription_context["condition_attrs"] is not None:
            # conditions - attributes
            for a in range(int(subscription_context["condition_attrs_number"])):
                if int(subscription_context["condition_attrs_number"]) > 1:
                    condition_attr = "%s_%s" % (subscription_context["condition_attrs"], str(a))
                else:
                    condition_attr = subscription_context["condition_attrs"]
                if subscription_context["condition_attrs"] == "without condition field":
                    assert len(curs["conditions"]) == 0, \
                        u' ERROR - the condition attrs "%s" are not empty into DB' % str(curs["conditions"])
                else:
                    assert condition_attr in curs["conditions"], \
                        u' ERROR - the condition attr "%s" does not exist in conditions values into DB' % condition_attr
            __logger__.info("attributes in the \"conditions\" field are verified successfully")

        # expression field
        if subscription_context["condition_expression"] is not None and subscription_context["condition_expression"] != "object is empty":
            exp_op = subscription_context["condition_expression"].split("&")
            for op in exp_op:
                exp_split = op.split(">>>")
                for i in range(len(exp_split)):  # used to remove quotes when are using raw mode
                    exp_split[i] = remove_quote(exp_split[i])
                assert exp_split[1] == curs["expression"][exp_split[0]], \
                    u' ERROR - the "%s" key does not match with the expected value: %s' % (exp_split[0], exp_split[1])
        else:
            keys = ["q", "geometry", "georel", "coords"]
            for k in keys:
                assert curs["expression"][k] == "", u' ERROR - the "%s" key in expression is not empty or not exists' % k
        __logger__.info("expressions in the \"conditions\" field are verified successfully")

        # reference field
        if subscription_context["notification_http_url"] is not None:
            assert subscription_context["notification_http_url"] == curs["reference"], \
                u' ERROR - the http url "%s" is not the expected' % curs["reference"]
            __logger__.info("notification http url field is verified successfully")
        elif subscription_context["notification_http_custom_url"] is not None:
            assert subscription_context["notification_http_custom_url"] == curs["reference"], \
                u' ERROR - the httpCustom url "%s" is not the expected' % curs["reference"]
            assert "custom" in curs, " ERROR - custom field does not exist in DB"
            assert curs["custom"] == True, " ERROR - custom field \"%s\" in DB is not activated" % curs["custom"]
            __logger__.info("notification httpCustom url field is verified successfully")
        else:
            assert EMPTY == curs["reference"], u' ERROR - the reference "%s" is not the expected' % curs["reference"]

        # throttling field
        if subscription_context["throttling"] is None:
            subscription_context["throttling"] = 0
        assert curs["throttling"] == int(subscription_context["throttling"]), u' ERROR - the throttling "%s" is not the expected' % curs[
            "throttling"]
        __logger__.info("throttling in the \"notification\" field is verified successfully")

        # expiration field
        if subscription_context["expires"] is None or subscription_context["expires"] == EMPTY:
            ts_expires = 9000000000000000000
        else:
            ts_expires = generate_timestamp(date=subscription_context["expires"], utc=True)
        __logger__.info("expires: %s changed to timestamp: %s" % (subscription_context["expires"], ts_expires))
        assert int(curs["expiration"]) == int(ts_expires), u' ERROR - the expiration "%s" is not the expected' % str(curs["expiration"])
        __logger__.info("expiration field is verified successfully")

        # service path field
        ALL_SERVICE_PATHS = u'/#'
        if FIWARE_SERVICE_PATH_HEADER in headers:
            if headers[FIWARE_SERVICE_PATH_HEADER] == EMPTY:
                service_path = ALL_SERVICE_PATHS
            else:
                service_path = headers[FIWARE_SERVICE_PATH_HEADER]
        else:
            service_path = ALL_SERVICE_PATHS
        assert curs["servicePath"] == service_path, u' ERROR the servicePath field "%s" is not the expected "%s"' % (
        curs["servicePath"], service_path)
        __logger__.info("servicePath field is verified successfully")

        # attrs and exceptAttrs fields
        for a in range(int(subscription_context["notification_attrs_number"])):
            if int(subscription_context["notification_attrs_number"]) > 1:
                if curs["blacklist"]:
                    condition_attr = "%s_%s" % (subscription_context["notification_except_attrs"], str(a))
                else:
                    condition_attr = "%s_%s" % (subscription_context["notification_attrs"], str(a))
            else:
                if curs["blacklist"]:
                    condition_attr = subscription_context["notification_except_attrs"]
                else:
                    condition_attr = subscription_context["notification_attrs"]
            assert condition_attr in curs["attrs"], \
                u' ERROR - the notification attrs "%s" does not exist in attrs into DB' % condition_attr
        __logger__.info("attributes in the \"notification\" field are verified successfully")

        # status field
        if subscription_context["status"] is None:
            subscription_context["status"] = "active"
        assert subscription_context["status"] == curs["status"], " ERROR - the status \"%s\" does not match with the expected \"%s\"" \
                                                                 % (curs["status"], subscription_context["status"])
        __logger__.info("the subscription status is the expected: %s" % subscription_context["status"])

        # metadata field
        if subscription_context["notification_metadata"] is not None and subscription_context["notification_metadata"] != "array is empty":
            metadata = subscription_context["notification_metadata"].split(",")
            for item in metadata:
                assert item in curs["metadata"], \
                    " ERROR - the metadata \"%s\" does not exist in mongo metadata: %s" % (item, curs["metadata"])
            __logger__.info("metadata \"%s\" stored succesfully in mongo" % metadata)

    def verify_log_level(self, context, level):
        """
        verify if the log level is the expected
        :param level: log level expected
        """
        resp_dict = convert_str_to_dict(context.resp.text, JSON)
        assert resp_dict["level"] == level, " ERROR - the log level \"%s\" is not the expected: %s" % (resp_dict["level"], level)

    def verify_admin_error(self, context, error):
        """
        verify admin error message
        :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
        :param error: error message expected
        """
        msg = {}
        __logger__.debug("Response: %s" % error)

        resp_dict = convert_str_to_dict(context.resp.text, JSON)
        assert "error" in resp_dict, "ERROR - error field does not exists"
        assert error == resp_dict["error"], 'ERROR -  error: "%s" is not the expected: "%s"' % \
                                            (str(resp_dict["error"]), error)

    def __get_attribute_fields_to_notifications(self, entity_context, subsc_context):
        """
        get attribute names, values and types from entities and subscriptions to be used with notifications
        :param entity_context: entity context used
        :param subsc_context: subscription context used
        :returns lists (attribute names list, attribute values list, attribute types list)
        """
        # attribute field lists
        attrs_to_notif = []
        attr_values_list = []
        attr_types_list = []
        if entity_context["attributes_number"] > 1:
            for i in range(int(entity_context["attributes_number"])):
                attrs_to_notif.append("%s_%s" % (entity_context["attributes_name"], str(i)))
                attr_values_list.append(entity_context["attributes_value"])
                attr_types_list.append(self.__change_attribute_type(entity_context["attributes_value"], entity_context["attributes_type"]))
        else:
            if entity_context["attributes_name"].find("&") < 0:
                attrs_to_notif.append(entity_context["attributes_name"])
                attr_values_list.append(entity_context["attributes_value"])
                attr_types_list.append(self.__change_attribute_type(entity_context["attributes_value"], entity_context["attributes_type"]))
            else:
                attrs_to_notif = entity_context["attributes_name"].split("&")
                attr_values_list = entity_context["attributes_value"].split("&")
                while entity_context["attributes_type"].find("&&") >= 0:
                    entity_context["attributes_type"] = entity_context["attributes_type"].replace("&&", "&none&")
                attr_types_list = entity_context["attributes_type"].split("&")
                if len(attr_types_list) != len(attrs_to_notif):
                    diff = len(attrs_to_notif) - len(attr_types_list)
                    for i in range(diff):
                        attr_types_list.append(u'none')
                for i in range(len(attr_types_list)):
                    attr_types_list[i] = self.__change_attribute_type(attr_values_list[i], attr_types_list[i])
                    if attr_types_list[i] == "Boolean":
                        attr_values_list[i] = "%s%s" % (attr_values_list[i][0].upper(), attr_values_list[i][1:])

        if subsc_context["notification_attrs"] is not None:
            i = 0
            while i < len(attrs_to_notif):
                if attrs_to_notif[i] not in subsc_context["notification_attrs"]:
                    attrs_to_notif.pop(i)
                    attr_values_list.pop(i)
                    attr_types_list.pop(i)
                else:
                    i += 1
        elif subsc_context["notification_except_attrs"] is not None:
            except_attrs = []
            if subsc_context["notification_attrs_number"] == 1:
                except_attrs.append(subsc_context["notification_except_attrs"])
            else:
                for i in range(int(subsc_context["notification_attrs_number"])):
                    except_attrs.append("%s_%s" % (subsc_context["notification_except_attrs"], str(i)))

            for e_a in range(len(except_attrs)):
                if except_attrs[e_a] in attrs_to_notif:
                    p = attrs_to_notif.index(except_attrs[e_a])
                    attrs_to_notif.pop(p)
                    attr_values_list.pop(p)
                    attr_types_list.pop(p)
        for e in range(len(attrs_to_notif)):  # remove quote from raw request
            attrs_to_notif[e] = remove_quote(attrs_to_notif[e])
            attr_values_list[e] = remove_quote(attr_values_list[e])
            attr_types_list[e] = remove_quote(attr_types_list[e])
        return attrs_to_notif, attr_values_list, attr_types_list

    def __get_metadata_attribute_fields_to_notifications(self, entity_context):
        """
        get metadata attribute names, metadata values and metadata types from entities to be used with notifications
        :param entity_context: entity context used
        :returns lists (metadata attribute names list, metadata attribute values list, metadata attribute types list)
        """
        meta_names_list = []
        meta_values_list = []
        meta_types_list = []
        if entity_context["metadatas_number"] > 1:
            for i in range(int(entity_context["metadatas_number"])):
                meta_names_list.append("%s_%s" % (entity_context["metadatas_name"], str(i)))
                meta_values_list.append(entity_context["metadatas_value"])
                meta_types_list.append(self.__change_attribute_type(entity_context["metadatas_value"], entity_context["metadatas_type"]))
        else:
            if entity_context["metadatas_name"].find("&") < 0:
                meta_names_list.append(entity_context["metadatas_name"])
                meta_values_list.append(entity_context["metadatas_value"])
                meta_types_list.append(self.__change_attribute_type(entity_context["metadatas_value"], entity_context["metadatas_type"]))
            else:
                meta_names_list = entity_context["metadatas_name"].split("&")
                meta_values_list = entity_context["metadatas_value"].split("&")
                while entity_context["metadatas_type"].find("&&") >= 0:
                    entity_context["metadatas_type"] = entity_context["metadatas_type"].replace("&&", "&none&")
                meta_types_list = entity_context["metadatas_type"].split("&")
                if len(meta_types_list) != len(meta_names_list):
                    diff = len(meta_names_list) - len(meta_types_list)
                    for i in range(diff):
                        meta_types_list.append(u'Thing')
                for i in range(len(meta_types_list)):
                    meta_types_list[i] = self.__change_attribute_type(meta_values_list[i], meta_types_list[i])
                    if meta_types_list[i] == "Boolean":
                        meta_values_list[i] = "%s%s" % (meta_values_list[i][0].upper(), meta_values_list[i][1:])
        for e in range(len(meta_names_list)):  # remove quote from raw request
            meta_names_list[e] = remove_quote(meta_names_list[e])
            meta_values_list[e] = remove_quote(meta_values_list[e])
            meta_types_list[e] = remove_quote(meta_types_list[e])
        return meta_names_list, meta_values_list, meta_types_list

    def __change_attribute_type(self, attributes_value, attributes_type):
        """
        change the attribute type if it is not defined depending to the value type
        types by default (Text, Number, Boolean, None or StruturedValue)
        :param attributes_value: attribute value
        :param attributes_type: attribute type (changed if it is "none")
        :return: string (attributes_type)
        """
        attributes_type = remove_quote(attributes_type)
        if attributes_type == "none":
            if (attributes_value.find(u'{') >= 0) or (attributes_value.find(u'[') >= 0):
                attributes_type = '"StructuredValue'
            elif (attributes_value == "true") or (attributes_value == "false"):
                attributes_type = 'Boolean'
            elif (attributes_value == "null"):
                attributes_type = 'None'
            else:
                try:
                    numeric = True
                    float(attributes_value)
                except Exception, e:
                    numeric = False
                if numeric:
                    attributes_type = u'Number'
                else:
                    attributes_type = u'Text'
        return attributes_type

    def verify_notification(self, notif_format, payload, headers, entity_context, subsc_context):
        """
        verify the notification in a given format
        :param notif_format: format expected (normalized | keyValues | values | legacy)
        :param payload: payload received
        :param headers: headers received
        :param entity_context: entity context used
        :param subsc_context: subscription context used
        """
        payload_dict = convert_str_to_dict(payload, JSON)

        # no notification is received
        assert "msg" not in payload_dict, " ERROR - no notification is received: \n   - %s" % payload_dict["msg"]

        # subscriptionId field
        assert len(payload_dict["subscriptionId"]) == 24, " ERROR - subscriptionId does not has 24 chars"
        try:
            int(payload_dict["subscriptionId"], 16)  # validate if it is hexadecimal
        except Exception, e:
            __logger__.error(" ERROR: the subcriptionId is not an hexadecimal: %s \n  - %s" % (payload_dict["subscriptionId"], str(e)))
            raise Exception(" ERROR: the subcriptionId is not an hexadecimal: %s" % payload_dict["subscriptionId"])
        __logger__.info("  - subcriptionId field does has 24 chars and it is an hexadecimal")

        # get attribute fields lists (attribute names list, attribute values list, attribute types list)
        attrs_to_notif, attr_values_list, attr_types_list = self.__get_attribute_fields_to_notifications(entity_context, subsc_context)

        # legacy format
        if notif_format == "legacy":
            statusCode = payload_dict["contextResponses"][0]["statusCode"]
            # status code fields
            assert statusCode["code"] == "200", " ERROR - the status code is not the expected: %s" % statusCode["code"]
            assert statusCode["reasonPhrase"] == "OK", " ERROR - the reason phrase is not the expected: %s" % statusCode["reasonPhrase"]
            __logger__.info("  - The status code and the reason phrase is the expected correctly")

            # context Element fields
            contextElement = payload_dict["contextResponses"][0]["contextElement"]
            # id, type and idPattern fields
            assert contextElement["id"] == entity_context["entities_id"], " ERROR - the id is not the expected: %s" % entity_context[
                "entities_id"]
            __logger__.info("  - id matches succesfully")
            assert contextElement["isPattern"] == "false", " ERROR - the isPattern is not the expected: %s" % contextElement["isPattern"]
            __logger__.info("  - isPattern matches succesfully")
            assert contextElement["type"] == entity_context["entities_type"], " ERROR - the type is not the expected: %s" % entity_context[
                "entities_type"]
            __logger__.info("  - type matches succesfully")

            # Attributes fields
            attributes = contextElement["attributes"]
            for i in range(len(attributes)):
                assert attributes[i]["name"] in attrs_to_notif, " ERROR - the attribute name is not the expected: %s" % attributes[i][
                    "name"]
                __logger__.info("  - attribute name: %s  matches succesfully" % attributes[i]["name"])
                assert attributes[i]["type"] in attr_types_list, " ERROR - the attribute type is not the expected: %s" % attributes[i][
                    "type"]
                __logger__.info("  - attribute type: %s  matches succesfully" % attributes[i]["type"])
                assert attributes[i]["value"] in attr_values_list, " ERROR - the attribute type is not the expected: %s" % attributes[i][
                    "value"]
                __logger__.info("  - attribute value: %s  matches succesfully" % attributes[i]["value"])

        # another formats: normalized, keyValues, values
        else:
            data = payload_dict["data"][0]
            # "id" and "type" fields only are sent in normalized or keyValues formats (not in values format)
            if notif_format == "normalized" or notif_format == "keyValues":
                # entity id field
                entity_id = []
                entity_type = []
                if ("id" in entity_context["entities_prefix"]) and (entity_context["entities_prefix"]["id"] == "true"):
                    for i in range(int(entity_context["entities_number"])):
                        entity_id.append("%s_%s" % (entity_context["entities_id"], str(i)))
                else:
                    entity_id.append(entity_context["entities_id"])
                for e in range(len(entity_id)):  # remove quote from raw request
                    entity_id[e] = remove_quote(entity_id[e])
                assert data["id"] in entity_id, " ERROR - id %s does not include into %s" % (data["id"], str(entity_id))
                __logger__.info("  - id matches succesfully")

                # entity type field
                if entity_context["entities_type"] is not None:
                    if ("type" in entity_context["entities_prefix"]) and (entity_context["entities_prefix"]["type"] == "true"):
                        for i in range(int(entity_context["entities_number"])):
                            entity_type.append("%s_%s" % (entity_context["entities_type"], str(i)))
                    else:
                        entity_type.append(entity_context["entities_type"])
                    for e in range(len(entity_type)):  # remove quote from raw request
                        entity_type[e] = remove_quote(entity_type[e])
                    assert data["type"] in entity_type, " ERROR - type does not match %s != %s" % \
                                                        (data["type"], str(entity_type))
                    __logger__.info("  - type matches succesfully")

            # attributes fields
            if notif_format == "normalized" or notif_format == "keyValues":  # attribute names notified only in or
                __logger__.info("attributes to notify:")
                for a in attrs_to_notif:
                    __logger__.info("  - %s" % a)
            for i in range(len(attrs_to_notif)):
                # attribute names only are sent in normalized or keyValues formats (not in values format)
                attrs_to_notif[i] = remove_quote(attrs_to_notif[i])
                if notif_format == "normalized" or notif_format == "keyValues":
                    assert remove_quote(attrs_to_notif[i]) in data, " ERROR - the attribute: %s does not exist in the notification" % \
                                                                    attrs_to_notif[i]
                    __logger__.info(
                        "the attribute \"%s\" is sent in the notification in \"%s\" format with:" % (attrs_to_notif[i], notif_format))
                if notif_format == "normalized":
                    if isinstance(data[attrs_to_notif[i]]["value"], dict):
                        data[attrs_to_notif[i]]["value"] = convert_dict_to_str(data[attrs_to_notif[i]]["value"], JSON)
                    assert str(attr_values_list[i]) == str(
                        data[attrs_to_notif[i]]["value"]), " ERROR - the attribute value does not match in normalized format: \n   %s != %s" \
                                                           % (attr_values_list[i], data[attrs_to_notif[i]]["value"])
                    __logger__.info("  - the attribute value sent is correct: %s" % attr_values_list[i])
                    assert attr_types_list[i] == data[attrs_to_notif[i]][
                        "type"], " ERROR - the attribute type does not match in normalized format: \n   %s != %s" \
                                 % (attr_types_list[i], data[attrs_to_notif[i]]["type"])
                    __logger__.info("  - the attribute type sent is correct: %s" % attr_types_list[i])
                elif notif_format == "keyValues":
                    assert unicode(attr_values_list[i]) == data[
                        attrs_to_notif[i]], " ERROR - the attribute value does not match in keyValues format: \n   %s != %s" \
                                            % (attr_values_list[i], data[attrs_to_notif[i]])
                    __logger__.info("  - the attribute value sent is correct: %s" % entity_context["attributes_value"])
                elif notif_format == "values":
                    assert attr_values_list[i] == data[i], " ERROR - the attribute value does not match in values format: \n   %s != %s" \
                                                           % (attr_values_list[i], data[i])
                    __logger__.info("  - the attribute value sent is correct: %s" % attr_values_list[i])

    def __replacing_values(self, str_to_replace, **kwargs):
        """
        replacing values from subscription request
        :param str_to_replace: string to replace values
        :param id: entity_id
        :param type: entity_type
        :param attr_names_list: list of attr names
        :param attr_values_list: list of attr values
        :return: string
        """
        entity_id = kwargs.get("id", "")
        entity_type = kwargs.get("type", "")
        attr_names_list = kwargs.get("names", [])
        attr_values_list = kwargs.get("values", [])
        str_to_replace = str_to_replace.replace("${id}", remove_quote(entity_id))
        str_to_replace = str_to_replace.replace("${type}", remove_quote(entity_type))
        for a in range(len(attr_names_list)):
            str_to_replace = str_to_replace.replace("${%s}" % attr_names_list[a], remove_quote(attr_values_list[a]))
        # if some attribute does not exist, it is replace to empty string
        str_to_replace_split = str_to_replace.split("${")
        str_to_replace = str_to_replace_split[0]
        for i in range(1, len(str_to_replace_split)):
            pos = str_to_replace_split[i].find("}") + 1
            str_to_replace = "%s%s" % (str_to_replace, str_to_replace_split[i][pos:])
        return str_to_replace

    def verify_custom_notification(self, payload, headers, entity_context, subsc_context):
        """
        verify the custom notification (templates)
        :param payload: payload received
        :param headers: headers received
        :param entity_context: entity context used
        :param subsc_context: subscription context used
        """

        # get attribute fields lists (attribute names list, attribute values list, attribute types list)
        attrs_to_notif, attr_values_list, attr_types_list = self.__get_attribute_fields_to_notifications(entity_context, subsc_context)

        # custom notification
        payload_type = headers["last-content-type"]
        __logger__.debug("payload type: %s" % payload_type)
        if payload_type == "application/json":  # to review this if
            payload = payload
        else:
            payload = payload
        url = headers["last-url"]

        # replacing values in payload, url, qs and headers in request fields (id, type and attributes)
        # payload
        expected_payload = remove_quote(subsc_context["notification_http_custom_payload"]).replace("%22", "\"")
        expected_payload = self.__replacing_values(expected_payload,
                                                   id=entity_context["entities_id"],
                                                   type=entity_context["entities_type"],
                                                   names=attrs_to_notif,
                                                   values=attr_values_list)

        assert expected_payload == payload, " ERROR - the payload \"%s\" is not the expected: \"%s\"" % (
            payload, expected_payload)
        __logger__.debug("the payload is the expected: (%s)" % payload)

        # url
        expected_url = remove_quote(subsc_context["notification_http_custom_url"])
        expected_method = remove_quote(subsc_context["notification_http_custom_method"])
        expected_url = "%s - %s" % (expected_method, expected_url)
        expected_qs = convert_str_to_dict(subsc_context["notification_http_custom_qs"], "json")

        if expected_qs != {}:
            for item in expected_qs:
                expected_url = "%s&%s=%s" % (expected_url, item, expected_qs[item])
            expected_url = expected_url.replace("&", "?", 1)
        __logger__.debug("url expected: %s" % expected_url)
        expected_url = self.__replacing_values(expected_url,
                                               id=entity_context["entities_id"],
                                               type=entity_context["entities_type"],
                                               names=attrs_to_notif,
                                               values=attr_values_list)

        assert expected_url == url, " ERROR - the url \"%s\" is not the expected: \"%s\"" % (url, expected_url)
        __logger__.debug("the url is the expected: (%s)" % url)

        # headers
        expected_headers_dict = convert_str_to_dict(subsc_context["notification_http_custom_headers"], "json")
        for item in expected_headers_dict:
            expected_headers_dict[item] = self.__replacing_values(expected_headers_dict[item],
                                                                  id=entity_context["entities_id"],
                                                                  type=entity_context["entities_type"],
                                                                  names=attrs_to_notif,
                                                                  values=attr_values_list)
            assert "last-%s" % item in headers, " ERROR - the \"%s\" header does not sent" % item
            __logger__.debug("the \"%s\" header has been sent" % item)
            assert expected_headers_dict[item] == headers["last-%s" % item], \
                ' ERROR - the header value "%s" does not match with the expected "%s"' % (
                    headers["last-%s" % item], expected_headers_dict[item])
            __logger__.debug("the header value does match: %s" % expected_headers_dict[item])

    def __get_order_by(self, values):
        """
        get attribute without prefix and the order
        :param value: attribute (with/without "!" prefix)
        :return: attribute and order
        """
        if values[:1] == "!":
            order_by = "DESC"
            values = values[1:]
        else:
            order_by = "ASC"
        return values, order_by

    def __sort_attrs(self, entities, attrs_to_sort_list):
        """
        sort a entities list by attributes
        :param entities: entities list
        :param attrs_to_sort_list: attributes list to sort
        :return: list sorted
        """
        pos_attr = 0
        attrs_to_sort, order_by = self.__get_order_by(attrs_to_sort_list[pos_attr])
        for pos_entity in range(len(entities) - 1):
            for pos_entity_adv in range(pos_entity + 1, len(entities)):
                swap_items = False
                if attrs_to_sort in entities[pos_entity]["attrs"]:
                    if entities[pos_entity]["attrs"][attrs_to_sort] < entities[pos_entity_adv]["attrs"][attrs_to_sort]:
                        if order_by == "DESC":
                            swap_items = True
                    elif entities[pos_entity]["attrs"][attrs_to_sort] > entities[pos_entity_adv]["attrs"][attrs_to_sort]:
                        if order_by == "ASC":
                            swap_items = True
                    else:
                        while pos_attr < len(attrs_to_sort_list) and swap_items is not True:
                            pos_attr = pos_attr + 1
                            if pos_attr < len(attrs_to_sort_list):
                                attrs_to_sort_temp, order_by_temp = self.__get_order_by(attrs_to_sort_list[pos_attr])
                                if entities[pos_entity]["attrs"][attrs_to_sort_temp] < entities[pos_entity_adv]["attrs"][
                                    attrs_to_sort_temp]:
                                    if order_by_temp == "DESC":
                                        swap_items = True
                                elif entities[pos_entity]["attrs"][attrs_to_sort_temp] > entities[pos_entity_adv]["attrs"][
                                    attrs_to_sort_temp]:
                                    if order_by_temp == "ASC":
                                        swap_items = True
                    if swap_items:
                        entities = list_swap(entities, pos_entity_adv, pos_entity)
        return entities

    def __get_entities_sorted_by_attributes(self, queries_parameters, accumulate_entities_context, sort):
        """
        get an entitie dict sorted by attributes
        :param queries_parameters: queries params dictionary
        :param accumulate_entities_context: accumulate of all entities context. See "entity_context" dict in cb_v2_utils.py
        :param sort: determine if the attributes are sorted or not (by default is True)
        :return: attributes to sort list and entities dict sorted
        """
        entities = []
        if "orderBy" in queries_parameters:
            attrs_to_sort_list = queries_parameters["orderBy"].split(",")
        __logger__.debug("Attrs used to sort: %s" % (attrs_to_sort_list))
        for ent in accumulate_entities_context:
            attrs_name_temp = ent["attributes_name"].split("&")
            attrs_value_temp = ent["attributes_value"].split("&")
            attrs = {}
            for i in range(len(attrs_name_temp)):
                name = remove_quote(attrs_name_temp[i])
                value = attrs_value_temp[i]
                if value.find("true") == 0 or value.find("false") == 0:
                    attrs[name] = bool(convert_str_to_bool(value))
                elif (attrs_value_temp[i].find(u'{') >= 0) or (value.find(u'[') >= 0):
                    attrs[name] = convert_str_to_dict(value, JSON)
                else:
                    try:
                        temp = float(value)
                        numeric = True
                    except Exception:
                        numeric = False
                    if numeric:
                        if temp.is_integer():
                            attrs[name] = int(temp)
                        else:
                            attrs[name] = temp
                    else:
                        attrs[name] = remove_quote(value)
            entities.append({"id": remove_quote(ent["entities_id"]),
                             "type": remove_quote(ent["entities_type"]),
                             "attrs": attrs})
        __logger__.debug("attributes to sort: %s" % str(attrs_to_sort_list))
        __logger__.debug("entities unsorted: %s" % str(entities))
        # Sorting
        if sort:
            entities = self.__sort_attrs(entities, attrs_to_sort_list)
            __logger__.debug("entities sorted: %s" % str(entities))

        if "attrs" in queries_parameters:
            attrs_to_show_list = queries_parameters["attrs"].split(",")
            for attr in attrs_to_sort_list:
                if attr not in attrs_to_show_list:
                    attrs_to_sort_list.remove(attr)

        return attrs_to_sort_list, entities

    def verify_that_entities_are_sorted_by_some_attributes(self, queries_parameters, accumulate_entities_context, resp, sort=True):
        """
        verify that entities are sorted by some attributes
        :param queries_parameters: queries params dictionary
        :param accumulate_entities_context: accumulate of all entities context. See "entity_context" dict in cb_v2_utils.py
        :param resp: http response
        :param sort: determine if the attributes are sorted or not (by default is True)
        """
        attrs_to_sort_list, entities = self.__get_entities_sorted_by_attributes(queries_parameters, accumulate_entities_context, sort)
        resp_dict = convert_str_to_dict(resp.content, "json")

        # verifying id and type
        for pos in range(len(resp_dict)):
            assert resp_dict[pos]["id"] == entities[pos]["id"], ' ERROR - the id "%s" does not match with "%s"' % (resp_dict[pos]["id"],
                                                                                                                   entities[pos]["id"])
            __logger__.info('the id "%s" is sorted correctly' % resp_dict[pos]["id"])
            assert resp_dict[pos]["type"] == entities[pos]["type"], ' ERROR - the type "%s" does not match with "%s"' % (resp_dict[pos]["type"],
                                                                                                                         entities[pos]["type"])
            __logger__.info('the type "%s" is sorted correctly' % resp_dict[pos]["type"])

        # verifying attributes
        for pos_attr in range(len(attrs_to_sort_list)):
            for pos_ent in range(len(resp_dict)):
                attrs_to_sort, order_by = self.__get_order_by(attrs_to_sort_list[pos_attr])
                assert attrs_to_sort in resp_dict[pos_ent], " ERROR - the attribute \"%s\" does not exist..." % attrs_to_sort
                entity = entities[pos_ent]
                if order_by == "DESC":
                    assert entity["attrs"][attrs_to_sort] == resp_dict[pos_ent][attrs_to_sort]["value"], \
                    " ERROR - the attribute \"%s\" is lower than the value \"%s\" with \"%s\"" % (
                    attrs_to_sort, resp_dict[pos_ent][attrs_to_sort]["value"], entity["attrs"][attrs_to_sort])
                else:
                    assert entity["attrs"][attrs_to_sort] == resp_dict[pos_ent][attrs_to_sort]["value"], \
                    " ERROR - the attribute \"%s\" is greater than the value \"%s\" with \"%s\"" % (
                    attrs_to_sort, resp_dict[pos_ent][attrs_to_sort]["value"], entity["attrs"][attrs_to_sort])
                __logger__.info('the attribute "%s" has it value "%s" and it is sorted correctly' %
                            (attrs_to_sort, str(resp_dict[pos_ent][attrs_to_sort]["value"])))

    def verify_metadata_notification(self, metadata_flags, payload, entity_context, subsc_context, action_type, previous_value):
        """
        verify metadata in the notification (custom user or/and specials)
        :param metadata_flags: metadata notified
        :param payload: notification payload
        :param entity_context: entity properties
        :param subsc_context: subscription properties
        :param action_type: action in entity (append | update |delete)
        :param previous_value: previous value before update (previous values and type)
        """
        __logger__.debug("action type: %s" % action_type)
        __logger__.debug("previous value: %s" % str(previous_value))
        special_meta = metadata_flags.split(",")
        payload_dict = convert_str_to_dict(payload, JSON)
        __logger__.debug("special metadata: %s" % str(special_meta))

        attrs_to_notif, attr_values_list, attr_types_list = self.__get_attribute_fields_to_notifications(entity_context, subsc_context)
        __logger__.debug("attributes_notified: %s " % str(attrs_to_notif))
        for attrs_name in attrs_to_notif:    # number of attributes notified
            metadata = payload_dict["data"][0][attrs_name]["metadata"]
            meta_names_list, meta_values_list, meta_types_list = self.__get_metadata_attribute_fields_to_notifications(entity_context)
            for i in range (len(meta_names_list)):  # number of ametadata attribute notified
                for meta_flag in special_meta:      # number of metadata flags
                    # previousValue metadata verification
                    if meta_flag == "previousValue" and previous_value["value"] is not None and previous_value["name"] == meta_names_list[i]:
                        assert "previousValue" in metadata, " ERROR - the previousValue does not exist im metadata: %s" % str (metadata)
                        assert metadata["previousValue"]["type"] == previous_value["type"], \
                            " ERROR - the previousValue type is not Text: %s" % previous_value["type"]
                        assert metadata["previousValue"]["value"] == previous_value["value"],\
                            " ERROR - the previousValue value is not the expected: %s" % previous_value["value"]
                        __logger__.info(" the previousValue metadata is the expected with type: %s and value:%s" % (previous_value["type"], previous_value["value"]))
                    # actionType metadata verification
                    if meta_flag == "actionType":
                        assert "actionType" in metadata, " ERROR - the actionType does not exist im metadata"
                        assert metadata["actionType"]["type"] == "Text", " ERROR - the actionType type is not Text: %s" % metadata["actionType"]["type"]
                        assert metadata["actionType"]["value"] == action_type, " ERROR - the actionType value is not the expected: %s" % action_type
                        __logger__.info(" the actionType metadata is the expected: %s" % action_type)
                    # custom user metadata verification
                    if meta_flag == "*":
                        meta_name = meta_names_list[i]
                        assert meta_name in metadata, " ERROR - the metadata \"%s\" does not exist" % meta_name
                        assert metadata[meta_name]["type"] == meta_types_list[i], \
                            " ERROR - the type in metadata \"%s\" does not match: \n %s != %s" % \
                            (meta_name, metadata[meta_name]["type"], meta_types_list[i])
                        assert metadata[meta_name]["value"] == meta_values_list[i], \
                            " ERROR - the value in metadata \"%s\" does not match: \n %s != %s" % \
                            (meta_name, metadata[meta_name]["value"], meta_values_list[i])
                        __logger__.info("In Attribute \"%s\", the metadata \"%s\" exists and the value and the type match successfully..." % (attrs_name, meta_name))
