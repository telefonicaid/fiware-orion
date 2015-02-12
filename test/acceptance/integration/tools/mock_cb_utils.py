# -*- coding: utf-8 -*-
"""
(c) Copyright 2014 Telefonica, I+D. Printed in Spain (Europe). All Rights
Reserved.

The copyright to the software program(s) is property of Telefonica I+D.
The program(s) may be used and or copied only with the express written
consent of Telefonica I+D or in accordance with the terms and conditions
stipulated in the agreement/contract under which the program(s) have
been supplied.
"""
__author__ = 'Jon'

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

