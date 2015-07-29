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

from iotqautils.helpers_utils import *

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
    def verify_entities_stored_in_mongo(self, mongo_driver, entities_contexts, headers, stored=True):
        """
        verify that entities are stored in mongo
        :param entities_contexts:  entities context (see constructor in CB_v2_utils.py)
        :param headers: headers used (see "definition_headers" method in CB_v2_utils.py)
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
        if FIWARE_SERVICE_HEADER in headers:
            database_name = "%s-%s" % (ORION_PREFIX, headers[FIWARE_SERVICE_HEADER])
        else:
            database_name = ORION_PREFIX
        # Orion Context Broker interprets the tentant name (service) in lowercase, thus,
        # although you can use tenants such as in updateContext "MyService" it is not advisable
        mongo_driver.connect(database_name.lower())
        mongo_driver.choice_collection("entities")

        if FIWARE_SERVICE_PATH_HEADER in headers:
            service_path = headers[FIWARE_SERVICE_PATH_HEADER]
        else:
            service_path = "/"
        if "entities_type" in entities_contexts:
            type = entities_contexts["entities_type"]
        else:
            type = ""

        curs_list = []
        curs = mongo_driver.find_data({"_id.id": {'$regex': '%s.*' % entities_contexts["entities_id"]},
                                       "_id.type": type,
                                       "_id.servicePath": service_path})
        curs_list = mongo_driver.get_cursor_value(curs)
        for i in range(int(entities_contexts["entities_number"])):
            # verify if the entity is stored in mongo
            if stored:
                for entity in curs_list:  # manages N entities
                    for a in range(int(entities_contexts["attributes_number"])): # manages N attributes
                        if int(entities_contexts["attributes_number"]) == 1:
                            attr_name = entities_contexts["attributes_name"]
                        else:
                            # prefix plus _<consecutive>. ex: room1_2
                            attr_name = "%s_%s" % (entities_contexts["attributes_name"], str(a))

                        #attr_name = "%s_%s" % (entities_contexts["attributes_name"], str(a))
                        if entities_contexts["attributes_name"] is not None:
                            assert attr_name in entity[u'attrNames'], \
                                " ERROR -- attribute name: %s is not stored in attrNames list" % attr_name

                            # '.' char is forbidden in MongoDB keys, so it needs to be replaced (we chose '=' as it is a
                            # forbidden character in the API). When returning information to user, the '=' is translated
                            # back to '.', thus from user perspective all works fine.
                            if attr_name.find(".") >= 0:
                                attr_name = attr_name.replace(".", "=")
                                __logger__.debug("attribute name: %s is changed by \".\" is forbidden in MongoDB keys" % attr_name)
                            __logger__.info("attribute full: %s" % str(entity))

                            assert attr_name in entity[u'attrs'], \
                                " ERROR -- attribute: %s is not stored in mongo" % attr_name

                            assert entities_contexts["attributes_value"] == entity[u'attrs'][attr_name][u'value'], \
                                " ERROR -- attribute value: %s is not stored successful in mongo" % attr_name
                            if entities_contexts["attributes_type"] is None:
                                entities_contexts["attributes_type"] = EMPTY
                            assert entities_contexts["attributes_type"] == entity[u'attrs'][attr_name][u'type'], \
                                " ERROR -- attribute type: %s is not stored successful in mongo" % entities_contexts["attributes_type"]
                            if entities_contexts["metadatas_number"] is not None:
                                for m in range(int(entities_contexts["metadatas_number"])):    # manages N metadatas
                                    md = entity[u'attrs'][attr_name][u'md']
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
                __logger__.debug(" Entity id: \"%s\"_%s is successful stored in database: \"%s\"" % (entities_contexts["entities_id"], str(i), database_name))
            # verify if the entity is not stored in mongo
            else:
                assert len(curs_list) == 0, "  ERROR - the entities should not have been saved in mongo"
                __logger__.debug(" Entity id: \"%s_%s\" is stored in database: \"%s\", not as expected" % (entities_contexts["entities_id"], str(i), database_name))

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
