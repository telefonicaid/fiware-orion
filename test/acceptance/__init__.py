import socket

__author__ = 'Jon'

from lettuce import world
import json
import os
import sys


"""
Parse the JSON configuration file located in the src folder and
store the resulting dictionary in the lettuce world global variable.
"""
with open("properties.json") as config_file:
    try:
        world.config = json.load(config_file)
    except Exception, e:
        print 'Error parsing config file: %s' % (e)
        sys.exit(1)
    # Get local ip
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(('8.8.8.8', 0))  # connecting to a UDP address doesn't send packets
    local_ip_address = s.getsockname()[0]
    # ***************************************************************************
    world.config['mock'].update({'host': local_ip_address})

"""
Make sure the logs path exists and create it otherwise.
"""
if not os.path.exists(world.config["environment"]["logs_path"]):
    os.makedirs(world.config["environment"]["logs_path"])