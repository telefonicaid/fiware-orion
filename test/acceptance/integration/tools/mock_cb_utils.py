# -*- coding: utf-8 -*-
"""
Copyright 2014 Telefonica Investigación y Desarrollo, S.A.U

This file is part of fiware-orion

fiware-orion is free software: you can redistribute it and/or
modify it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

fiware-orion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public
License along with fiware-orion.
If not, seehttp://www.gnu.org/licenses/.

For those usages not covered by the GNU Affero General Public License
please contact with::[iot_support@tid.es]
"""

__author__ = 'Jon Calderin Goñi (jcaldering@gmail.com)'

from flask import Flask, request
import json

app = Flask(__name__)

counter = 0
petitions = {}

query_context_response_json = {
    "contextResponses": [
        {
            "contextElement": {
                "attributes": [
                    {
                        "name": "temperature",
                        "type": "float",
                        "value": "23"
                    },
                    {
                        "name": "pressure",
                        "type": "integer",
                        "value": "720"
                    }
                ],
                "id": "Room1",
                "isPattern": "false",
                "type": "Room"
            },
            "statusCode": {
                "code": "200",
                "reasonPhrase": "OK"
            }
        }
    ]
}

query_context_response_xml = """<?xml version="1.0"?>
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
</queryContextResponse>"""

update_context_response_xml = """
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
"""


@app.route('/', defaults={'path': ''}, methods=['GET', 'POST', 'UPDATE', 'DELETE', 'PATCH', 'HEAD', 'OPTIONS', 'PUT'])
@app.route('/<path:path>', methods=['GET', 'POST', 'UPDATE', 'DELETE', 'PATCH', 'HEAD', 'OPTIONS', 'PUT'])
def catch(path):
    global counter
    global petitions

    if path == 'get_data':
        resp = petitions.copy()
        petitions = {}
        return json.dumps(resp), 200
    else:
        petitions.update({
            counter: {
                "path": path,
                "headers": dict(request.headers),
                "parms": request.args
            }
        })
        if request.data:
            try:
                petitions[counter].update({"payload": json.loads(request.data)})
            except:
                petitions[counter].update({"payload": request.data})
        if path.lower().find('querycontext') >= 0:
            return query_context_response_xml, 200
        if path.lower().find('updatecontext') >= 0:
            return update_context_response_xml, 200
        else:
            return "Saved", 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5566, debug=True)

