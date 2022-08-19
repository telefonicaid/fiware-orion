# -*- coding: utf-8 -*-
"""
# Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
"""

__author__ = 'Jon Calderin Goñi (jon.caldering@gmail.com)'


responses = {
    'query_context_response_from_context_provider_xml': {
        'body': """<?xml version="1.0"?>
                                                            <queryContextResponse>
                                                              <contextResponseList>
                                                                <contextElementResponse>
                                                                  <contextElement>
                                                                    <entityId type="Room" isPattern="false">
                                                                      <id>Room1</id>
                                                                    </entityId>
                                                                    <contextAttributeList>
                                                                      <contextAttribute>
                                                                        <name>temperature</name>
                                                                        <type>float</type>
                                                                        <contextValue>23</contextValue>
                                                                      </contextAttribute>
                                                                      <contextAttribute>
                                                                        <name>pressure</name>
                                                                        <type>integer</type>
                                                                        <contextValue>720</contextValue>
                                                                      </contextAttribute>
                                                                    </contextAttributeList>
                                                                  </contextElement>
                                                                  <statusCode>
                                                                    <code>200</code>
                                                                    <reasonPhrase>OK</reasonPhrase>
                                                                  </statusCode>
                                                                </contextElementResponse>
                                                              </contextResponseList>
                                                            </queryContextResponse>
                                                            """,
        'status_code': 200,
        'headers': {},
        'delay': 0
        },
    'update_context_response_from_context_provider_xml': {
        'body': """
                                                            <?xml version="1.0"?>
                                                            <updateContextResponse>
                                                              <contextResponseList>
                                                                <contextElementResponse>
                                                                  <contextElement>
                                                                    <entityId type="Room" isPattern="false">
                                                                      <id>Room1</id>
                                                                    </entityId>
                                                                    <contextAttributeList>
                                                                      <contextAttribute>
                                                                        <name>temperature</name>
                                                                        <type>float</type>
                                                                        <contextValue/>
                                                                      </contextAttribute>
                                                                      <contextAttribute>
                                                                        <name>pressure</name>
                                                                        <type>integer</type>
                                                                        <contextValue/>
                                                                      </contextAttribute>
                                                                    </contextAttributeList>
                                                                  </contextElement>
                                                                  <statusCode>
                                                                    <code>200</code>
                                                                    <reasonPhrase>OK</reasonPhrase>
                                                                  </statusCode>
                                                                </contextElementResponse>
                                                              </contextResponseList>
                                                            </updateContextResponse>
                                                            """,
        'status_code': 200,
        'headers': {},
        'delay': 0
    },
    'ok': {
        'body': "",
        'status_code': 200,
        'headers': {},
        'delay': 0
    }
}
