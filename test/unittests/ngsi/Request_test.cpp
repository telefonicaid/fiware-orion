/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/Request.h"



typedef struct Req
{
   RequestType  rt;
   const char*  string;
} Req;

/* ****************************************************************************
*
* requestType - 
*/
TEST(Request, requestType)
{
  Req   req[] =
  {
    { RegisterContext,                                 "RegisterContextRequest"                                 },
    { DiscoverContextAvailability,                     "DiscoverContextAvailabilityRequest"                     },    

    { QueryContext,                                "QueryContextRequest"                                    },
    { SubscribeContext,                            "SubscribeContextRequest"                                },
    { UpdateContextSubscription,                   "UpdateContextSubscriptionRequest"                       },
    { UnsubscribeContext,                          "UnsubscribeContextRequest"                              },
    { NotifyContext,                               "NotifyContextRequest"                                   },
    { UpdateContext,                               "UpdateContextRequest"                                   },

    { ContextEntitiesByEntityId,                   "ContextEntitiesByEntityId"                              },
    { ContextEntityAttributes,                     "ContextEntityAttributes"                                },
    { ContextEntityTypes,                          "ContextEntityTypes"                                     },
    { ContextEntityTypeAttributeContainer,         "ContextEntityTypeAttributeContainer"                    },
    { ContextEntityTypeAttribute,                  "ContextEntityTypeAttribute"                             },
    { EntityByIdAttributeByName,                   "EntityByIdAttributeByName"                              },

    { IndividualContextEntity,                     "IndividualContextEntity"                                },
    { IndividualContextEntityAttributes,           "IndividualContextEntityAttributes"                      },
    { IndividualContextEntityAttribute,            "IndividualContextEntityAttribute"                       },
    { AttributeValueInstance,                      "AttributeValueInstance"                                 },
    { UpdateContextElement,                        "UpdateContextElement"                                   },
    { AppendContextElement,                        "AppendContextElement"                                   },
    { UpdateContextAttribute,                      "UpdateContextAttribute"                                 },
    { Ngsi10ContextEntityTypes,                    "Ngsi10ContextEntityTypes"                               },
    { Ngsi10ContextEntityTypesAttributeContainer,  "Ngsi10ContextEntityTypesAttributeContainer"             },
    { Ngsi10ContextEntityTypesAttribute,           "Ngsi10ContextEntityTypesAttribute"                      },
    { Ngsi10SubscriptionsConvOp,                   "Ngsi10SubscriptionsConvOp"                              },

    { LogTraceRequest,                             "LogTrace"                                               },
    { LogLevelRequest,                             "LogLevel"                                               },
    { VersionRequest,                              "Version"                                                },
    { StatisticsRequest,                           "Statistics"                                             },
    { ExitRequest,                                 "Exit"                                                   },
    { LeakRequest,                                 "Leak"                                                   },
    { RegisterResponse,                            "RegisterContextResponse"                                },

    { RtSubscribeResponse,                         "SubscribeResponse"                                      },
    { RtSubscribeError,                            "SubscribeError"                                         },

    { EntityTypes,                                 "EntityTypes"                                            },
    { AttributesForEntityType,                     "AttributesForEntityType"                                },
    { RtEntityTypesResponse,                       "EntityTypesResponse"                                    },
    { RtAttributesForEntityTypeResponse,           "AttributesForEntityTypeResponse"                        },

    { InvalidRequest,                              "InvalidRequest"                                         },

  };

  for (unsigned int ix = 0; ix < sizeof(req) / sizeof(req[0]); ++ix)
  {
    EXPECT_STREQ(req[ix].string, requestType(req[ix].rt));
  }

  RequestType rt = (RequestType) (InvalidRequest + 1);
  EXPECT_STREQ("", requestType(rt));
}
