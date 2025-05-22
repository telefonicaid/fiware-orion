/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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


#include "serviceRoutines/logTraceTreat.h"
#include "serviceRoutines/versionTreat.h"
#include "serviceRoutines/statisticsTreat.h"
#include "serviceRoutines/exitTreat.h"
#include "serviceRoutines/leakTreat.h"
#include "serviceRoutines/optionsVersionRequest.h"
#include "serviceRoutines/badVerbPostOnly.h"
#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetPostOnly.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutines/badVerbPutOnly.h"
#include "serviceRoutines/badVerbGetPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetOnly.h"
#include "serviceRoutines/badRequest.h"


#include "serviceRoutinesV2/badVerbGetPutOnly.h"
#include "serviceRoutinesV2/badVerbGetDeletePatchOnly.h"
#include "serviceRoutinesV2/badVerbAllNotDelete.h"
#include "serviceRoutinesV2/getEntities.h"
#include "serviceRoutinesV2/entryPointsTreat.h"
#include "serviceRoutinesV2/getEntity.h"
#include "serviceRoutinesV2/getEntityAttribute.h"
#include "serviceRoutinesV2/putEntityAttribute.h"
#include "serviceRoutinesV2/getEntityAttributeValue.h"
#include "serviceRoutinesV2/putEntityAttributeValue.h"
#include "serviceRoutinesV2/postEntities.h"
#include "serviceRoutinesV2/putEntity.h"
#include "serviceRoutinesV2/postEntity.h"
#include "serviceRoutinesV2/deleteEntity.h"
#include "serviceRoutinesV2/getEntityType.h"
#include "serviceRoutinesV2/getEntityAllTypes.h"
#include "serviceRoutinesV2/patchEntity.h"
#include "serviceRoutinesV2/getAllSubscriptions.h"
#include "serviceRoutinesV2/getSubscription.h"
#include "serviceRoutinesV2/postSubscriptions.h"
#include "serviceRoutinesV2/deleteSubscription.h"
#include "serviceRoutinesV2/patchSubscription.h"
#include "serviceRoutinesV2/postBatchQuery.h"
#include "serviceRoutinesV2/postBatchUpdate.h"
#include "serviceRoutinesV2/logLevelTreat.h"
#include "serviceRoutinesV2/semStateTreat.h"
#include "serviceRoutinesV2/getMetrics.h"
#include "serviceRoutinesV2/deleteMetrics.h"
#include "serviceRoutinesV2/optionsGetOnly.h"
#include "serviceRoutinesV2/optionsGetPostOnly.h"
#include "serviceRoutinesV2/optionsGetDeleteOnly.h"
#include "serviceRoutinesV2/optionsAllNotDelete.h"
#include "serviceRoutinesV2/optionsGetPutOnly.h"
#include "serviceRoutinesV2/optionsGetPutDeleteOnly.h"
#include "serviceRoutinesV2/optionsGetDeletePatchOnly.h"
#include "serviceRoutinesV2/optionsPostOnly.h"

#include "serviceRoutinesV2/getRegistration.h"
#include "serviceRoutinesV2/deleteRegistration.h"
#include "serviceRoutinesV2/getRegistrations.h"
#include "serviceRoutinesV2/postRegistration.h"
#include "serviceRoutinesV2/postNotifyContext.h"

#include "rest/RestService.h"
#include "rest/rest.h"
#include "contextBroker/orionRestServices.h"



/* ****************************************************************************
*
* getServiceV - 
*/
static RestService getServiceV[] =
{
  { EntryPointsRequest,                            1, { "v2"                                                                           },  entryPointsTreat                                 },
  { EntitiesRequest,                               2, { "v2", "entities"                                                               },  getEntities                                      },
  { EntityRequest,                                 3, { "v2", "entities", "*"                                                          },  getEntity                                        },
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                                 },  getEntity                                        },
  { EntityAttributeValueRequest,                   6, { "v2", "entities", "*", "attrs", "*", "value"                                   },  getEntityAttributeValue                          },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                            },  getEntityAttribute                               },
  { EntityTypeRequest,                             3, { "v2", "types", "*"                                                             },  getEntityType                                    },
  { EntityAllTypesRequest,                         2, { "v2", "types"                                                                  },  getEntityAllTypes                                },
  { SubscriptionsRequest,                          2, { "v2", "subscriptions"                                                          },  getAllSubscriptions                              },
  { SubscriptionRequest,                           3, { "v2", "subscriptions", "*"                                                     },  getSubscription                                  },
  { RegistrationRequest,                           3, { "v2", "registrations", "*"                                                     },  getRegistration                                  },
  { RegistrationsRequest,                          2, { "v2", "registrations"                                                          },  getRegistrations                                 },
  { LogTraceRequest,                               2, { "log", "trace"                                                                 },  logTraceTreat                                    },
  { StatisticsRequest,                             1, { "statistics"                                                                   },  statisticsTreat                                  },
  { StatisticsRequest,                             2, { "cache", "statistics"                                                          },  statisticsCacheTreat                             },
  { VersionRequest,                                1, { "version"                                                                      },  versionTreat                                     },
  { LogLevelRequest,                               2, { "admin", "log"                                                                 },  getLogConfig                                     },
  { SemStateRequest,                               2, { "admin", "sem"                                                                 },  semStateTreat                                    },
  { MetricsRequest,                                2, { "admin", "metrics"                                                             },  getMetrics                                       },

#ifdef DEBUG
  { ExitRequest,                                   2, { "exit", "*"                                                                    },  exitTreat                                        },
  { ExitRequest,                                   1, { "exit"                                                                         },  exitTreat                                        },
  { LeakRequest,                                   2, { "leak", "*"                                                                    },  leakTreat                                        },
  { LeakRequest,                                   1, { "leak"                                                                         },  leakTreat                                        },
#endif

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* postServiceV - 
*/
static RestService postServiceV[] =
{
  { EntitiesRequest,                               2, { "v2", "entities"                                                               }, postEntities                                      },
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                                 }, postEntity                                        },
  { NotifyContext,                                 3, { "v2", "op", "notify"                                                           }, postNotifyContext                                 },
  { BatchQueryRequest,                             3, { "v2", "op", "query"                                                            }, postBatchQuery                                    },
  { BatchUpdateRequest,                            3, { "v2", "op", "update"                                                           }, postBatchUpdate                                   },
  { SubscriptionsRequest,                          2, { "v2", "subscriptions"                                                          }, postSubscriptions                                 },
  { RegistrationsRequest,                          2, { "v2", "registrations"                                                          }, postRegistration                                  },  

  ORION_REST_SERVICE_END
};


/* ****************************************************************************
*
* putServiceV - 
*/
static RestService putServiceV[] =
{
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                               }, putEntity                                        },
  { EntityAttributeValueRequest,                   6, { "v2", "entities", "*", "attrs", "*", "value"                                 }, putEntityAttributeValue                          },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                          }, putEntityAttribute                               },
  { LogTraceRequest,                               3, { "log", "trace",      "*"                                                     }, logTraceTreat                                    },
  { LogLevelRequest,                               2, { "admin", "log"                                                               }, changeLogConfig                                  },

  ORION_REST_SERVICE_END
};


/* ****************************************************************************
*
* patchServiceV - 
*/
static RestService patchServiceV[] =
{
  { EntityRequest,       4, { "v2", "entities", "*", "attrs" }, patchEntity       },
  { SubscriptionRequest, 3, { "v2", "subscriptions", "*"     }, patchSubscription },

  ORION_REST_SERVICE_END
};


/* ****************************************************************************
*
* deleteServiceV - 
*/
static RestService deleteServiceV[] =
{
  { EntityRequest,                                 3, { "v2", "entities", "*"                                                        }, deleteEntity                                        },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                          }, deleteEntity                                        },
  { SubscriptionRequest,                           3, { "v2", "subscriptions", "*"                                                   }, deleteSubscription                                  },
  { RegistrationRequest,                           3, { "v2", "registrations", "*"                                                   }, deleteRegistration                                  },
  { LogTraceRequest,                               2, { "log", "trace"                                                               }, logTraceTreat                                       },
  { LogTraceRequest,                               3, { "log", "trace",      "*"                                                     }, logTraceTreat                                       },
  { StatisticsRequest,                             1, { "statistics"                                                                 }, statisticsTreat                                     },
  { StatisticsRequest,                             2, { "cache", "statistics"                                                        }, statisticsCacheTreat                                },
  { MetricsRequest,                                2, { "admin", "metrics"                                                           }, deleteMetrics                                       },

  ORION_REST_SERVICE_END
};


/* ****************************************************************************
*
* badVerbV - 
*/
static RestService badVerbV[] =
{
  { EntryPointsRequest,                            1, { "v2"                                                                           }, badVerbGetOnly            },
  { EntitiesRequest,                               2, { "v2", "entities"                                                               }, badVerbGetPostOnly        },
  { EntityRequest,                                 3, { "v2", "entities", "*"                                                          }, badVerbGetDeleteOnly      },
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                                 }, badVerbAllNotDelete       },
  { EntityAttributeValueRequest,                   6, { "v2", "entities", "*", "attrs", "*", "value"                                   }, badVerbGetPutOnly         },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                            }, badVerbGetPutDeleteOnly   },
  { EntityTypeRequest,                             3, { "v2", "types", "*"                                                             }, badVerbGetOnly            },
  { EntityAllTypesRequest,                         2, { "v2", "types"                                                                  }, badVerbGetOnly            },
  { SubscriptionsRequest,                          2, { "v2", "subscriptions"                                                          }, badVerbGetPostOnly        },
  { SubscriptionRequest,                           3, { "v2", "subscriptions", "*"                                                     }, badVerbGetDeletePatchOnly },
  { BatchQueryRequest,                             3, { "v2", "op", "query"                                                            }, badVerbPostOnly           },
  { BatchUpdateRequest,                            3, { "v2", "op", "update"                                                           }, badVerbPostOnly           },
  { RegistrationRequest,                           3, { "v2", "registrations", "*"                                                     }, badVerbGetDeleteOnly      },
  { RegistrationsRequest,                          2, { "v2", "registrations"                                                          }, badVerbGetPostOnly        },
  { LogTraceRequest,                               2, { "log", "trace"                                                                 }, badVerbGetDeleteOnly      },
  { LogTraceRequest,                               3, { "log", "trace",      "*"                                                       }, badVerbPutDeleteOnly      },
  { StatisticsRequest,                             1, { "statistics"                                                                   }, badVerbGetDeleteOnly      },
  { StatisticsRequest,                             2, { "cache", "statistics"                                                          }, badVerbGetDeleteOnly      },
  { VersionRequest,                                1, { "version"                                                                      }, badVerbGetOnly            },
  { LogLevelRequest,                               2, { "admin", "log"                                                                 }, badVerbPutOnly            },
  { SemStateRequest,                               2, { "admin", "sem"                                                                 }, badVerbGetOnly            },
  { MetricsRequest,                                2, { "admin", "metrics"                                                             }, badVerbGetDeleteOnly      },

  { InvalidRequest,                                0, { "*", "*", "*", "*", "*", "*"                                                   }, badRequest                },
  { InvalidRequest,                                0, {                                                                                }, NULL                      },

  ORION_REST_SERVICE_END
};


/* ****************************************************************************
*
* optionsV - 
*
*/
static RestService optionsV[] =
{
  { EntryPointsRequest,            1, { "v2"                                         }, optionsGetOnly            },
  { EntitiesRequest,               2, { "v2", "entities"                             }, optionsGetPostOnly        },
  { EntityRequest,                 3, { "v2", "entities", "*"                        }, optionsGetDeleteOnly      },
  { EntityRequest,                 4, { "v2", "entities", "*", "attrs"               }, optionsAllNotDelete       },
  { EntityAttributeValueRequest,   6, { "v2", "entities", "*", "attrs", "*", "value" }, optionsGetPutOnly         },
  { EntityAttributeRequest,        5, { "v2", "entities", "*", "attrs", "*"          }, optionsGetPutDeleteOnly   },
  { EntityTypeRequest,             3, { "v2", "types", "*"                           }, optionsGetOnly            },
  { EntityAllTypesRequest,         2, { "v2", "types"                                }, optionsGetOnly            },
  { SubscriptionsRequest,          2, { "v2", "subscriptions"                        }, optionsGetPostOnly        },
  { SubscriptionRequest,           3, { "v2", "subscriptions", "*"                   }, optionsGetDeletePatchOnly },
  { BatchQueryRequest,             3, { "v2", "op", "query"                          }, optionsPostOnly           },
  { BatchUpdateRequest,            3, { "v2", "op", "update"                         }, optionsPostOnly           },
  { RegistrationRequest,           3, { "v2", "registrations", "*"                   }, optionsGetDeleteOnly      },
  { RegistrationsRequest,          2, { "v2", "registrations"                        }, optionsGetPostOnly        },
  { VersionRequest,                1, { "version"                                    }, optionsVersionRequest     },

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* orionRestServicesInit - 
*/
void orionRestServicesInit
(
   IpVersion           ipVersion,
   const char*         bindAddress,
   unsigned short      port,
   bool                multitenant,
   unsigned int        connectionMemory,
   unsigned int        maxConnections,
   unsigned int        mhdThreadPoolSize,
   const char*         allowedOrigin,
   int                 corsMaxAge,
   int                 mhdTimeoutInSeconds,
   const char*         httpsKey,
   const char*         httpsCert
)
{
  // Use options service vector (optionsServiceV) only when CORS is enabled
  RestService* optionsServiceV  = (strlen(allowedOrigin) > 0) ? optionsV : NULL;

  restInit(getServiceV,
           putServiceV,
           postServiceV,
           patchServiceV,
           deleteServiceV,
           optionsServiceV,
           badVerbV,
           ipVersion,
           bindAddress,
           port,
           multitenant,
           connectionMemory,
           maxConnections,
           mhdThreadPoolSize,
           allowedOrigin,
           corsMaxAge,
           mhdTimeoutInSeconds,
           httpsKey,
           httpsCert);
}
