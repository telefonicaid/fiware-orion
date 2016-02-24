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
import copy
import re

__author__ = 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

from iotqatools.helpers_utils import *

# constants
EMPTY = u''
FIWARE_SERVICE_HEADER = u'Fiware-Service'
FIWARE_SERVICE_PATH_HEADER = u'Fiware-ServicePath'
ORION_PREFIX = u'orion'
PARAMETER = u'parameter'
VALUE = u'value'

__logger__ = logging.getLogger("utils")


class NGSI:
    """
    manage Context broker operations
    """
    def __init__(self,  **kwargs):
        """
        constructor
        """
    # ------------------------------------ validations ------------------------------------------
    def __get_mongo_cursor(self, mongo_driver, entities_contexts, headers):
        """
        return a cursor with a entities list
        :param entities_contexts: entities context (attr_name, attr_value, attr_type, id, type, metadatas, etc)
        :param headers: headers dict (service, service_path, etc)
        :param mongo_driver: mongo driver from steps
        :return: list
        """
        if FIWARE_SERVICE_HEADER in headers:
            __logger__.debug("service header: %s" % headers[FIWARE_SERVICE_HEADER])
            if headers[FIWARE_SERVICE_HEADER] == EMPTY:
                database_name = ORION_PREFIX
            else:
                database_name = "%s-%s" % (ORION_PREFIX, headers[FIWARE_SERVICE_HEADER])
        else:
            database_name = ORION_PREFIX
        # Orion Context Broker interprets the tentant name (service) in lowercase, thus,
        # although you can use tenants such as in updateContext "MyService" it is not advisable
        mongo_driver.connect(database_name.lower())
        mongo_driver.choice_collection("entities")

        if FIWARE_SERVICE_PATH_HEADER in headers:
            if headers[FIWARE_SERVICE_PATH_HEADER] == EMPTY:
                service_path = "/"
            else:
                service_path = headers[FIWARE_SERVICE_PATH_HEADER]
        else:
            service_path = "/"
        if "entities_type" in entities_contexts:
            type = entities_contexts["entities_type"]
        else:
            type = ""
        __logger__.info("Database verified: %s" % database_name)
        __logger__.info("collection verified: entities")
        __logger__.info("service path verified: %s" % service_path)
        curs_list = []
        if type is not None:
            query = {"_id.id": {'$regex': '%s.*' % entities_contexts["entities_id"]}, "_id.servicePath": service_path,
                     "_id.type": type}
        else:
            query = {"_id.id": {'$regex': '%s.*' % entities_contexts["entities_id"]}, "_id.servicePath": service_path}
        __logger__.info("query: %s" % str(query))

        curs = mongo_driver.find_data(query)
        curs_list = mongo_driver.get_cursor_value(curs)
        __logger__.debug("documents stored in mongo:  %s" % str(curs_list))
        return curs_list

    def __verify_attributes(self, entity, entities_contexts):
        """
        verify attributes from mongo
        """
        # verify attributes
        for a in range(int(entities_contexts["attributes_number"])):    # manages N attributes
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
                if entities_contexts["attributes_type"] is not None:
                    assert entities_contexts["attributes_type"] == entity[u'attrs'][attr_name][u'type'], \
                    " ERROR -- attribute type: %s is not stored successful in mongo" % entities_contexts["attributes_type"]
                # verify metadatas
                if entities_contexts["metadatas_number"] > 0:
                    md = entity[u'attrs'][attr_name][u'md']
                    for m in range(int(entities_contexts["metadatas_number"])):    # manages N metadatas
                        assert entities_contexts["metadatas_value"] == md[m][u'value'], \
                            " ERROR -- metadata value: %s is not stored successful in mongo" % entities_contexts["metadatas_value"]
                        if entities_contexts["metadatas_type"] is not None:

                            assert entities_contexts["metadatas_type"] == md[m][u'type'], \
                                " ERROR -- metadata type: %s is not stored successful in mongo" % entities_contexts["metadatas_type"]
                assert u'creDate' in entity[u'attrs'][attr_name],\
                    " ERROR -- creDate field into attribute does not exists in document"
                assert u'modDate' in entity[u'attrs'][attr_name],\
                    " ERROR -- modDate field into attribute does not exists in document"

            assert u'creDate' in entity,\
                " ERROR -- creDate field does not exists in document"
            assert u'modDate' in entity,\
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
                                "type": "date",
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
        curs_list = self.__get_mongo_cursor(mongo_driver, entities_contexts, headers)

        # verify entities
        for i in range(int(entities_contexts["entities_number"])):
            # verify if the entity is stored in mongo
            if stored:
                __logger__.debug("Number of doc read from mongo: %s" % str(i+1))
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
        mongo_driver.disconnect()

    def verify_entity_updated_in_mongo(self, mongo_driver, entities_contexts, headers):
        """
        verify that entities are not stored in mongo
        :param entities_contexts:  entities context (see constructor in cb_v2_utils.py)
        :param headers: headers used (see "definition_headers" method in cb_v2_utils.py)
        """
        entity_list = self.__get_mongo_cursor(mongo_driver, entities_contexts, headers)
        assert len(entity_list) > 0, " ERROR - notihing is returned from mongo, review the query in behave log"
        entity = entity_list[0]
        # verify attributes
        self.__verify_attributes(entity, entities_contexts)
        mongo_driver.disconnect()

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
        has_quote = True

        while has_quote:
            quote = value[posi:].find("\'")
            if quote < 0:
                has_quote = False
            else:
                posi = posi + quote + 1
                if posi >= 0:
                    posf = value[posi:].find("'")
                    comma = value[posi:posf+posi].find(",")
                    if comma >= 0:
                        comma += posi
                        value = "%s%s%s" % (value[:comma], text_to_comma_replace, value[comma+1:])
                posi = posi+posf + len(text_to_comma_replace) + 1
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
                    if op == "+" and entity_context["entities_type"] is not None:
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
                    else:   # The value is a single element
                        items = self.__get_simple_quote_values(value[1])

                    if items[0] == "range":
                        try:
                            if op == "==" and ((float(entity_context["attributes_value"]) >= float(items[1][0])) and
                                              (float(entity_context["attributes_value"]) <= float(items[1][1]))):
                                    result = "this statement does match"  # range q=attr==x..y
                            elif op == "!=" and ((float(entity_context["attributes_value"]) <= float(items[1][0])) or \
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
                            else:     # OR operation
                                if eval:
                                    result = "this statement does match"
                                    break

            if result is None:
                    break
        return result

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
        elif limit+offset > total:
            items = total-offset
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
                    if items_list[i]["type"] is not None:
                        for tp in types:
                            if tp is not None:
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
                    except Exception, e:
                        __logger__.warn(" WARN - the attribute value is not verified if it is digit: \n   - %s" % e)
                    if not options_key_values:
                        if sub_group["attributes_type"] is not None:
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

                                assert meta_name in attribute["metadata"],  \
                                    'ERROR - attribute metadata name "%s" does not exist' % meta_name
                                metadata = attribute["metadata"][meta_name]
                                if sub_group["metadatas_type"] is not None:
                                    assert sub_group["metadatas_type"] == metadata["type"], \
                                        'ERROR - in attribute metadata type "%s"' % sub_group["metadatas_type"]
                                if sub_group["metadatas_value"] is not None:
                                    assert sub_group["metadatas_value"] == metadata["value"], \
                                        'ERROR - in attribute metadata value "%s"' % sub_group["metadatas_value"]

    def verify_headers_response(self, context):
        """
        verify headers in response
        Ex:
          | parameter      | value      |
          | x-total-counts | 5          |
        :param context: It’s a clever place where you and behave can store information to share around. It runs at three levels, automatically managed by behave.
        """
        headers= {}                                # headers to verify
        for row in context.table:
            headers[row[PARAMETER]] = row[VALUE]
        headers_list = dict(context.resp.headers)  # headers returned in response
        for h in headers:
            assert h in headers_list, " ERROR, %s header does no exist in headers response\n -- %s" % (h, str(headers_list))
            assert headers[h] == headers_list[h], " ERROR - %s = %s not matches with headers response: %s" % \
                                                  (h, headers[h], headers_list[h])

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
        entity = convert_str_to_dict(resp.text, JSON)[0]   # in raw mode is used only one entity
        assert remove_quote(entities_context["entities_id"]) == entity["id"],  \
            'ERROR - in id "%s" in raw mode' % (entity["id"])
        assert remove_quote(entities_context["entities_type"]) == entity["type"],  \
            'ERROR - in type "%s" in raw mode' % (entity["type"])

        # verify attribute fields
        assert remove_quote(entities_context["attributes_name"]) in entity,   \
            'ERROR - attribute name "%s" does not exist in raw mode' % entities_context["attributes_name"]
        attribute = entity[remove_quote(entities_context["attributes_name"])]
        assert remove_quote(entities_context["attributes_type"]) == attribute["type"],\
            'ERROR - in attribute type "%s" in raw mode' % remove_quote(entities_context["attributes_type"])

        if str(type(attribute["value"])) == "<type 'float'>":
            if attribute["value"] % 1 == 0:
                attribute["value"] = int(attribute["value"])
        if str(type(attribute["value"])) == "<type 'unicode'>":
            attribute["value"] = str(attribute["value"])

        assert str(type(attribute["value"])) == "<type '%s'>" % field_type,  \
            'ERROR - in attribute value "%s" with type "%s" does not match' % (str(attribute["value"]), field_type)

    def verify_an_entity_by_id(self, queries_parameters, entities_context, resp, entity_id_to_test):
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
        attrs_list = []
        options_key_values = False
        resp_json = convert_str_to_dict(resp.content, JSON)
        __logger__.debug("query parameter: %s" % str(queries_parameters))
        assert resp_json["id"] == entity_id_to_test,  'ERROR - in id "%s"' % resp_json["id"]
        if entities_context["entities_type"] is not None:
            assert resp_json["type"] == entities_context["entities_type"],  'ERROR - in type "%s" ' % resp_json["type"]

        #  attr query parameter
        if "attrs" in queries_parameters:
            attrs_list = queries_parameters["attrs"].split(",")
        else:
            if int(entities_context["attributes_number"]) == 1:
                attrs_list.append(entities_context["attributes_name"])
            else:
                for e in range(int(entities_context["attributes_number"])):
                    attrs_list.append("%s_%s" % (entities_context["attributes_name"], str(e)))
        if  "options" in queries_parameters:
            if queries_parameters["options"] == "keyValues":
                options_key_values = True

         # verify attributes
        for i in range(len(attrs_list)):
            attr_name = attrs_list[i]
            __logger__.info("attribute name: %s" % attr_name)
            assert attr_name in resp_json, 'ERROR - in attribute name "%s" ' % attr_name
            attribute = resp_json[attr_name]
            if not options_key_values:
                if entities_context["attributes_type"] is not None:
                    assert entities_context["attributes_type"] == attribute["type"], \
                        'ERROR - in attribute type "%s" without keyValues option' % entities_context["attributes_type"]
                if entities_context["attributes_value"] is not None:
                    assert entities_context["attributes_value"] == attribute["value"], \
                        'ERROR - in attribute value "%s" without keyValues option' % entities_context["attributes_value"]
               # verify attribute metadatas
                if entities_context["metadatas_number"] > 0:
                    for m in range(int(entities_context["metadatas_number"])):
                        if int(entities_context["metadatas_number"]) == 1:
                            meta_name = entities_context["metadatas_name"]
                        else:
                            meta_name = "%s_%s" % (entities_context["metadatas_name"], str(m))
                        assert meta_name in attribute["metadata"],\
                            'ERROR - attribute metadata name "%s" does not exist without keyValues option' \
                            % meta_name
                        metadata = attribute["metadata"][meta_name]
                        if entities_context["metadatas_type"] is not None:
                            assert entities_context["metadatas_type"] == metadata["type"], \
                                'ERROR - in attribute metadata type "%s" without keyValues option' % entities_context["metadatas_type"]
                        if entities_context["metadatas_value"] is not None:
                            assert entities_context["metadatas_value"] == metadata["value"], \
                                'ERROR - in attribute metadata value "%s" without keyValues option' % entities_context["metadatas_value"]
            else:
                if entities_context["attributes_value"] is not None:
                    assert entities_context["attributes_value"] == attribute, \
                        'ERROR - in attribute value "%s" with keyValues option' % entities_context["attributes_value"]



    def verify_an_attribute_by_id(self, entities_context, resp, attribute_name_to_request):
        """
        verify that the attribute by ID is returned
        :param entities_context:
        :param resp:
        :param attribute_name_to_request:
        ex:
            {
                timestamp: {
                    type: "date"
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
        if entities_context["attributes_type"] is not None:
            assert entities_context["attributes_type"] == attribute["type"],\
                'ERROR - in attribute type "%s"' % (entities_context["attributes_type"])
        if entities_context["attributes_value"] is not None:
            assert entities_context["attributes_value"] == attribute["value"],\
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
                if entities_context["metadatas_type"] is not None:
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
        attribute = convert_str_to_dict(resp.text, JSON)   # in raw mode is used only one entity
        __logger__.debug("attribute: %s" % str(attribute))

        if entities_context["attributes_type"] is not None:
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
            {
                value: "017-06-17T07:21:24.238Z"
            }
        """
        resp_json = convert_str_to_dict(resp.content, JSON)
        assert "value" in resp_json, 'ERROR - value key dos not exist in response: \n %s' %  resp.content
        assert resp_json["value"] == entities_context["attributes_value"], "ERROR - the value %s is not the expected: %s" % (resp_json["value"], entities_context["attributes_value"])

    def verify_attribute_is_deleted(self, mongo_driver, entities_contexts, headers):
        """
        verify if the attribute has been deleted
        :param entity_context:
        :param headers:
        """
        curs_list = self.__get_mongo_cursor(mongo_driver, entities_contexts, headers)

        # verify entities
        __logger__.debug("number of entities: %s" % str(entities_contexts["entities_number"]))
        for i in range(int(entities_contexts["entities_number"])):
            # verify if the entity is stored in mongo
            __logger__.debug("Number of docs read from mongo: %s" % str(len(curs_list)))
            __logger__.debug("attribute name to verify: %s" % entities_contexts["attributes_name"])
            assert i < len(curs_list), " ERROR - the entity \"%s\" is not stored" % str(i)
            entity = curs_list[i]  # manages N entities
            assert entities_contexts["attributes_name"] not in entity["attrNames"], " ERROR - the attribute: %s exists in the entity No: %s" % \
                                                                                    (entities_contexts["attributes_name"], str(i+1))
        mongo_driver.disconnect()

    def verify_entity_types(self, types, resp):
        """
        verify entity types  -- /v2/types
        :param queries_parameters: queries parameters used in the request
        :param accumulate_entities_context: accumulate of all entities context. See "entity_context" dict in cb_v2_utils.py
        :param resp: http response
        """
        # verify entities types
        type_list = types.split(",")
        items_dict = convert_str_to_dict(resp.content, JSON)
        items_list = items_dict.keys()  # list of keys
        for item in items_list:
            if item == EMPTY:
                item = "untyped"
            __logger__.debug("verified: %s include in %s" % (item, types))
            assert item in type_list, " ERROR - \"%s\" type does not match with types to verify" % item

    def verify_attributes_types_with_entity_types(self, queries_parameters, accumulate_entities_context, resp):
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
        count_total = 0

        for entities_group in accumulate_entities_context:
            temp = remove_quote(entities_group["entities_type"])
            if temp not in types_count:  # avoid types duplicated
                types_count[temp] = int(entities_group["entities_number"])
                total += 1
            else:
                types_count[temp] += int(entities_group["entities_number"])  # entities counter

        if "limit" in queries_parameters:
            limit = int(queries_parameters["limit"])
        if "offset" in queries_parameters:
            offset = int(queries_parameters["offset"])
        if "options" in queries_parameters:
            if queries_parameters["options"] == "count":
                count = int(resp.headers["x-total-count"])

            if queries_parameters["options"] == "values":
                pass  # not implemented (develop team) and not tested yet (pending)

        # determinate number of items and position in response
        if offset >= total:
            items = 0
            position = 0
        elif limit+offset > total:
            items = total-offset
            position = offset
        else:
            items = limit
            position = offset

        #  info in log
        __logger__.debug("total:  %s" % str(total))
        __logger__.debug("limit:  %s" % str(limit))
        __logger__.debug("offset: %s" % str(offset))
        __logger__.debug("items:  %s" % str(items))
        __logger__.debug("pos:    %s" % str(position))
        __logger__.debug("types count:  ")
        for i in types_count:
            __logger__.debug("   %s = %s" % (i, str(types_count[i])))

        # verify entities number
        items_dict = convert_str_to_dict(resp.content, JSON)
        items_list = items_dict.keys()  # list of keys
        assert items == len(items_list), " ERROR - the entities group total: %d is not the expected:%d" % (len(items_list), items)
        for item in items_list:
            attr_exists = False
            type_resp = item
            if item == EMPTY:
                type_resp = None

            assert type_resp in types_count, " ERROR - wrong entity type: %s" % item  # verify that the entities was created
            count_total += int(items_dict[item]["count"])

            assert types_count[type_resp] == int(items_dict[item]["count"]), \
                " ERROR - the entities type counter %s does not match with the expected: %s" % \
                (items_dict[item]["count"], types_count[type_resp])

            attr_list = items_dict[item]["attrs"].keys()       # list of attributes in each entities group
            for attr in attr_list:
                for entities_group in accumulate_entities_context:
                    if (entities_group["entities_type"] == type_resp) and (attr.find(entities_group["attributes_name"]) >= 0):
                        if entities_group["attributes_type"] == items_dict[item]["attrs"][attr]["type"]:
                            attr_exists = True
                            break
            assert attr_exists, ' ERROR - the attribute: "%s" with type: "%s" does not exist...' % (attr, items_dict[item]["attrs"][attr]["type"])

        # options=count query parameter
        if "options" in queries_parameters:
            if queries_parameters["options"] == "count":
                count = int(resp.headers["x-total-count"])
                assert count == count_total, " ERROR - the x-total-count header: %d does not match with the expected total: %d" \
                                             % (count, count_total)

