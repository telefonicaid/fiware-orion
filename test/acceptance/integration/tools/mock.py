# -*- coding: utf-8 -*-
"""
# Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

from flask import Flask, request
import json
import sys

app = Flask(__name__)

counter = 0
petitions = {}
response_defined_by_user = {}

@app.route('/', defaults={'path': ''}, methods=['GET', 'POST', 'UPDATE', 'DELETE', 'PATCH', 'HEAD', 'OPTIONS', 'PUT'])
@app.route('/<path:path>', methods=['GET', 'POST', 'UPDATE', 'DELETE', 'PATCH', 'HEAD', 'OPTIONS', 'PUT'])
def catch(path):
    """
    Capture all requests to the mock. Depending of the path, the functionality changes, the possibilities are:
    - get_data --> Response with all petitions and reset the petitions
    - set_response --> Set specific response for all petitions (except specials)
    - reset_response --> Reset the response for all petitions (except specials)
    :param path:
    :return:
    """
    global counter
    global petitions
    global response_defined_by_user

    if path == 'get_data':
        resp = petitions.copy()
        petitions = {}
        return json.dumps(resp), 200
    elif path == 'set_response':
        try:
            response_recv = json.loads(request.data)
        except Exception as e:
            raise e, 'The payload is not a json object, the payload is: {payload}'.format(payload=request.data)
        try:
            response_defined_by_user.update({'headers', response_recv['headers']})
            response_defined_by_user.update({'payload', response_recv['payload']})
            response_defined_by_user.update({'status_code', response_recv['status_code']})
        except KeyError as e:
            raise e, 'The response received in the payload is {response}'.format(response=response_recv)
        return 'Response set', 200
    elif path == 'reset_response':
        response_defined_by_user = {}
        return 'Response reset', 200
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
        if response_defined_by_user != {}:
            return response_defined_by_user['payload'], response_defined_by_user['status_code'], response_defined_by_user['headers']
        else:
            return "Saved", 200


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise AttributeError, "Usage is \"python mock.py bind_ip port\""
    app.run(host=sys.argv[1], port=sys.argv[2], debug=True)

