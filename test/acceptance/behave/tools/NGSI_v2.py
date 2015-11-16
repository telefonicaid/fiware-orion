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
        '''
        if operation.lower() == "create":
            query = {"_id.id": {'$regex': '%s.*' % entities_contexts["entities_id"]}, "_id.type": type,
                     "_id.servicePath": service_path}
        elif operation.lower() == "update":
            query = {"_id.id": {'$regex': '%s.*' % entities_contexts["entities_id"]}, "_id.servicePath": service_path}
        '''
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
                    __logger__.debug("attribute name: %s is changed by \"=\" because \".\" is forbidden in MongoDB keys" % attr_name)

                assert attr_name in entity[u'attrs'], \
                    " ERROR -- attribute: %s is not stored in mongo" % attr_name
                assert entities_contexts["attributes_value"] == entity[u'attrs'][attr_name][u'value'], \
                    " ERROR -- attribute value: %s is not stored successful in mongo" % str(entities_contexts["attributes_value"])
                if entities_contexts["attributes_type"] is None:
                    entities_contexts["attributes_type"] = EMPTY
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
        __logger__.debug(" Entity id: \"%s\" is successful stored in mongo" % entities_contexts["entities_id"])


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
        __logger__.debug("number of entities: %s" % str(entities_contexts["entities_number"]))
        for i in range(int(entities_contexts["entities_number"])):
            # verify if the entity is stored in mongo
            if stored:
                __logger__.debug("Number of docs read from mongo: %s" % str(len(curs_list)))
                assert i < len(curs_list), " ERROR - the entity \"%s\" is not stored" % str(i)
                entity = curs_list[i]  # manages N entities
                # verify attributes
                self.__verify_attributes(entity, entities_contexts)
            # verify if the entity is not stored in mongo
            else:
                assert len(curs_list) == 0, "  ERROR - the entities should not have been saved in mongo"
                __logger__.debug(" Entity id: \"%s\" is not stored in mongo as expected" % entities_contexts["entities_id"])
        mongo_driver.disconnect()

    def verify_entity_updated_in_mongo(self, mongo_driver, entities_contexts, headers):
        """
        verify that entities are not stored in mongo
        :param entities_contexts:  entities context (see constructor in cb_v2_utils.py)
        :param headers: headers used (see "definition_headers" method in cb_v2_utils.py)
        """
        entity_list = self.__get_mongo_cursor(mongo_driver, entities_contexts, headers)
        assert  len(entity_list) > 0, " ERROR - notihing is returned from mongo, review the query in behave log"
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

    def verify_get_all_entities(self, queries_parameters, entities_context, resp):
        """
        verify get all entities
        :param queries_parameters: queries parameters used
        :param entities_context: context values
        :param resp: http response
        """
        total = int(entities_context["entities_number"])
        if "limit" in queries_parameters:
            limit = int(queries_parameters["limit"])
        else:
            limit = 10000000000
        if "offset" in queries_parameters:
            offset = int(queries_parameters["offset"])
        else:
            offset = 0
        # determinate number od items and position in response
        if offset >= total:
            items = 0
            position = 0
        elif limit+offset > total:
            items = total-offset
            position = offset
        else:
            items = limit
            position = offset
        __logger__.debug("total:  %s" % str(total))
        __logger__.debug("limit:  %s" % str(limit))
        __logger__.debug("offset: %s" % str(offset))
        __logger__.debug("items:  %s" % str(items))
        __logger__.debug("pos:    %s" % str(position))

        # verify entities
        pos = position
        items_list = convert_str_to_dict(resp.text, JSON)
        assert len(items_list) == items, "ERROR - in number of items in response\n  received: %s \n  expected: %s" % \
                                         (str(len(items_list)), str(items))
        for i in range(items):
            assert "%s_%s" % (entities_context["entities_id"], str(pos)) == items_list[i]["id"], \
                'ERROR - in id "%s" in position %s' % (items_list[i]["id"], str(pos))
            if "entities_type" in entities_context:
                assert "%s" % (entities_context["entities_type"]) == items_list[i]["type"], \
                    'ERROR - in type "%s" in position %s' % (items_list[i]["type"], str(pos))
            # verify attributes
            for a in range(int(entities_context["attributes_number"])):
                if int(entities_context["attributes_number"]) == 1:
                    attr_name = entities_context["attributes_name"]
                else:
                    # prefix plus _<consecutive>. ex: room1_2
                    attr_name = "%s_%s" % (entities_context["attributes_name"], str(a))
                assert attr_name in items_list[i], \
                    'ERROR - attribute name "%s" does not exist in position:%s' % (attr_name, str(i))
                attribute = items_list[i][attr_name]
                if entities_context["attributes_type"] is not None:
                    assert entities_context["attributes_type"] == attribute["type"], \
                        'ERROR - in attribute type "%s" in position: %s' % (entities_context["attributes_type"], str(i))
                    assert entities_context["attributes_value"] == attribute["value"], \
                        'ERROR - in attribute value "%s" in position: %s' % (entities_context["attributes_value"], str(i))
                else:
                    if entities_context["metadatas_number"] > 0:
                        assert entities_context["attributes_value"] == attribute["value"], \
                            'ERROR - in attribute value "%s" in position: %s without attribute type nor metadatas' % (entities_context["attributes_value"], str(i))
                    else:
                        assert entities_context["attributes_value"] == attribute, \
                            'ERROR - in attribute value "%s" in position: %s without attribute type but with metadatas' % (entities_context["attributes_value"], str(i))
                # verify attribute metadatas
                if entities_context["metadatas_number"] > 0:
                    for m in range(int(entities_context["metadatas_number"])):
                        assert "%s_%s" % (entities_context["metadatas_name"], str(m)) in attribute,\
                            'ERROR - attribute metadata name "%s_%s" does not exist in position:%s' \
                            % (entities_context["metadatas_name"], str(a), str(m))
                        metadata = attribute["%s_%s" % (entities_context["metadatas_name"], str(m))]
                        if entities_context["metadatas_type"] is not None:
                            assert entities_context["metadatas_type"] == metadata["type"], \
                                'ERROR - in attribute metadata type "%s" in position: %s' % (entities_context["metadatas_type"],
                                                                                             str(i))
                            assert entities_context["metadatas_value"] == metadata["value"], \
                                'ERROR - in attribute metadata value "%s" in position: %s' % (entities_context["metadatas_value"],
                                                                                              str(i))
                        else:
                            assert entities_context["metadatas_value"] == metadata, \
                                'ERROR - in attribute metadata value "%s" in position: %s' % (entities_context["metadatas_value"], str(i))
            pos += 1

    @staticmethod
    def __remove_quote(text):
        """
        remove first and last characters if they are quote
        :param text:
        :return: string
        """
        text = text.lstrip('"')
        return text.rstrip('"')

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
        assert self.__remove_quote(entities_context["entities_id"]) == entity["id"],  \
            'ERROR - in id "%s" in raw mode' % (entity["id"])
        if "entities_type" in entities_context:
            assert self.__remove_quote(entities_context["entities_type"]) == entity["type"], \
                'ERROR - in type "%s" in raw mode' % (entity["type"])
        # verify attribute
        assert self.__remove_quote(entities_context["attributes_name"]) in entity,   \
            'ERROR - attribute name "%s" does not exist in raw mode' % entities_context["attributes_name"]
        attribute = entity[self.__remove_quote(entities_context["attributes_name"])]
        __logger__.debug("attribute: %s" % str(attribute))
        if entities_context["attributes_type"] is not None:
            assert self.__remove_quote(entities_context["attributes_type"]) == attribute["type"], \
                'ERROR - in attribute type "%s" in raw mode' % self.__remove_quote(entities_context["attributes_type"])

            attribute["value"] = self.__verify_if_is_int_or_str(attribute["value"])
            __logger__.debug("type:  %s" % field_type)
            __logger__.debug('field type: %s' % str(type(attribute)))
            assert str(type(attribute["value"])) == "<type '%s'>" % field_type, \
                'ERROR - in attribute value "%s" with type "%s" does not match' % (str(entities_context["attributes_value"]), field_type)
        else:
            if entities_context["metadatas_number"] > 0:
                attribute["value"] = self.__verify_if_is_int_or_str(attribute["value"])
                __logger__.debug("type:  %s" % (field_type))
                __logger__.debug('field type: %s' % str(type(attribute)))

                assert str(type(attribute["value"])) == "<type '%s'>" % field_type, \
                    'ERROR - in attribute value "%s" with type "%s" does not match without attribute type but with metadatas' % (str(attribute["value"]), field_type)
            else:
                attribute = self.__verify_if_is_int_or_str(attribute)
                assert str(type(attribute)) == "<type '%s'>" % field_type, \
                    'ERROR - in attribute value "%s" with type "%s" does not match without attribute type and without metadatas' % (str(attribute), field_type)


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
                    "type":"celcius",
                    "value":"34",
                    "very_hot_1":{
                        "type":"alarm",
                        "value":"hot"
                    }
                }
            }
        """
        attrs_list = []

        resp_json = convert_str_to_dict(resp.content, JSON)
        __logger__.debug("query parameter: %s" % str(queries_parameters))
        assert resp_json["id"] == entity_id_to_test,  'ERROR - in id "%s"' % resp_json["id"]
        if entities_context["entities_type"] is not None:
            assert resp_json["type"] == entities_context["entities_type"],  'ERROR - in type "%s" ' % resp_json["type"]
        # queries parameters
        if queries_parameters != {}:
            attrs_list = queries_parameters["attrs"].split(",")
        else:
            if int(entities_context["attributes_number"]) == 1:
                attrs_list.append(entities_context["attributes_name"])
            else:
                for e in range(int(entities_context["attributes_number"])):
                    attrs_list.append("%s_%s" % (entities_context["attributes_name"], str(e)))
        # verify attributes
        for i in range(len(attrs_list)):
            attr_name = attrs_list[i]
            __logger__.info("attribute name: %s" % attr_name)
            assert attr_name in resp_json, 'ERROR - in attribute name "%s" ' % attr_name
            attribute = resp_json[attr_name]
            if entities_context["attributes_type"] is not None:
                assert entities_context["attributes_type"] == attribute["type"], \
                    'ERROR - in attribute type "%s" in position: %s' % (entities_context["attributes_type"], str(i))
                assert entities_context["attributes_value"] == attribute["value"], \
                    'ERROR - in attribute value "%s" in position: %s' % (entities_context["attributes_value"], str(i))
            else:
                if entities_context["metadatas_number"] > 0:
                    assert entities_context["attributes_value"] == attribute["value"], \
                        'ERROR - in attribute value "%s" in position: %s without attribute type nor metadatas' % (entities_context["attributes_value"], str(i))
                else:
                    assert entities_context["attributes_value"] == attribute, \
                        'ERROR - in attribute value "%s" in position: %s without attribute type but with metadatas' % (entities_context["attributes_value"], str(i))
            # verify attribute metadatas
            if entities_context["metadatas_number"] > 0:
                for m in range(int(entities_context["metadatas_number"])):
                    assert "%s_%s" % (entities_context["metadatas_name"], str(m)) in attribute,\
                        'ERROR - attribute metadata name "%s_%s" does not exist in position:%s' \
                        % (entities_context["metadatas_name"], str(i), str(m))
                    metadata = attribute["%s_%s" % (entities_context["metadatas_name"], str(m))]
                    if entities_context["metadatas_type"] is not None:
                        assert entities_context["metadatas_type"] == metadata["type"], \
                            'ERROR - in attribute metadata type "%s" in position: %s' % (entities_context["metadatas_type"],
                                                                                         str(i))
                        assert entities_context["metadatas_value"] == metadata["value"], \
                            'ERROR - in attribute metadata value "%s" in position: %s' % (entities_context["metadatas_value"],
                                                                                          str(i))
                    else:
                        assert entities_context["metadatas_value"] == metadata, \
                            'ERROR - in attribute metadata value "%s" in position: %s' % (entities_context["metadatas_value"], str(i))

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
                    very_hot: {
                        type: "alarm"
                        value: "false"
                    }
                }
            }
        """
        resp_json = convert_str_to_dict(resp.content, JSON)
        assert attribute_name_to_request in resp_json, 'ERROR - in attribute name "%s" ' % attribute_name_to_request
        attribute = resp_json[attribute_name_to_request]
        if entities_context["attributes_type"] is not None:
            assert entities_context["attributes_type"] == attribute["type"],\
                'ERROR - in attribute type "%s"' % (entities_context["attributes_type"])
            assert entities_context["attributes_value"] == attribute["value"],\
                'ERROR - in attribute value "%s"' % (entities_context["attributes_value"])
        else:
            if entities_context["metadatas_number"] > 0:
                assert entities_context["attributes_value"] == attribute["value"],\
                    'ERROR - in attribute value "%s" without attribute type nor metadatas' % (entities_context["attributes_value"])
            else:
                assert entities_context["attributes_value"] == attribute,\
                    'ERROR - in attribute value "%s" without attribute type but with metadatas' % (entities_context["attributes_value"])
        # verify attribute metadatas
        if entities_context["metadatas_number"] > 0:
            for m in range(int(entities_context["metadatas_number"])):
                assert "%s_%s" % (entities_context["metadatas_name"], str(m)) in attribute,\
                    'ERROR - attribute metadata name "%s_%s" does not exist' % (entities_context["metadatas_name"], str(m))
                metadata = attribute["%s_%s" % (entities_context["metadatas_name"], str(m))]
                if entities_context["metadatas_type"] is not None:
                    assert entities_context["metadatas_type"] == metadata["type"], \
                        'ERROR - in attribute metadata type "%s"' % (entities_context["metadatas_type"])
                    assert entities_context["metadatas_value"] == metadata["value"], \
                        'ERROR - in attribute metadata value "%s"' % (entities_context["metadatas_value"])
                else:
                    assert entities_context["metadatas_value"] == metadata, \
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
        entity = convert_str_to_dict(resp.text, JSON)   # in raw mode is used only one entity
        # verify attribute
        assert self.__remove_quote(entities_context["attributes_name"]) in entity,   \
            'ERROR - attribute name "%s" does not exist in raw mode' % entities_context["attributes_name"]
        attribute = entity[self.__remove_quote(entities_context["attributes_name"])]
        __logger__.debug("attribute: %s" % str(attribute))
        if entities_context["attributes_type"] is not None:
            assert self.__remove_quote(entities_context["attributes_type"]) == attribute["type"], \
                'ERROR - in attribute type "%s" in raw mode' % (self.__remove_quote(entities_context["attributes_type"]))

            attribute["value"] = self.__verify_if_is_int_or_str(attribute["value"])
            __logger__.debug("type:  %s" % (field_type))
            __logger__.debug('field type: %s' % str(type(attribute)))
            assert str(type(attribute["value"])) == "<type '%s'>" % field_type, \
                'ERROR - in attribute value "%s" with type "%s" does not match' % (str(entities_context["attributes_value"]), field_type)
        else:
            if entities_context["metadatas_number"] > 0:
                attribute["value"] = self.__verify_if_is_int_or_str(attribute["value"])
                __logger__.debug("type:  %s" % field_type)
                __logger__.debug('field type: %s' % str(type(attribute)))

                assert str(type(attribute["value"])) == "<type '%s'>" % field_type, \
                    'ERROR - in attribute value "%s" with type "%s" does not match without attribute type but with metadatas' % (str(attribute["value"]), field_type)
            else:
                attribute = self.__verify_if_is_int_or_str(attribute)
                assert str(type(attribute)) == "<type '%s'>" % field_type, \
                    'ERROR - in attribute value "%s" with type "%s" does not match without attribute type and ' \
                    'without metadatas' % (str(attribute), field_type)

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

