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

Feature: create new subscriptions (POST) using NGSI v2. "POST" - /v2/subscriptions plus payload
  As a context broker user
  I would like to create new subscriptions (POST) using NGSI v2
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

  # ------------ notification field and its sub-fields ---------------------

  @notification_without
  Scenario:  try to create a new subscription using NGSI v2 without notification field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                      |
      | subject_idPattern     | .*                         |
      | condition_attrs       | temperature                |
      | notification_http_url | without notification field |
      | notification_attrs    | temperature                |
      | expires               | 2016-04-05T14:00:00.00Z    |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                      |
      | error       | BadRequest                                 |
      | description | no notification for subscription specified |

  # ------------ notification - http field ---------------------
  @notification_http_without
  Scenario:  try to create a new subscription using NGSI v2 without notification - http field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter          | value                   |
      | subject_idPattern  | .*                      |
      | condition_attrs    | temperature             |
      | notification_attrs | temperature             |
      | expires            | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                        |
      | error       | BadRequest                   |
      | description | http notification is missing |

  # ------------ notification - http - url field ---------------------
  @notification_http_url_without
  Scenario:  try to create a new subscription using NGSI v2 without notification - http - url field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                               |
      | subject_idPattern     | .*                                  |
      | condition_attrs       | temperature                         |
      | notification_attrs    | temperature                         |
      | notification_http_url | without notification http url field |
      | expires               | 2016-04-05T14:00:00.00Z             |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                            |
      | error       | BadRequest                       |
      | description | url http notification is missing |

  @notification_http_url_hostname
  Scenario Outline:  create a new subscription using NGSI v2 with notification http url field values using hostname valids
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_notification_http_url |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value       |
      | subject_idPattern     | .*          |
      | condition_attrs       | temperature |
      | notification_http_url | <url>       |
      | notification_attrs    | temperature |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | url                                                                                                                                                                                                                                                            |
      | http://localhost                                                                                                                                                                                                                                               |
      | http://es.localhost                                                                                                                                                                                                                                            |
      | http://es.localhost.my                                                                                                                                                                                                                                         |
      | http://es.localhost-45.my                                                                                                                                                                                                                                      |
      | http://localhost:1                                                                                                                                                                                                                                             |
      | http://localhost:65535/my_path                                                                                                                                                                                                                                 |
      | https://localhost:1234/my_path/multiple                                                                                                                                                                                                                        |
      | https://localhost-12345678901234567890123456789012345678901234567890123.my:1234/my_path/multiple                                                                                                                                                               |
      | https://localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost1234567890123456789012:1234 |
      | http://e34.56.45.34                                                                                                                                                                                                                                            |
      | http://34.56:3454                                                                                                                                                                                                                                              |
      | http://a.b--c.de/                                                                                                                                                                                                                                              |
      | http://0.0.0.0                                                                                                                                                                                                                                                 |
      | http://1.1.1.1.1                                                                                                                                                                                                                                               |
      | http://3441.2344.1231.1123                                                                                                                                                                                                                                     |
      | http://123.123.123                                                                                                                                                                                                                                             |
      | http://3628126748                                                                                                                                                                                                                                              |

  @notification_http_url_ipv4
  Scenario Outline:  create a new subscription using NGSI v2 with notification http url field values using ipv4 valids
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_notification_http_url |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value       |
      | subject_idPattern     | .*          |
      | condition_attrs       | temperature |
      | notification_http_url | <url>       |
      | notification_attrs    | temperature |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | url                                       |
      | http://0.0.0.0                            |
      | http://255.255.255.255                    |
      | http://192.10.10.1                        |
      | http://192.10.10.1:1                      |
      | http://192.10.10.1:65535/my_path          |
      | https://192.10.10.1:1234/my_path/multiple |

  @notification_http_url_ipv6 @BUG_2859 @skip
  Scenario Outline:  create a new subscription using NGSI v2 with notification http url field values using ipv6 valids
    Given  a definition of headers
      | parameter          | value                      |
      | Fiware-Service     | test_notification_http_url |
      | Fiware-ServicePath | /test                      |
      | Content-Type       | application/json           |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value       |
      | subject_idPattern     | .*          |
      | condition_attrs       | temperature |
      | notification_http_url | <url>       |
      | notification_attrs    | temperature |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | url                                                            |
      | http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html |
      | http://[1080:0:0:0:8:800:200C:417A]/index.html                 |
      | http://[3ffe:2a00:100:7031::1]                                 |
      | http://[1080::8:800:200C:417A]/foo                             |
      | http://[::192.9.5.5]/ipng                                      |
      | http://[::FFFF:129.144.52.38]:80/index.html                    |
      | http://[2010:836B:4179::836B:4179]                             |

  @notification_http_url_q_param_error
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification http url field values using queries parameters
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value       |
      | subject_idPattern     | .*          |
      | condition_attrs       | temperature |
      | notification_http_url | <url>       |
      | notification_attrs    | temperature |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                    |
      | error       | BadRequest                               |
      | description | forbidden characters in http field /url/ |
    Examples:
      | url                                                                             |
      | https://192.10.10.1:1234/my_path/multiple?id=45&count=true                      |
      | https://localhost:1234/my_path/multiple?id=45&count=true                        |
      | http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html?id=45&count=true |

  @notification_http_url_empty @BUG_2007
  Scenario:  try to create a new subscription using NGSI v2 with empty values in notification http url
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url |                         |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid URL parsing notification url |

  @notification_http_url_invalid_desc @BUG_2006 @BUG_2093
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification http url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | <url>                   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid URL parsing notification url |
    Examples:
      | url               |
      | ws://             |
      | ://localhost      |
      | http//localhost   |
      | http:/localhost   |
      | http:localhost    |
      | http://localhost: |
      | http://           |
      | //a               |
      | ///a              |
      | foo.com           |
      | rdar://1234       |
      | h://test          |
      | http://           |
      | ://               |
      | ftps://foo.bar/   |

  @notification_http_url_invalid @BUG_2006 @BUG_2009 @skip
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification http url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | <url>                   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid URL parsing notification url |
    Examples:
      | url                                                                                                                                                                                                                                                                              |
      | http://my_host                                                                                                                                                                                                                                                                   |
      | http://my_host.server                                                                                                                                                                                                                                                            |
      | http://my_host-2                                                                                                                                                                                                                                                                 |
      | http://my_host-2.server                                                                                                                                                                                                                                                          |
      | http://my_host_2                                                                                                                                                                                                                                                                 |
      | http://my_host-2:2134                                                                                                                                                                                                                                                            |
      | http://my_host:3455/my_path                                                                                                                                                                                                                                                      |
      | http://my_host:3455/my_path/multiple                                                                                                                                                                                                                                             |
      | https://localhost123456789012345678901234567890123456789012345678901234567890.my:1234                                                                                                                                                                                            |
      | https://localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost1234567890123456789012345678901234567890:1234 |
      | http://localhost:900000                                                                                                                                                                                                                                                          |
      | http://localhost:dsfsdf                                                                                                                                                                                                                                                          |
      | http://localhost\my_path                                                                                                                                                                                                                                                         |
      | http://34,56.45.34                                                                                                                                                                                                                                                               |
      | http://.                                                                                                                                                                                                                                                                         |
      | http://..                                                                                                                                                                                                                                                                        |
      | http://../                                                                                                                                                                                                                                                                       |
      | http://?                                                                                                                                                                                                                                                                         |
      | http://??/                                                                                                                                                                                                                                                                       |
      | http://#                                                                                                                                                                                                                                                                         |
      | http://##/                                                                                                                                                                                                                                                                       |
      | http://-error-.invalid/                                                                                                                                                                                                                                                          |
      | http://_error_.invalid/                                                                                                                                                                                                                                                          |
      | http://-a.b.co                                                                                                                                                                                                                                                                   |
      | http://a.b-.co                                                                                                                                                                                                                                                                   |
      | http://.www.foo.bar/                                                                                                                                                                                                                                                             |
      | http://www.foo.bar./                                                                                                                                                                                                                                                             |
      | http://.www.foo.bar./                                                                                                                                                                                                                                                            |
      | http://192.168. 224.0                                                                                                                                                                                                                                                            |
      | http://192.168.224.0 1                                                                                                                                                                                                                                                           |
      | 3ffe:b00::1::a                                                                                                                                                                                                                                                                   |
      | 192.168.1.1/                                                                                                                                                                                                                                                                     |
      | 192.168.1.1/12.34                                                                                                                                                                                                                                                                |
      | 192.168.1.1/01                                                                                                                                                                                                                                                                   |
      | 192.168.1.1/33                                                                                                                                                                                                                                                                   |
      | 02001:0000:1234:0000:0000:C1C0:ABCD:0876                                                                                                                                                                                                                                         |
      | 2001:0000:1234:0000:00001:C1C0:ABCD:0876                                                                                                                                                                                                                                         |
      | 2001:0000:1234: 0000:0000:C1C0:ABCD:0876                                                                                                                                                                                                                                         |
      | 2001:0000:1234:0000:0000:C1C0:ABCD:0876 0                                                                                                                                                                                                                                        |
      | 3ffe:0b00:0000:0001:0000:0000:000a                                                                                                                                                                                                                                               |
      | FF02:0000:0000:0000:0000:0000:0000:0000:0001                                                                                                                                                                                                                                     |
      | ::1111:2222:3333:4444:5555:6666::                                                                                                                                                                                                                                                |
      | 0000:0000:0000:0000:0000:0000:0000:0000/                                                                                                                                                                                                                                         |
      | 0000:0000:0000:0000:0000:0000:0000:0000/012                                                                                                                                                                                                                                      |
      | 0000:0000:0000:0000:0000:0000:0000:0000/129                                                                                                                                                                                                                                      |
      | 0000:0000:0000:0000:0000:0000:0000:0000/130                                                                                                                                                                                                                                      |

  @notification_http_url_invalid @BUG_2006 @BUG_2009
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification http url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | <url>                   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                    |
      | error       | BadRequest                               |
      | description | forbidden characters in http field /url/ |
    Examples:
      | url                        |
      | http://34.56.45.34;3454    |
      | http://foo.bar/foo(bar)baz |

  @notification_http_url_invalid @BUG_2006 @BUG_2009
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification http url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
     # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | <url>                   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                    |
      | error       | BadRequest                               |
      | description | forbidden characters in http field /url/ |
    Examples:
      | url                        |
      | http://34.56.45.34;3454    |
      | http://foo.bar/foo(bar)baz |

  @notification_http_url_invalid_crash @BUG_2006 @BUG_2092
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification http url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | <url>                   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | Invalid URL parsing notification url |
    Examples:
      | url       |
      | //        |
      | ///       |
      | http:///a |

  @notification_http_url_forbidden @BUG_2006 @BUG_2093
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification http url field forbidden values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notification_http_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter             | value                   |
      | subject_idPattern     | .*                      |
      | condition_attrs       | temperature             |
      | notification_http_url | <url>                   |
      | notification_attrs    | temperature             |
      | expires               | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                    |
      | error       | BadRequest                               |
      | description | forbidden characters in http field /url/ |
    Examples:
      | url         |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  # ------------ notification - httpCustom field ---------------------
  @notification_httpCustom_without
  Scenario:  try to create a new subscription using NGSI v2 without notification - http field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter          | value                   |
      | subject_idPattern  | .*                      |
      | condition_attrs    | temperature             |
      | notification_attrs | temperature             |
      | expires            | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                        |
      | error       | BadRequest                   |
      | description | http notification is missing |

  # ------------ notification - httpCustom - url field ---------------------
  @notification_httpCustom_url_without
  Scenario:  try to create a new subscription using NGSI v2 without notification - httpCustom - url field
    Given  a definition of headers
      | parameter          | value                  |
      | Fiware-Service     | test_csub_notification |
      | Fiware-ServicePath | /test                  |
      | Content-Type       | application/json       |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                                     |
      | subject_idPattern            | .*                                        |
      | condition_attrs              | temperature                               |
      | notification_attrs           | temperature                               |
      | notification_http_custom_url | without notification httpCustom url field |
      | expires                      | 2016-04-05T14:00:00.00Z                   |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                  |
      | error       | BadRequest                             |
      | description | url httpCustom notification is missing |

  @notification_http_custom_url_hostname
  Scenario Outline:  create a new subscription using NGSI v2 with notification httpCustom url field values using a valid hostname
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_notification_http_custom_url |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value       |
      | subject_idPattern            | .*          |
      | condition_attrs              | temperature |
      | notification_http_custom_url | <url>       |
      | notification_attrs           | temperature |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | url                                                                                                                                                                                                                                                            |
      | http://localhost                                                                                                                                                                                                                                               |
      | http://es.localhost                                                                                                                                                                                                                                            |
      | http://es.localhost.my                                                                                                                                                                                                                                         |
      | http://es.localhost-45.my                                                                                                                                                                                                                                      |
      | http://localhost:1                                                                                                                                                                                                                                             |
      | http://localhost:65535/my_path                                                                                                                                                                                                                                 |
      | https://localhost:1234/my_path/multiple                                                                                                                                                                                                                        |
      | https://localhost-12345678901234567890123456789012345678901234567890123.my:1234/my_path/multiple                                                                                                                                                               |
      | https://localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost1234567890123456789012:1234 |
      | http://e34.56.45.34                                                                                                                                                                                                                                            |
      | http://34.56:3454                                                                                                                                                                                                                                              |
      | http://a.b--c.de/                                                                                                                                                                                                                                              |
      | http://0.0.0.0                                                                                                                                                                                                                                                 |
      | http://1.1.1.1.1                                                                                                                                                                                                                                               |
      | http://3441.2344.1231.1123                                                                                                                                                                                                                                     |
      | http://123.123.123                                                                                                                                                                                                                                             |
      | http://3628126748                                                                                                                                                                                                                                              |

  @notification_http_custom_url_ipv4
  Scenario Outline:  create a new subscription using NGSI v2 with notification httpCustom url field values using a valid ipv4 address
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_notification_http_custom_url |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value       |
      | subject_idPattern            | .*          |
      | condition_attrs              | temperature |
      | notification_http_custom_url | <url>       |
      | notification_attrs           | temperature |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | url                                       |
      | http://0.0.0.0                            |
      | http://255.255.255.255                    |
      | http://192.10.10.1                        |
      | http://192.10.10.1:1                      |
      | http://192.10.10.1:65535/my_path          |
      | https://192.10.10.1:1234/my_path/multiple |

  @notification_http_custom_url_ipv6 @BUG_2859 @skip
  Scenario Outline:  create a new subscription using NGSI v2 with notification httpCustom url field values using a valid ipv6 address
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_notification_http_custom_url |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value       |
      | subject_idPattern            | .*          |
      | condition_attrs              | temperature |
      | notification_http_custom_url | <url>       |
      | notification_attrs           | temperature |
    When create a new subscription
    Then verify that receive a "Created" http code
    And verify headers in response
      | parameter      | value                |
      | Location       | /v2/subscriptions/.* |
      | Content-Length | 0                    |
    And verify that the subscription is stored in mongo
    Examples:
      | url                                                            |
      | http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html |
      | http://[1080:0:0:0:8:800:200C:417A]/index.html                 |
      | http://[3ffe:2a00:100:7031::1]                                 |
      | http://[1080::8:800:200C:417A]/foo                             |
      | http://[::192.9.5.5]/ipng                                      |
      | http://[::FFFF:129.144.52.38]:80/index.html                    |
      | http://[2010:836B:4179::836B:4179]                             |

  @notification_http_custom_url_error_q_param
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification httpCustom url field values with quries parameters
    Given  a definition of headers
      | parameter          | value                             |
      | Fiware-Service     | test_notification_http_custom_url |
      | Fiware-ServicePath | /test                             |
      | Content-Type       | application/json                  |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_custom_url | <url>                   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | forbidden characters in custom /url/ |
    Examples:
      | url                                                                             |
      | https://192.10.10.1:1234/my_path/multiple?id=45&count=true                      |
      | https://localhost:1234/my_path/multiple?id=45&count=true                        |
      | http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html?id=45&count=true |

  @notification_http_custom_url_empty @BUG_2279
  Scenario:  try to create a new subscription using NGSI v2 with empty values in notification httpCustom url
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_http_custom_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
   # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_custom_url |                         |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                |
      | error       | BadRequest           |
      | description | invalid custom /url/ |

  @notification_http_custom_url_invalid_desc @BUG_2280
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification httpCustom url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_http_custom_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_custom_url | <url>                   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                |
      | error       | BadRequest           |
      | description | invalid custom /url/ |
    Examples:
      | url               |
      | ws://             |
      | ://localhost      |
      | http//localhost   |
      | http:/localhost   |
      | http:localhost    |
      | http://localhost: |
      | http://           |
      | //a               |
      | ///a              |
      | foo.com           |
      | rdar://1234       |
      | h://test          |
      | http://           |
      | ://               |
      | ftps://foo.bar/   |

  @notification_http_custom_url_invalid @BUG_2280 @skip
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification httpCustom url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_http_custom_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_custom_url | <url>                   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                |
      | error       | BadRequest           |
      | description | invalid custom /url/ |
    Examples:
      | url                                                                                                                                                                                                                                                                              |
      | http://my_host                                                                                                                                                                                                                                                                   |
      | http://my_host.server                                                                                                                                                                                                                                                            |
      | http://my_host-2                                                                                                                                                                                                                                                                 |
      | http://my_host-2.server                                                                                                                                                                                                                                                          |
      | http://my_host_2                                                                                                                                                                                                                                                                 |
      | http://my_host-2:2134                                                                                                                                                                                                                                                            |
      | http://my_host:3455/my_path                                                                                                                                                                                                                                                      |
      | http://my_host:3455/my_path/multiple                                                                                                                                                                                                                                             |
      | https://localhost123456789012345678901234567890123456789012345678901234567890.my:1234                                                                                                                                                                                            |
      | https://localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost123456789012345678901234567890123456789012345678901234567890.localhost1234567890123456789012345678901234567890:1234 |
      | http://localhost:900000                                                                                                                                                                                                                                                          |
      | http://localhost:dsfsdf                                                                                                                                                                                                                                                          |
      | http://localhost\my_path                                                                                                                                                                                                                                                         |
      | http://34,56.45.34                                                                                                                                                                                                                                                               |
      | http://.                                                                                                                                                                                                                                                                         |
      | http://..                                                                                                                                                                                                                                                                        |
      | http://../                                                                                                                                                                                                                                                                       |
      | http://?                                                                                                                                                                                                                                                                         |
      | http://??/                                                                                                                                                                                                                                                                       |
      | http://#                                                                                                                                                                                                                                                                         |
      | http://##/                                                                                                                                                                                                                                                                       |
      | http://-error-.invalid/                                                                                                                                                                                                                                                          |
      | http://_error_.invalid/                                                                                                                                                                                                                                                          |
      | http://-a.b.co                                                                                                                                                                                                                                                                   |
      | http://a.b-.co                                                                                                                                                                                                                                                                   |
      | http://.www.foo.bar/                                                                                                                                                                                                                                                             |
      | http://www.foo.bar./                                                                                                                                                                                                                                                             |
      | http://.www.foo.bar./                                                                                                                                                                                                                                                            |
      | http://192.168. 224.0                                                                                                                                                                                                                                                            |
      | http://192.168.224.0 1                                                                                                                                                                                                                                                           |
      | 3ffe:b00::1::a                                                                                                                                                                                                                                                                   |
      | 192.168.1.1/                                                                                                                                                                                                                                                                     |
      | 192.168.1.1/12.34                                                                                                                                                                                                                                                                |
      | 192.168.1.1/01                                                                                                                                                                                                                                                                   |
      | 192.168.1.1/33                                                                                                                                                                                                                                                                   |
      | 02001:0000:1234:0000:0000:C1C0:ABCD:0876                                                                                                                                                                                                                                         |
      | 2001:0000:1234:0000:00001:C1C0:ABCD:0876                                                                                                                                                                                                                                         |
      | 2001:0000:1234: 0000:0000:C1C0:ABCD:0876                                                                                                                                                                                                                                         |
      | 2001:0000:1234:0000:0000:C1C0:ABCD:0876 0                                                                                                                                                                                                                                        |
      | 3ffe:0b00:0000:0001:0000:0000:000a                                                                                                                                                                                                                                               |
      | FF02:0000:0000:0000:0000:0000:0000:0000:0001                                                                                                                                                                                                                                     |
      | ::1111:2222:3333:4444:5555:6666::                                                                                                                                                                                                                                                |
      | 0000:0000:0000:0000:0000:0000:0000:0000/                                                                                                                                                                                                                                         |
      | 0000:0000:0000:0000:0000:0000:0000:0000/012                                                                                                                                                                                                                                      |
      | 0000:0000:0000:0000:0000:0000:0000:0000/129                                                                                                                                                                                                                                      |
      | 0000:0000:0000:0000:0000:0000:0000:0000/130                                                                                                                                                                                                                                      |

  @notification_http_custom_url_invalid_crash @BUG_2280
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification httpCustom url field invalid values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_http_custom_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_custom_url | <url>                   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                |
      | error       | BadRequest           |
      | description | invalid custom /url/ |
    Examples:
      | url       |
      | //        |
      | ///       |
      | http:///a |

  @notification_http_custom_url_forbidden @BUG_2006 @BUG_2093
  Scenario Outline:  try to create a new subscription using NGSI v2 with notification httpCustom url field forbidden values
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_http_custom_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
    # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_custom_url | <url>                   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | forbidden characters in custom /url/ |
    Examples:
      | url         |
      | house<flat> |
      | house=flat  |
      | house"flat" |
      | house'flat' |
      | house;flat  |
      | house(flat) |

  @notification_http_custom_url_forbidden
  Scenario Outline:  try to create a new subscription using NGSI v2 with forbidden notification httpCustom url
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_http_custom_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_custom_url | <url>                   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | forbidden characters in custom /url/ |
    Examples:
      | url                        |
      | http://34.56.45.34;3454    |
      | http://foo.bar/foo(bar)baz |

  @notification_http_and_http_custom_url
  Scenario:  try to create a new subscription using NGSI v2 with notification http and httpCustom url field together
    Given  a definition of headers
      | parameter          | value                            |
      | Fiware-Service     | test_notif_http_custom_url_error |
      | Fiware-ServicePath | /test                            |
      | Content-Type       | application/json                 |
  # These properties below are used in subscriptions request
    And properties to subscriptions
      | parameter                    | value                   |
      | subject_idPattern            | .*                      |
      | condition_attrs              | temperature             |
      | notification_http_url        | http://localhost:1234   |
      | notification_http_custom_url | http://localhost:4567   |
      | notification_attrs           | temperature             |
      | expires                      | 2016-04-05T14:00:00.00Z |
    When create a new subscription
    Then verify that receive a "Bad Request" http code
    And verify an error response
      | parameter   | value                                |
      | error       | BadRequest                           |
      | description | notification has http and httpCustom |
  #  headers, qs, method, payload fields  in notification http still are not  defined their tests
