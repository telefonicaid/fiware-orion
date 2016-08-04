# -*- coding: utf-8 -*-

# Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#
# author: 'Iván Arias León (ivan dot ariasleon at telefonica dot com)'

#
#  Note: the "skip" tag is to skip the scenarios that still are not developed or failed
#        -tg=-skip
#


Feature: create an entity with geospatial properties of entities using NGSIv2. "POST" - /v2/entities/ plus payload and queries parameters
  As a context broker user
  I would like to create an entity with geospatial properties of entities using NGSIv2
  So that I can manage and use them in my scripts

  Actions Before the Feature:
  Setup: update properties test file from "epg_contextBroker.txt" and sudo local "false"
  Setup: update contextBroker config file
  Setup: start ContextBroker
  Check: verify contextBroker is installed successfully
  Check: verify mongo is installed successfully

  Actions After each Scenario:
  Setup: delete database in mongo

  Actions After the Feature:
  Setup: stop ContextBroker

  # the issue #2426 is appended to backlog with possibles verifications in attribute value. Tests still are not designed.

  # -------------------- Simple Location Format ---------------
  @simple_location_wo_metadata
  Scenario Outline:  create an entity with Simple Location Format to define location and to geo-query using NGSIv2 without metadata
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_simple_location |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value                |
      | entities_type    | "home"               |
      | entities_id      | "room_1"             |
      | attributes_name  | "location"           |
      | attributes_value | <geojson_coords>     |
      | attributes_type  | "geo:<geojson_type>" |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Created" http code
    Examples:
      | geojson_type | geojson_coords                                                                        |
      | point        | "25.774,-80.190"                                                                      |
      | point        | "-90,180"                                                                             |
      | Point        | "90,-180"                                                                             |
      | Point        | "75.774,40.190"                                                                       |
      | Point        | "80.19056, 125.77498"                                                                 |
      | line         | ["90,-180","-90,180"]                                                                 |
      | line         | ["-15.01621,139.57422","-82.35529,23.1082"]                                           |
      | Line         | ["25.774,-80.190","18.466,-66.118"]                                                   |
      | Line         | ["25.774,-80.190","18.466,-66.118","-80.190,25.774"]                                  |
      | box          | ["90,-180","-90,180"]                                                                 |
      | box          | ["-15.01621,139.57422","-82.35529,23.1082"]                                           |
      | Box          | ["25.774,-80.190","18.466,-66.118"]                                                   |
      | Polygon      | ["-75.690,35.742","-75.59,35.742","-75.541,35.585","-75.941,35.485","-75.690,35.742"] |

  @simple_location_w_metadata
  Scenario Outline:  create an entity with Simple Location Format to define location and to geo-query using NGSIv2 with metadata
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_simple_location |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value                |
      | entities_type    | "home"               |
      | entities_id      | "room_1"             |
      | attributes_name  | "location"           |
      | attributes_value | <geojson_coords>     |
      | attributes_type  | "geo:<geojson_type>" |
      | metadatas_name   | "very_hot"           |
      | metadatas_type   | "alarm"              |
      | metadatas_value  | "hot"                |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Created" http code
    Examples:
      | geojson_type | geojson_coords                                                                        |
      | point        | "25.774,-80.190"                                                                      |
      | point        | "-90,180"                                                                             |
      | Point        | "90,-180"                                                                             |
      | Point        | "75.774,40.190"                                                                       |
      | Point        | "80.19056, 125.77498"                                                                 |
      | line         | ["90,-180","-90,180"]                                                                 |
      | line         | ["-15.01621,139.57422","-82.35529,23.1082"]                                           |
      | Line         | ["25.774,-80.190","18.466,-66.118"]                                                   |
      | Line         | ["25.774,-80.190","18.466,-66.118","-80.190,25.774"]                                  |
      | box          | ["90,-180","-90,180"]                                                                 |
      | box          | ["-15.01621,139.57422","-82.35529,23.1082"]                                           |
      | Box          | ["25.774,-80.190","18.466,-66.118"]                                                   |
      | Polygon      | ["-75.690,35.742","-75.59,35.742","-75.541,35.585","-75.941,35.485","-75.690,35.742"] |

  @simple_location_malformed  @simple_location_malformed.row<row.id>
  Scenario Outline:  try to create an entity with Simple Location Format to define location and to geo-query using NGSIv2 with format error
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_simple_location |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value                |
      | entities_type    | "home"               |
      | entities_id      | "room_1"             |
      | attributes_name  | "location"           |
      | attributes_value | <geojson_coords>     |
      | attributes_type  | "geo:<geojson_type>" |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                   |
      | error       | BadRequest                                                              |
      | description | geo coordinates format error [see Orion user manual]: <coords_response> |
    Examples:
      | geojson_type | geojson_coords                                                                              | coords_response |
      | point        | "180,-90"                                                                                   | 180,-90         |
      | point        | "-180,90"                                                                                   | -180,90         |
      | point        | "90,181"                                                                                    | 90,181          |
      | line         | ["-180,90","-90,180"]                                                                       | -180,90         |
      | box          | ["-180,90","-90,180"]                                                                       | -180,90         |
      | polygon      | ["-180,90","-90,180","-75.59,35.742","-75.541,35.585","-75.941,35.485","-180,90","-90,180"] | -180,90         |

  @simple_location_not_list
  Scenario Outline:  try to create an entity with Simple Location Format to define location and to geo-query using NGSIv2 with no list
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_simple_location |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value                |
      | entities_type    | "home"               |
      | entities_id      | "room_1"             |
      | attributes_name  | "location"           |
      | attributes_value | <geojson_coords>     |
      | attributes_type  | "geo:<geojson_type>" |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | ParseError                           |
      | description | Errors found in incoming JSON buffer |
    Examples:
      | geojson_type | geojson_coords                                                                                        |
      | line         | "25.774,-80.190","25.874,-80.390"                                                                     |
      | box          | "25.774,-80.190","25.874,-80.390"                                                                     |
      | polygon      | "25.774,-80.190","25.874,-80.390","-75.59,35.742","-75.541,35.585","-75.941,35.485","25.774,-80.190"] |

  @simple_location_empty
  Scenario Outline:  try to create an entity with Simple Location Format to define location and to geo-query using NGSIv2 with emtpy value
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_simple_location |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value                |
      | entities_type    | "home"               |
      | entities_id      | "room_1"             |
      | attributes_name  | "location"           |
      | attributes_value | <geojson_coords>     |
      | attributes_type  | "geo:<geojson_type>" |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                   |
      | error       | BadRequest                                                              |
      | description | geo coordinates format error [see Orion user manual]: <coords_response> |
    Examples:
      | geojson_type | geojson_coords                                                                       | coords_response |
      | point        | ""                                                                                   |                 |
      | line         | ["","-90,180"]                                                                       |                 |
      | box          | ["","-90,180"]                                                                       |                 |
      | polygon      | ["","-90,180","-75.59,35.742","-75.541,35.585","-75.941,35.485","-180,90","-90,180"] |                 |

  @simple_location_invalid_type
  Scenario Outline:  try to create an entity with Simple Location Format to define location and to geo-query using NGSIv2 with invalid value type
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_simple_location |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value                |
      | entities_type    | "home"               |
      | entities_id      | "room_1"             |
      | attributes_name  | "location"           |
      | attributes_value | <geojson_coords>     |
      | attributes_type  | "geo:<geojson_type>" |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                                                   |
      | error       | BadRequest                                                              |
      | description | geo coordinates format error [see Orion user manual]: <coords_response> |
    Examples:
      | geojson_type | geojson_coords                                                                     | coords_response |
      | point        | true                                                                               |                 |
      | point        | false                                                                              |                 |
      | point        | null                                                                               |                 |
      | point        | "nothing, 45"                                                                      | nothing, 45     |
      | point        | ["25.774,-80.190"]                                                                 |                 |
      | point        | {"25.774": "-80.190"}                                                              |                 |
      | line         | ["nothing, 45","25.774,-80.190"]                                                   | nothing, 45     |
      | box          | ["nothing, 45","25.774,-80.190"]                                                   | nothing, 45     |
      | polygon      | ["-75.690,35.742","-75.59,35.742","nothing, 45","-75.941,35.485","-75.690,35.742"] | nothing, 45     |

  @simple_location_invalid_type
  Scenario Outline:  try to create an entity with Simple Location Format to define location and to geo-query using NGSIv2 with invalid value type
    Given  a definition of headers
      | parameter          | value                |
      | Fiware-Service     | test_simple_location |
      | Fiware-ServicePath | /test                |
      | Content-Type       | application/json     |
    And properties to entities
      | parameter        | value                |
      | entities_type    | "home"               |
      | entities_id      | "room_1"             |
      | attributes_name  | "location"           |
      | attributes_value | <geojson_coords>     |
      | attributes_type  | "geo:<geojson_type>" |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value         |
      | error       | BadRequest    |
      | description | <description> |
    Examples:
      | geojson_type | geojson_coords            | description                                                                               |
      | line         | [true, "25.774,-80.190"]  | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | line         | [false, "25.774,-80.190"] | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | line         | [null, "25.774,-80.190"]  | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | line         | {"25.774": "-80.190"}     | geo:line, geo:box and geo:polygon needs array of strings as value                         |
    Examples:
      | geojson_type | geojson_coords            | description                                                                               |
      | box          | [true, "25.774,-80.190"]  | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | box          | [false, "25.774,-80.190"] | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | box          | [null, "25.774,-80.190"]  | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | box          | {"25.774": "-80.190"}     | geo:line, geo:box and geo:polygon needs array of strings as value                         |
    Examples:
      | geojson_type | geojson_coords                                                                  | description                                                                               |
      | polygon      | ["-75.690,35.742","-75.59,35.742",true,"-75.941,35.485","-75.690,35.742"]       | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | polygon      | ["-75.690,35.742","-75.59,35.742",false,"-75.941,35.485","-75.690,35.742"]      | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | polygon      | ["-75.690,35.742","-75.59,35.742",null,"-75.941,35.485","-75.690,35.742"]       | geo:line, geo:box and geo:polygon needs array of strings but some element is not a string |
      | polygon      | {"value": ["-75.690,35.742","-75.59,35.742","-75.941,35.485","-75.690,35.742"]} | geo:line, geo:box and geo:polygon needs array of strings as value                         |

  # ---------------------- GeoJSON ----------------------------
  @geo_json_wo_metadata
  Scenario Outline:  create an entity with GeoJSON support to define location and to geo-query using NGSIv2 without metadata
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_geo_json    |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value                                                        |
      | entities_type    | "house"                                                      |
      | entities_id      | "room1"                                                      |
      | attributes_name  | "location"                                                   |
      | attributes_value | {"type": "<geojson_type>","coordinates": [<geojson_coords>]} |
      | attributes_type  | "geo:json"                                                   |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Created" http code
    Examples:
      | geojson_type    | geojson_coords                                                                                                                                                                  |
      | Point           | 180,-90                                                                                                                                                                         |
      | Point           | -180,90                                                                                                                                                                         |
      | Point           | 75.774,40.190                                                                                                                                                                   |
      | Point           | 80.19056,25.77498                                                                                                                                                               |
      | MultiPoint      | [-180,90],[180,-90]                                                                                                                                                             |
      | MultiPoint      | [139.57422,-15.01621],[-82.35529,23.1082]                                                                                                                                       |
      | LineString      | [25.774,-80.190],[18.466,-66.118]                                                                                                                                               |
      | LineString      | [25.774,-80.190],[18.466,-66.118],[-80.190,25.774]                                                                                                                              |
      | MultiLineString | [[-80.190,25.774],[-66.118,18.466]],[[25.774,-80.190],[18.466,-66.118]]                                                                                                         |
      | MultiLineString | [[-80.190,25.774],[-66.118,18.466]],[[25.774,-80.190],[18.466,-66.118]],[[11.774,-60.190],[11.969,-76.118]]                                                                     |
      | Polygon         | [[-75.690,35.742],[-75.59,35.742],[-75.541,35.585],[-75.941,35.485],[-75.690,35.742]]                                                                                           |
      | MultiPolygon    | [[[-73.958,40.8003 ],[-73.9498,40.7968],[-73.9737,40.7648],[-73.9814,40.7681],[-73.958,40.8003]]],[[[-73.958,40.8003],[-73.9498,40.7968],[-73.9737,40.7648],[-73.958,40.8003]]] |

  @geo_json_w_metadata
  Scenario Outline:  create an entity with GeoJSON support to define location and to geo-query using NGSIv2 with metadata
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_geo_json    |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value                                                        |
      | entities_type    | "house"                                                      |
      | entities_id      | "room1"                                                      |
      | attributes_name  | "location"                                                   |
      | attributes_value | {"type": "<geojson_type>","coordinates": [<geojson_coords>]} |
      | attributes_type  | "geo:json"                                                   |
      | metadatas_name   | "very_hot"                                                   |
      | metadatas_type   | "alarm"                                                      |
      | metadatas_value  | "hot"                                                        |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Created" http code
    Examples:
      | geojson_type    | geojson_coords                                                                                                                                                                  |
      | Point           | 180,-90                                                                                                                                                                         |
      | Point           | -180,90                                                                                                                                                                         |
      | Point           | 75.774,40.190                                                                                                                                                                   |
      | Point           | 80.19056,25.77498                                                                                                                                                               |
      | MultiPoint      | [-180,90],[180,-90]                                                                                                                                                             |
      | MultiPoint      | [139.57422,-15.01621],[-82.35529,23.1082]                                                                                                                                       |
      | LineString      | [25.774,-80.190],[18.466,-66.118]                                                                                                                                               |
      | LineString      | [25.774,-80.190],[18.466,-66.118],[-80.190,25.774]                                                                                                                              |
      | MultiLineString | [[-80.190,25.774],[-66.118,18.466]],[[25.774,-80.190],[18.466,-66.118]]                                                                                                         |
      | MultiLineString | [[-80.190,25.774],[-66.118,18.466]],[[25.774,-80.190],[18.466,-66.118]],[[11.774,-60.190],[11.969,-76.118]]                                                                     |
      | Polygon         | [[-75.690,35.742],[-75.59,35.742],[-75.541,35.585],[-75.941,35.485],[-75.690,35.742]]                                                                                           |
      | MultiPolygon    | [[[-73.958,40.8003 ],[-73.9498,40.7968],[-73.9737,40.7648],[-73.9814,40.7681],[-73.958,40.8003]]],[[[-73.958,40.8003],[-73.9498,40.7968],[-73.9737,40.7648],[-73.958,40.8003]]] |

  @geo_json_wrong
  Scenario Outline:  try to create an entity with GeoJSON support to define location and to geo-query using NGSIv2, but attribute value is not a json object
    Given  a definition of headers
      | parameter          | value            |
      | Fiware-Service     | test_geo_json    |
      | Fiware-ServicePath | /test            |
      | Content-Type       | application/json |
    And properties to entities
      | parameter        | value             |
      | entities_type    | "house"           |
      | entities_id      | "room2"           |
      | attributes_name  | "location"        |
      | attributes_value | <attribute_value> |
      | attributes_type  | "geo:json"        |
      | metadatas_name   | "very_hot"        |
      | metadatas_type   | "alarm"           |
      | metadatas_value  | "hot"             |
    When create an entity in raw and "normalized" modes
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                             |
      | error       | BadRequest                        |
      | description | geo:json needs an object as value |
    Examples:
      | attribute_value |
      | 34              |
      | "nothing"       |
      | [34, 45, "a"]   |
      | true            |
      | false           |
      | null            |
