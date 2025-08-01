/*
 # Copyright 2024 Telefonica Investigacion y Desarrollo, S.A.U
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
 */

db.getSiblingDB("orion-validation").entities.insertMany([
    {
      "_id": {
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule10.1: missing entity id",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule10.2",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule10.2: missing entity type",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule10.3",
        "type": "T"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule10.3: missing entity servicePath",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule11.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrs": {
        "desc": {
          "value": "Rule11.1: missing attrNames",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule11.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule11.2: missing entity creDate",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule11.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule11.3: missing entity modDate",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule12.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule12.1: missing attribute mdNames",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          }
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule12.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule12.2: missing attribute creDate",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule12.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule12.3: missing attribute modDate",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule13.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule13.1: attribute in attrsName but not in attrs object",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule13.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule13.2: attribute in attrs object but not in attrNames",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule13.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule13.3: attribute in attrsName but not in attrs object with dot in the name",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule13.4",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2"
      ],
      "attrs": {
        "desc": {
          "value": "Rule13.4: attribute in attrs object but not in attrNames with dot in the name",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule14.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule14.1: md in mdNames but not in md object",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule14.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule14.2: md in md object but not in mdNames",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule14.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule14.3: md in mdNames but not in md object with dot in the name",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule14.4",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule14.4: md in md object but not in mdNames with dot in the name",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule15",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule15.1: conflicting entity",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "type": "T",  
        "id": "Rule15",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule15.2: conflicting entity",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "servicePath": "/SS",
        "id": "Rule15",
        "type": "T"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule15.3: conflicting entity",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.1",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location",
        "ignoredLocation",
        "otherLocation"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.1: more than one geo-attribute",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "mdNames": []
        },
        "ignoredLocation": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "md": {
            "ignoreType": {
              "type": "Bool",
              "value": true
            }
          },
          "mdNames": [
            "ignoreType"
          ]
        },    
        "otherLocation": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "mdNames": []
        }
      },
      "creDate": 1705933908.9073899,
      "modDate": 1705933908.9073899,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            2,
            1
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.2",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "ignoredLocation"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.2: no geo-attribute but location field",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "ignoredLocation": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "md": {
            "ignoreType": {
              "type": "Bool",
              "value": true
            }
          },
          "mdNames": [
            "ignoreType"
          ]
        }
      },
      "creDate": 1705933908.9073899,
      "modDate": 1705933908.9073899,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            2,
            1
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.3",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location",
        "ignoredLocation"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.3: geo-attribute with null value and location field",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": null,
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "mdNames": []
        },
        "ignoredLocation": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "md": {
            "ignoreType": {
              "type": "Bool",
              "value": true
            }
          },
          "mdNames": [
            "ignoreType"
          ]
        }
      },
      "creDate": 1705933908.9073899,
      "modDate": 1705933908.9073899,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            2,
            1
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.4",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location",
        "ignoredLocation"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.4: geo-attribute but not location field",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "mdNames": []
        },
        "ignoredLocation": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "md": {
            "ignoreType": {
              "type": "Bool",
              "value": true
            }
          },
          "mdNames": [
            "ignoreType"
          ]
        }
      },
      "creDate": 1705933908.9073899,
      "modDate": 1705933908.9073899,
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.5",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location",
        "ignoredLocation"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.5: location.attrName inconsistency",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "mdNames": []
        },
        "ignoredLocation": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "md": {
            "ignoreType": {
              "type": "Bool",
              "value": true
            }
          },
          "mdNames": [
            "ignoreType"
          ]
        }
      },
      "creDate": 1705933908.9073899,
      "modDate": 1705933908.9073899,
      "location": {
        "attrName": "ignoredLocation",
        "coords": {
          "type": "Point",
          "coordinates": [
            2,
            1
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.6",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.6: geo:point coords inconsistency",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "mdNames": []
        }
      },
      "creDate": 1705933908.9073899,
      "modDate": 1705933908.9073899,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            20,
            1
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.7",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.7: geo:line coords inconsistency",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": [
            "1, 2",
            "3, 4"
          ],
          "type": "geo:line",
          "creDate": 1705933908.9088006,
          "modDate": 1705933908.9088006,
          "mdNames": []
        }
      },
      "creDate": 1705933908.9088006,
      "modDate": 1705933908.9088006,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "LineString",
          "coordinates": [
            [
              2,
              1
            ],
            [
              40,
              3
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.8",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.8: geo:box coords inconsistency",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },  
        "location": {
          "value": [
            "1, 2",
            "3, 4"
          ],
          "type": "geo:box",
          "creDate": 1705933908.9101331,
          "modDate": 1705933908.9101331,
          "mdNames": []
        }
      },
      "creDate": 1705933908.9101331,
      "modDate": 1705933908.9101331,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                2,
                1
              ],
              [
                2,
                3
              ],
              [
                4,
                30
              ],
              [
                4,
                1
              ],
              [
                2,
                1
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.9",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.9: geo:polygon inconsistency",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },  
        "location": {
          "value": [
            "1, 2",
            "10, 20",
            "10, -20",
            "1, 2"
          ],
          "type": "geo:polygon",
          "creDate": 1705933908.9129946,
          "modDate": 1705933908.9129946,
          "mdNames": []
        }
      },
      "creDate": 1705933908.9129946,
      "modDate": 1705933908.9129946,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                2,
                1
              ],
              [
                200,
                10
              ],
              [
                -20,
                10
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.10",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.10: geo:json coords inconsistency",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },  
        "location": {
          "value": {
            "type": "Polygon",
            "coordinates": [
              [
                [
                  1,
                  2
                ],
                [
                  10,
                  20
                ],
                [
                  10,
                  -20
                ],
                [
                  1,
                  2
                ]
              ]
            ]
          },
          "type": "geo:json",
          "creDate": 1705933908.9142942,
          "modDate": 1705933908.9142942,
          "mdNames": []
        }
      },
      "creDate": 1705933908.9142942,
      "modDate": 1705933908.9142942,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                1,
                2
              ],
              [
                10,
                20
              ],
              [
                10,
                -200
              ],
              [
                1,
                2
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.11",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.11: geo:json coords strings",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },  
        "location": {
          "value": {
            "type": "Polygon",
            "coordinates": [
              [
                [
                  "1",
                  "2"
                ],
                [
                  "10",
                  "20"
                ],
                [
                  "10",
                  "-20"
                ],
                [
                  "1",
                  "2"
                ]
              ]
            ]
          },
          "type": "geo:json",
          "creDate": 1705933908.9142942,
          "modDate": 1705933908.9142942,
          "mdNames": []
        }
      },
      "creDate": 1705933908.9142942,
      "modDate": 1705933908.9142942,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                1,
                2
              ],
              [
                10,
                20
              ],
              [
                10,
                -20
              ],
              [
                1,
                2
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.12",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.12: Feature without geometry",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": {
            "type": "Feature",
            "properties": {
              "label": "-3.6127119138731127, 40.53901978067972"
            }
          },
          "type": "geo:json",
          "creDate": 1706274323.1297336,
          "modDate": 1706274323.1297336,
          "mdNames": []
        }
      },
      "creDate": 1706274323.1297336,
      "modDate": 1706274323.1297336,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            -3.6127119138731127,
            40.53901978067972
          ]
        }
      },
      "lastCorrelator": "90b31496-bc4b-11ee-ab64-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.13",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.13: FeatureCollection without features element",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": {
            "type": "FeatureCollection"
          },
          "type": "geo:json",
          "creDate": 1706274323.130671,
          "modDate": 1706274323.130671,
          "mdNames": []
        }
      },
      "creDate": 1706274323.130671,
      "modDate": 1706274323.130671,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            -3.6127119138731127,
            40.53901978067972
          ]
        }
      },
      "lastCorrelator": "90b31496-bc4b-11ee-ab64-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.14",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.14: FeatureCollection with 0 Feature",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": {
            "type": "FeatureCollection",
            "features": [
            ]
          },
          "type": "geo:json",
          "creDate": 1706274323.130671,
          "modDate": 1706274323.130671,
          "mdNames": []
        }
      },
      "creDate": 1706274323.130671,
      "modDate": 1706274323.130671,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            -3.6127119138731127,
            40.53901978067972
          ]
        }
      },
      "lastCorrelator": "90b31496-bc4b-11ee-ab64-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.15",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.15: FeatureCollection with more than one Feature",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": {
            "type": "FeatureCollection",
            "features": [
              {
                "type": "Feature",
                "properties": {},
                "geometry": {
                  "type": "Point",
                  "coordinates": [
                    -3.6127119138731127,
                    40.53901978067972
                  ]
                }
              },
              {
                "type": "Feature",
                "properties": {},
                "geometry": {
                  "type": "Point",
                  "coordinates": [
                    -3.6127119138731127,
                    40.53901978067972
                  ]
                }
              }
            ]
          },
          "type": "geo:json",
          "creDate": 1706274323.130671,
          "modDate": 1706274323.130671,
          "mdNames": []
        }
      },
      "creDate": 1706274323.130671,
      "modDate": 1706274323.130671,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            -3.6127119138731127,
            40.53901978067972
          ]
        }
      },
      "lastCorrelator": "90b31496-bc4b-11ee-ab64-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule16.16",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule16.16: FeatureCollection with Feature without geometry",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": {
            "type": "FeatureCollection",
            "features": [
              {
                "type": "Feature",
                "properties": {}
              }
            ]
          },
          "type": "geo:json",
          "creDate": 1706274323.130671,
          "modDate": 1706274323.130671,
          "mdNames": []
        }
      },
      "creDate": 1706274323.130671,
      "modDate": 1706274323.130671,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            -3.6127119138731127,
            40.53901978067972
          ]
        }
      },
      "lastCorrelator": "90b31496-bc4b-11ee-ab64-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule17.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule17.1: missing lastCorrelator field",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858
    },
    {
      "_id": {
        "id": "",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule20.1: id syntax minimum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule20.2xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule20.2: id syntax maximum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule20.3(",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule20.3: id syntax general forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule20.4#",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule20.4: id syntax identifier forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule21.1",
        "type": "",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule21.1: type syntax minimum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule21.2",
        "type": "Txxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule21.2: type syntax maximum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule21.3",
        "type": "T(",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule21.3: type syntax general forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule21.4",
        "type": "T#",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule21.4: type syntax identifier forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule22.1",
        "type": "T",
        "servicePath": "SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule21.1: servicePath does not starts with slash",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule22.2",
        "type": "T",
        "servicePath": "/S1/S2/S3/S4/S5/S6/S7/S8/S9/S10/S11"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule21.2: more than 10 levels in servicePath",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule22.3",
        "type": "T",
        "servicePath": "/S1//S3/"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule22.3: servicePath level less than minimum",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule22.4",
        "type": "T",
        "servicePath": "/S1/Sxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/S3/"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule22.4: servicePath level greater than maximum",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule22.5",
        "type": "T",
        "servicePath": "/S1/S#/S3/"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule22.5: servicePath syntax problem",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule23.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule23.1: attr name syntax minimum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule23.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule23.2: attr name syntax maximum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule23.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1(",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule23.3: attr name syntax general forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1(": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule23.4",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1#",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule23.4: attr name syntax identifier forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1#": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule24.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule24.1: attr type syntax minimum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule24.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule24.2: attr type syntax maximum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Numberxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule24.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule24.3: attr type syntax general forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number(",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule24.4",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule24.4: attr type syntax identifier forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number#",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule24.5",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule24.5: attr type is missing",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },    
    {
      "_id": {
        "id": "Rule25.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule25.1: md name syntax minimum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule25.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule25.2: md name syntax maximum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule25.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule25.3: md name syntax general forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1(": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1(",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule25.4",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule25.4: md name syntax identifier forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1#": {
              "type": "Number",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1#",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule26.1",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule26.1: md type syntax minimum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule26.2",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule26.2: md type syntax maximum length",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Numberxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule26.3",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule26.3: md type syntax general forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number(",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule26.4",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule26.4: md type syntax identifier forbidden",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "type": "Number#",
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule26.5",
        "type": "T",
        "servicePath": "/SS"
      },
      "attrNames": [
        "desc",
        "A1",
        "A2",
        "A.3"
      ],
      "attrs": {
        "desc": {
          "value": "Rule26.5: md type is missing",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A1": {
          "value": 10,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "md": {
            "MD1": {
              "value": 100
            },
            "MD=2": {
              "type": "Number",
              "value": 200
            }
          },
          "mdNames": [
            "MD1",
            "MD.2"
          ]
        },
        "A2": {
          "value": 20,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "A=3": {
          "value": 30,
          "type": "Number",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        }
      },
      "creDate": 1705931202.187858,
      "modDate": 1705931202.187858,
      "lastCorrelator": "acae5f4c-b92c-11ee-8f0c-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule90.1",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule90.1: usage of legacy geo:point",
          "type": "Text",
          "creDate": 1706006146.1601377,
          "modDate": 1706006146.1601377,
          "mdNames": []
        },
        "location": {
          "value": "1, 2",
          "type": "geo:point",
          "creDate": 1706006146.1601377,
          "modDate": 1706006146.1601377,
          "mdNames": []
        }
      },
      "creDate": 1706006146.1601377,
      "modDate": 1706006146.1601377,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            2,
            1
          ]
        }
      },
      "lastCorrelator": "2ac4f6a8-b9db-11ee-8c49-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule90.2",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule90.2: usage of legacy geo:line",
          "type": "Text",
          "creDate": 1706006146.17774,
          "modDate": 1706006146.17774,
          "mdNames": []
        },
        "location": {
          "value": [
            "1, 2",
            "3, 4"
          ],
          "type": "geo:line",
          "creDate": 1706006146.17774,
          "modDate": 1706006146.17774,
          "mdNames": []
        }
      },
      "creDate": 1706006146.17774,
      "modDate": 1706006146.17774,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "LineString",
          "coordinates": [
            [
              2,
              1
            ],
            [
              4,
              3
            ]
          ]
        }
      },
      "lastCorrelator": "2ac4f6a8-b9db-11ee-8c49-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule90.3",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule90.3: usage of legacy geo:box",
          "type": "Text",
          "creDate": 1706006146.179189,
          "modDate": 1706006146.179189,
          "mdNames": []
        },
        "location": {
          "value": [
            "1, 2",
            "3, 4"
          ],
          "type": "geo:box",
          "creDate": 1706006146.179189,
          "modDate": 1706006146.179189,
          "mdNames": []
        }
      },
      "creDate": 1706006146.179189,
      "modDate": 1706006146.179189,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                2,
                1
              ],
              [
                2,
                3
              ],
              [
                4,
                3
              ],
              [
                4,
                1
              ],
              [
                2,
                1
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "2ac4f6a8-b9db-11ee-8c49-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule90.4",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule90.4: usage of legacy geo:polygon",
          "type": "Text",
          "creDate": 1706006146.1812057,
          "modDate": 1706006146.1812057,
          "mdNames": []
        },
        "location": {
          "value": [
            "1, 2",
            "10, 20",
            "10, -20",
            "1, 2"
          ],
          "type": "geo:polygon",
          "creDate": 1706006146.1812057,
          "modDate": 1706006146.1812057,
          "mdNames": []
        }
      },
      "creDate": 1706006146.1812057,
      "modDate": 1706006146.1812057,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                2,
                1
              ],
              [
                20,
                10
              ],
              [
                -20,
                10
              ],
              [
                2,
                1
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "2ac4f6a8-b9db-11ee-8c49-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule91.1",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location",
        "ignoredLocation"
      ],
      "attrs": {
        "desc": {
          "value": "Rule91.1: no more than one location metadata",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },  
        "location": {
          "value": {
            "type": "Polygon",
            "coordinates": [
              [
                [
                  1,
                  2
                ],
                [
                  10,
                  20
                ],
                [
                  10,
                  -20
                ],
                [
                  1,
                  2
                ]
              ]
            ]
          },
          "type": "polygon",
          "creDate": 1705933908.9142942,
          "modDate": 1705933908.9142942,
          "md": {
            "location": {
              "type": "string",
              "value": "WSG84"
            }
          },
          "mdNames": [
            "location"
          ]
        },
        "ignoredLocation": {
          "value": null,
          "type": "point",
          "creDate": 1705933908.9073899,
          "modDate": 1705933908.9073899,
          "md": {
            "ignoreType": {
              "type": "Bool",
              "value": true
            },
            "location": {
              "type": "string",
              "value": "WSG84"
            }
          },
          "mdNames": [
            "ignoreType",
            "location"
          ]
        } 
      },
      "creDate": 1705933908.9142942,
      "modDate": 1705933908.9142942,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                1,
                2
              ],
              [
                10,
                20
              ],
              [
                10,
                -20
              ],
              [
                1,
                2
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule92.1",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule92.1: location must be WSG84 or WGS84",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },  
        "location": {
          "value": {
            "type": "Polygon",
            "coordinates": [
              [
                [
                  1,
                  2
                ],
                [
                  10,
                  20
                ],
                [
                  10,
                  -20
                ],
                [
                  1,
                  2
                ]
              ]
            ]
          },
          "type": "polygon",
          "creDate": 1705933908.9142942,
          "modDate": 1705933908.9142942,
          "md": {
            "location": {
              "type": "string",
              "value": "WSG99"
            }
          },
          "mdNames": [
            "location"
          ]
        }
      },
      "creDate": 1705933908.9142942,
      "modDate": 1705933908.9142942,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                1,
                2
              ],
              [
                10,
                20
              ],
              [
                10,
                -20
              ],
              [
                1,
                2
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule93.1",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule93.1: redundant metadata location",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },  
        "location": {
          "value": {
            "type": "Polygon",
            "coordinates": [
              [
                [
                  1,
                  2
                ],
                [
                  10,
                  20
                ],
                [
                  10,
                  -20
                ],
                [
                  1,
                  2
                ]
              ]
            ]
          },
          "type": "geo:json",
          "creDate": 1705933908.9142942,
          "modDate": 1705933908.9142942,
          "md": {
            "location": {
              "type": "string",
              "value": "WSG84"
            }
          },
          "mdNames": [
            "location"
          ]
        }
      },
      "creDate": 1705933908.9142942,
      "modDate": 1705933908.9142942,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Polygon",
          "coordinates": [
            [
              [
                1,
                2
              ],
              [
                10,
                20
              ],
              [
                10,
                -20
              ],
              [
                1,
                2
              ]
            ]
          ]
        }
      },
      "lastCorrelator": "fa02fa68-b932-11ee-a0fb-080027cd35f1"
    },
    {
      "_id": {
        "id": "Rule94.1",
        "type": "T",
        "servicePath": "/"
      },
      "attrNames": [
        "desc",
        "location"
      ],
      "attrs": {
        "desc": {
          "value": "Rule94.1: non redundant location metadata",
          "type": "Text",
          "creDate": 1705931202.187858,
          "modDate": 1705931202.187858,
          "mdNames": []
        },
        "location": {
          "value": "40.418889, -3.691944",
          "type": "coords",
          "creDate": 1706007163.0475833,
          "modDate": 1706007163.0475833,
          "md": {
            "location": {
              "type": "string",
              "value": "WSG84"
            }
          },
          "mdNames": [
            "location"
          ]
        }
      },
      "creDate": 1706007163.0475833,
      "modDate": 1706007163.0475833,
      "location": {
        "attrName": "location",
        "coords": {
          "type": "Point",
          "coordinates": [
            -3.691944,
            40.418889
          ]
        }
      },
      "lastCorrelator": "88e1bc10-b9dd-11ee-8755-080027cd35f1"
    }    
])

db.getSiblingDB("orion-validation").csubs.insertMany([
    {
      "expiration": NumberLong("9223372036854775807"),
      "reference": "http://notify-receptor:5055/notify",
      "custom": true,
      "timeout": NumberLong("0"),
      "headers": {
        "fiware-servicepath": "/SS1"
      },
      "throttling": NumberLong("0"),
      "maxFailsLimit": NumberLong("-1"),
      "servicePath": "/SS2",
      "status": "active",
      "statusLastChange": 1682640509.6958137,
      "entities": [
        {
          "id": ".*",
          "isPattern": "true",
          "type": "T",
          "isTypePattern": false
        }
      ],
      "attrs": [
        "temperature",
      ],
      "metadata": [],
      "blacklist": false,
      "onlyChanged": false,
      "covered": false,
      "description": "Foobar",
      "conditions": [
        "TimeInstant"
      ],
      "expression": {
        "q": "",
        "mq": "",
        "geometry": "",
        "coords": "",
        "georel": ""
      },
      "altTypes": [],
      "format": "JSON"
    }
])
