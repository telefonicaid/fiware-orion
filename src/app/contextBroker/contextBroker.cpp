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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>                             // getppid, for, setuid, etc.
#include <fcntl.h>                              // open
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "mongoBackend/MongoGlobal.h"

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "xmlParse/xmlRequest.h"
#include "jsonParse/jsonRequest.h"
#include "rest/ConnectionInfo.h"
#include "rest/RestService.h"
#include "rest/restReply.h"
#include "rest/rest.h"

#include "common/sem.h"
#include "common/globals.h"
#include "common/Timer.h"
#include "common/compileInfo.h"

#include "serviceRoutines/logVerboseTreat.h"
#include "serviceRoutines/logTraceTreat.h"

#include "ngsi/ParseData.h"
#include "ngsiNotify/onTimeIntervalThread.h"

#include "serviceRoutines/versionTreat.h"
#include "serviceRoutines/statisticsTreat.h"
#include "serviceRoutines/exitTreat.h"
#include "serviceRoutines/leakTreat.h"
#include "serviceRoutines/postDiscoverContextAvailability.h"
#include "serviceRoutines/postQueryContext.h"
#include "serviceRoutines/postRegisterContext.h"
#include "serviceRoutines/postSubscribeContext.h"
#include "serviceRoutines/postSubscribeContextAvailability.h"
#include "serviceRoutines/postUnsubscribeContextAvailability.h"
#include "serviceRoutines/postUpdateContext.h"
#include "serviceRoutines/postUpdateContextAvailabilitySubscription.h"
#include "serviceRoutines/postUpdateContextSubscription.h"
#include "serviceRoutines/postUnsubscribeContext.h"
#include "serviceRoutines/postNotifyContext.h"
#include "serviceRoutines/postNotifyContextAvailability.h"

#include "serviceRoutines/getContextEntitiesByEntityId.h"
#include "serviceRoutines/postContextEntitiesByEntityId.h"
#include "serviceRoutines/getContextEntityAttributes.h"
#include "serviceRoutines/postContextEntityAttributes.h"
#include "serviceRoutines/getEntityByIdAttributeByName.h"
#include "serviceRoutines/postEntityByIdAttributeByName.h"
#include "serviceRoutines/getContextEntityTypes.h"
#include "serviceRoutines/postContextEntityTypes.h"
#include "serviceRoutines/getContextEntityTypeAttribute.h"
#include "serviceRoutines/postContextEntityTypeAttribute.h"
#include "serviceRoutines/putAvailabilitySubscriptionConvOp.h"
#include "serviceRoutines/deleteAvailabilitySubscriptionConvOp.h"

#include "serviceRoutines/getIndividualContextEntity.h"
#include "serviceRoutines/putIndividualContextEntity.h"
#include "serviceRoutines/badVerbPostOnly.h"
#include "serviceRoutines/badVerbPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetPostOnly.h"
#include "serviceRoutines/postIndividualContextEntity.h"
#include "serviceRoutines/deleteIndividualContextEntity.h"
#include "serviceRoutines/badVerbAllFour.h"
#include "serviceRoutines/getIndividualContextEntityAttributes.h"
#include "serviceRoutines/putIndividualContextEntityAttributes.h"
#include "serviceRoutines/postIndividualContextEntityAttributes.h"
#include "serviceRoutines/deleteIndividualContextEntityAttributes.h"
#include "serviceRoutines/getIndividualContextEntityAttribute.h"
#include "serviceRoutines/getNgsi10ContextEntityTypes.h"
#include "serviceRoutines/getNgsi10ContextEntityTypesAttribute.h"
#include "serviceRoutines/postIndividualContextEntityAttribute.h"
#include "serviceRoutines/deleteIndividualContextEntityAttribute.h"
#include "serviceRoutines/putSubscriptionConvOp.h"
#include "serviceRoutines/deleteSubscriptionConvOp.h"
#include "serviceRoutines/getAttributeValueInstance.h"
#include "serviceRoutines/putAttributeValueInstance.h"
#include "serviceRoutines/deleteAttributeValueInstance.h"

#include "serviceRoutines/badVerbGetPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetPostDeleteOnly.h"
#include "serviceRoutines/badVerbGetOnly.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutines/badNgsi9Request.h"
#include "serviceRoutines/badNgsi10Request.h"
#include "serviceRoutines/badRequest.h"
#include "contextBroker/version.h"


/* ****************************************************************************
*
* Option variables
*/
bool            fg;
char            localIp[15];
int             port;
char            dbHost[64];
char            dbName[64];
char            user[64];
char            pwd[64];
char            pidPath[256];
char            fwdHost[64];
int             fwdPort;
bool            ngsi9Only;
bool            harakiri;



#define PIDPATH _i "/tmp/contextBroker.pid"
/* ****************************************************************************
*
* parse arguments
*/
PaArgument paArgs[] =
{
  { "-fg",          &fg,           "FOREGROUND",   PaBool,   PaOpt, false,          false,  true,  "don't start as daemon"           },
  { "-localIp",     localIp,       "LOCALIP",      PaString, PaOpt, _i "0.0.0.0",   PaNL,   PaNL,  "IP to receive new connections"   },
  { "-port",        &port,         "PORT",         PaInt,    PaOpt, 1026,           PaNL,   PaNL,  "port to receive new connections" },
  { "-pidpath",      pidPath,      "PID_PATH",     PaString, PaOpt, PIDPATH,        PaNL,   PaNL,  "pid file path"                   },

  { "-dbhost",      dbHost,        "DB_HOST",      PaString, PaOpt, _i "localhost", PaNL,   PaNL,  "database host"                   },
  { "-dbuser",      user,          "DB_USER",      PaString, PaOpt, _i "",          PaNL,   PaNL,  "database user"                   },
  { "-dbpwd",       pwd,           "DB_PASSWORD",  PaString, PaOpt, _i "",          PaNL,   PaNL,  "database password"               },
  { "-db",          dbName,        "DB",           PaString, PaOpt, _i "orion",     PaNL,   PaNL,  "database name"                   },

  { "-fwdHost",     fwdHost,       "FWD_HOST",     PaString, PaOpt, _i "localhost", PaNL,   PaNL,  "host for forwarding NGSI9 regs"  },
  { "-fwdPort",     &fwdPort,      "FWD_PORT",     PaInt,    PaOpt, 0,              0,      65000, "port for forwarding NGSI9 regs"  },
  { "-ngsi9",       &ngsi9Only,    "CONFMAN",      PaBool,   PaOpt, false,          false,  true,  "run as Configuration Manager"    },
  { "-harakiri",    &harakiri,     "HARAKIRI",     PaBool,   PaHid, false,          false,  true,  "commits harakiri on request"     },

  PA_END_OF_ARGS
};

/* ****************************************************************************
*
* restServiceV - vector of REST services for the context broker
*
* This vector matches an incoming REST service, using the path of the URL, to a function
* to treat the incoming request.
*
* The URL path is divided into components (Using '/' as field separator) so that the URL
* "/ngsi9/registerContext" becomes a component vector of the two components
* "ngsi9" and "registerContext".
*
* Each line contains the necessary information for ONE service:
*   std::string   verb        - GET/POST/PUT/DELETE
*   RequestType   request     - The type of the request
*   int           components  - Number of components in the following URL component vector
*   std::string   compV       - Component vector of the URL
*   std::string   payloadWord - first word in the payload for the request (to verify that the payload matches the URL). If empty, no check is performed)
*   RestTreat     treat       - Function pointer to the function to treat the incoming REST request
*
*/
RestService restServiceV[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                             2, { "ngsi9",  "registerContext"                              }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             2, { "ngsi9",  "registerContext"                              }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },
  { "POST",   NotifyContextAvailability,                   2, { "ngsi9",  "notifyContextAvailability"                    }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability             },
  { "*",      NotifyContextAvailability,                   2, { "ngsi9",  "notifyContextAvailability"                    }, "notifyContextAvailabilityRequest",             badVerbPostOnly                           },


  // NGSI-10 Requests
  { "POST",   UpdateContext,                               2, { "ngsi10", "updateContext"                                }, "updateContextRequest",                         postUpdateContext                         },
  { "*",      UpdateContext,                               2, { "ngsi10", "updateContext"                                }, "updateContextRequest",                         badVerbPostOnly                           },
  { "POST",   QueryContext,                                2, { "ngsi10", "queryContext"                                 }, "queryContextRequest",                          postQueryContext                          },
  { "*",      QueryContext,                                2, { "ngsi10", "queryContext"                                 }, "queryContextRequest",                          badVerbPostOnly                           },
  { "POST",   SubscribeContext,                            2, { "ngsi10", "subscribeContext"                             }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            2, { "ngsi10", "subscribeContext"                             }, "subscribeContextRequest",                      badVerbPostOnly                           },
  { "POST",   UpdateContextSubscription,                   2, { "ngsi10", "updateContextSubscription"                    }, "updateContextSubscriptionRequest",             postUpdateContextSubscription             },
  { "*",      UpdateContextSubscription,                   2, { "ngsi10", "updateContextSubscription"                    }, "updateContextSubscriptionRequest",             badVerbPostOnly                           },
  { "POST",   UnsubscribeContext,                          2, { "ngsi10", "unsubscribeContext"                           }, "unsubscribeContextRequest",                    postUnsubscribeContext                    },
  { "*",      UnsubscribeContext,                          2, { "ngsi10", "unsubscribeContext"                           }, "unsubscribeContextRequest",                    badVerbPostOnly                           },
  { "POST",   NotifyContext,                               2, { "ngsi10", "notifyContext"                                }, "notifyContextRequest",                         postNotifyContext                         },
  { "*",      NotifyContext,                               2, { "ngsi10", "notifyContext"                                }, "notifyContextRequest",                         badVerbPostOnly                           },


  // NGSI-9 Convenience operations
  { "GET",    ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                          } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                          } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                          } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"            } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"       } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                       } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                       } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"         } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             badVerbGetPostOnly                        },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"              }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"              }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             badVerbPutDeleteOnly                      },


  // NGSI-10 Convenience operations
  { "GET",    IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                         }, "",                                             getIndividualContextEntity                },
  { "PUT",    IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                         }, "updateContextElementRequest",                  putIndividualContextEntity                },
  { "POST",   IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                         }, "appendContextElementRequest",                  postIndividualContextEntity               },
  { "DELETE", IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                         }, "",                                             deleteIndividualContextEntity             },
  { "*",      IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                         }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"           }, "",                                             getIndividualContextEntityAttributes      },
  { "PUT",    IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"           }, "updateContextElementRequest",                  putIndividualContextEntityAttributes      },
  { "POST",   IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"           }, "appendContextElementRequest",                  postIndividualContextEntityAttributes     },
  { "DELETE", IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"           }, "",                                             deleteIndividualContextEntityAttributes   },
  { "*",      IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"           }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"      }, "",                                             getIndividualContextEntityAttribute       },
  { "POST",   IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"      }, "updateContextAttributeRequest",                postIndividualContextEntityAttribute      },
  { "DELETE", IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"      }, "",                                             deleteIndividualContextEntityAttribute    },
  { "*",      IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"      }, "",                                             badVerbGetPostDeleteOnly                  },
  
  { "GET",    AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                                             getAttributeValueInstance                 },
  { "PUT",    AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "updateContextAttributeRequest",                putAttributeValueInstance                 },
  { "DELETE", AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                                             deleteAttributeValueInstance              },
  { "*",      AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                                             badVerbGetPutDeleteOnly                   },

  { "GET",    Ngsi10ContextEntityTypes,                    3, { "ngsi10", "contextEntityTypes", "*"                      }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypes,                    3, { "ngsi10", "contextEntityTypes", "*"                      }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttributeContainer,  4, { "ngsi10", "contextEntityTypes", "*", "attributes"        }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypesAttributeContainer,  4, { "ngsi10", "contextEntityTypes", "*", "attributes"        }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttribute,           5, { "ngsi10", "contextEntityTypes", "*", "attributes", "*"   }, "",                                             getNgsi10ContextEntityTypesAttribute      },
  { "*",      Ngsi10ContextEntityTypesAttribute,           5, { "ngsi10", "contextEntityTypes", "*", "attributes", "*"   }, "",                                             badVerbGetOnly                            },
  
  { "POST",   SubscribeContext,                            2, { "ngsi10", "contextSubscriptions"                         }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            2, { "ngsi10", "contextSubscriptions"                         }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                    }, "updateContextSubscriptionRequest",             putSubscriptionConvOp                     },
  { "DELETE", Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                    }, "",                                             deleteSubscriptionConvOp                  },
  { "*",      Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                    }, "",                                             badVerbPutDeleteOnly                      },

  // log request
  { "GET",    LogRequest,                                  2, { "log", "verbose"                                         }, "",                                             logVerboseTreat                           },
  { "PUT",    LogRequest,                                  3, { "log", "verbose", "*"                                    }, "",                                             logVerboseTreat                           },
  { "POST",   LogRequest,                                  3, { "log", "verbose", "*"                                    }, "",                                             logVerboseTreat                           },
  { "DELETE", LogRequest,                                  2, { "log", "verbose"                                         }, "",                                             logVerboseTreat                           },

  { "GET",    LogRequest,                                  2, { "log", "traceLevel"                                      }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "traceLevel", "*"                                 }, "",                                             logTraceTreat                             },
  { "POST",   LogRequest,                                  3, { "log", "traceLevel", "*"                                 }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "traceLevel"                                      }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "traceLevel", "*"                                 }, "",                                             logTraceTreat                             },


  // version request
  { "GET",    VersionRequest,                              1, { "version"                                                }, "",                                             versionTreat                              },
  { "*",      VersionRequest,                              1, { "version"                                                }, "",                                             badVerbGetOnly                            },

  // statistics request
  { "GET",    StatisticsRequest,                           1, { "statistics"                                             }, "",                                             statisticsTreat                           },
  { "DELETE", StatisticsRequest,                           1, { "statistics"                                             }, "",                                             statisticsTreat                           },
  { "*",      StatisticsRequest,                           1, { "statistics"                                             }, "",                                             badVerbGetDeleteOnly                      },

#ifdef DEBUG
  { "GET",    ExitRequest,                                 2, { "exit", "*"                                              }, "",                                             exitTreat                                 },
  { "GET",    ExitRequest,                                 1, { "exit"                                                   }, "",                                             exitTreat                                 },
  { "GET",    LeakRequest,                                 2, { "leak", "*"                                              }, "",                                             leakTreat                                 },
  { "GET",    LeakRequest,                                 1, { "leak"                                                   }, "",                                             leakTreat                                 },
#endif

  // Bad requests
  { "*",      InvalidRequest,                              2, { "ngsi9",  "*"                                            }, "",                                             badNgsi9Request                           },
  { "*",      InvalidRequest,                              2, { "ngsi10", "*"                                            }, "",                                             badNgsi10Request                          },
  { "*",      InvalidRequest,                              0, { "*", "*", "*", "*", "*", "*"                             }, "",                                             badRequest                                },

  // End marker for the array
  { "",       InvalidRequest,                              0, {                                                          }, "",                                             NULL                                      }
};


RestService restServiceNgsi9V[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                             2, { "ngsi9",  "registerContext"                              }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             2, { "ngsi9",  "registerContext"                              }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },


  // NGSI-9 Convenience operations
  { "GET",    ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                          } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                          } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                          } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"            } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"       } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                       } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                       } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"         } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             badVerbGetPostOnly                        },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"              }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"              }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             badVerbPutDeleteOnly                      },


  // log request
  { "GET",    LogRequest,                                  2, { "log", "verbose"                                         }, "",                                             logVerboseTreat                           },
  { "PUT",    LogRequest,                                  3, { "log", "verbose", "*"                                    }, "",                                             logVerboseTreat                           },
  { "POST",   LogRequest,                                  3, { "log", "verbose", "*"                                    }, "",                                             logVerboseTreat                           },
  { "DELETE", LogRequest,                                  2, { "log", "verbose"                                         }, "",                                             logVerboseTreat                           },

  { "GET",    LogRequest,                                  2, { "log", "traceLevel"                                      }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "traceLevel", "*"                                 }, "",                                             logTraceTreat                             },
  { "POST",   LogRequest,                                  3, { "log", "traceLevel", "*"                                 }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "traceLevel"                                      }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "traceLevel", "*"                                 }, "",                                             logTraceTreat                             },


  // version request
  { "GET",    VersionRequest,                              1, { "version"                                                }, "",                                             versionTreat                              },
  { "*",      VersionRequest,                              1, { "version"                                                }, "",                                             badVerbGetOnly                            },

  // statistics request
  { "GET",    StatisticsRequest,                           1, { "statistics"                                             }, "",                                             statisticsTreat                           },
  { "DELETE", StatisticsRequest,                           1, { "statistics"                                             }, "",                                             statisticsTreat                           },
  { "*",      StatisticsRequest,                           1, { "statistics"                                             }, "",                                             badVerbGetDeleteOnly                      },

#ifdef DEBUG
  { "GET",    ExitRequest,                                 2, { "exit", "*"                                              }, "",                                             exitTreat                                 },
  { "GET",    ExitRequest,                                 1, { "exit"                                                   }, "",                                             exitTreat                                 },
  { "GET",    LeakRequest,                                 2, { "leak", "*"                                              }, "",                                             leakTreat                                 },
  { "GET",    LeakRequest,                                 1, { "leak"                                                   }, "",                                             leakTreat                                 },
#endif

  // Bad requests
  { "*",      InvalidRequest,                              2, { "ngsi9",  "*"                                            }, "",                                             badNgsi9Request                           },
  { "*",      InvalidRequest,                              0, { "*", "*", "*", "*", "*", "*"                             }, "",                                             badRequest                                },

  // End marker for the array
  { "",       InvalidRequest,                              0, {                                                          }, "",                                             NULL                                      }
};

/* ****************************************************************************
*
* pidFile -
*/
int pidFile(void)
{
  int    fd = open(pidPath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  pid_t  pid;
  char   buffer[32];
  int    sz;
  int    nb;

  if (fd == -1)
    LM_RE(-1, ("Error opening pid file '%s': %s", pidPath, strerror(errno)));

  pid = getpid();

  sprintf(buffer, "%d", pid);
  sz = strlen(buffer);
  nb = write(fd, buffer, sz);
  if (nb != sz)
     LM_RE(-2, ("written %d bytes and not %d to pid file '%s': %s", nb, sz, pidPath, strerror(errno)));

  return 0;
}



/* ****************************************************************************
*
* daemonize -
*/
void daemonize(void)
{
    pid_t  pid;
    pid_t  sid;

    // already daemon
    if (getppid() == 1)
        return;

    pid = fork();
    if (pid == -1)
        LM_X(1, ("fork: %s", strerror(errno)));

    // Exiting father process
    if (pid > 0)
        exit(0);

    // Change the file mode mask */
    umask(0);

    // Removing the controlling terminal
    sid = setsid();
    if (sid == -1)
        LM_X(1, ("setsid: %s", strerror(errno)));

    // Change current working directory.
    // This prevents the current directory from being locked; hence not being able to remove it.
    if (chdir("/") == -1)
        LM_X(1, ("chdir: %s", strerror(errno)));

    // We have to call this after a fork, see: http://api.mongodb.org/cplusplus/2.2.2/classmongo_1_1_o_i_d.html
    OID::justForked();
}

/* ****************************************************************************
*
* sigHandler - 
*/
void sigHandler(int sigNo)
{
  int fd;

  LM_M(("In sigHandler - caught signal %d", sigNo));

  switch (sigNo)
  {
  case SIGINT:
  case SIGTERM:
    LM_X(1, ("Received signal %d", sigNo));
    break;

  case SIGUSR1:
    fd = lmFirstDiskFileDescriptor();
    LM_M(("Caught SIGUSR1 - inhibiting logs (on fd %d) until SIGUSR2 arrives", fd));
    lmFdUnregister(fd);
    close(fd);
    LM_M(("Caught SIGUSR1 - this message should not be seen in log file, only on stdout"));
    break;

  case SIGUSR2:
     // FIXME P7: "/tmp" ... Not always correct
    lmPathRegister("/tmp", "DEF", "DEF", NULL);
    LM_M(("Caught SIGUSR2 - log goes on"));
    break;
  }
}

/* ****************************************************************************
*
* exitFunc - 
*/
void exitFunc(void)
{
  if (unlink(pidPath) != 0)
    LM_E(("unlink(%s): %s", pidPath, strerror(errno)));
}

const char* description =
   "\n"
   "Orion context broker version details:\n"
   "  version:            " ORION_VERSION   "\n"
   "  git hash:           " GIT_HASH        "\n"
   "  compile time:       " COMPILE_TIME    "\n"
   "  compiled by:        " COMPILED_BY     "\n"
   "  compiled in:        " COMPILED_IN     "\n";

/* ****************************************************************************
*
* main - 
*/
int main(int argC, char* argV[])
{
  signal(SIGINT,  sigHandler);
  signal(SIGTERM, sigHandler);
  signal(SIGUSR1, sigHandler);
  signal(SIGUSR2, sigHandler);

  atexit(exitFunc);

  paConfig("man synopsis",         (void*) "[options]");
  paConfig("man shortdescription", (void*) "Options:");
  paConfig("man description",      (void*) description);
  paConfig("man author",           (void*) "Telefonica I+D");
  paConfig("man exitstatus",       (void*) "The orion broker is a daemon. If it exits, something is wrong ...");
  paConfig("man version",          (void*) ORION_VERSION);

  paConfig("builtin prefix",                    (void*) "ORION_");
  paConfig("usage and exit on any warning",     (void*) true);
  paConfig("log to screen",                     (void*) true);
  paConfig("log to file",                       (void*) true);
  paConfig("remove builtin", "-d");
  paConfig("remove builtin", "-r");
  paConfig("remove builtin", "-w");
  paConfig("log file line format",              (void*) "TYPE:DATE:EXEC-AUX/FILE[LINE] FUNC: TEXT");
  paConfig("screen line format",                (void*) "TYPE@TIME  FUNC[LINE]: TEXT");

  paParse(paArgs, argC, (char**) argV, 1, false);

  if (fg == false)
    daemonize();

  pidFile();

  LM_M(("Opening mongo connection"));
  std::string entitiesCollection                      = std::string(dbName) + ".entities";
  std::string registrationCollection                  = std::string(dbName) + ".registrations";
  std::string subscribeContextCollection              = std::string(dbName) + ".csubs";
  std::string subscribeContextAvailabilityCollection  = std::string(dbName) + ".casubs";
  std::string associationsCollection                  = std::string(dbName) + ".associations";

  if (!mongoConnect(dbHost, dbName, user, pwd))
    LM_X(1, ("MongoDB error"));
  LM_M(("Connected to %s:%s as user %s", dbHost, dbName, user));

  setEntitiesCollectionName(entitiesCollection.c_str());
  setRegistrationsCollectionName(registrationCollection.c_str());
  setSubscribeContextCollectionName(subscribeContextCollection.c_str());
  setSubscribeContextAvailabilityCollectionName(subscribeContextAvailabilityCollection.c_str());
  setAssociationsCollectionName(associationsCollection.c_str());

  /* Set timer object (singleton) */
  setTimer(new Timer());

  /* Set notifier object (singleton) */
  setNotifier(new Notifier());

  if (ngsi9Only)
      LM_M(("Running in NGSI9 only mode"));

  /* Launch threads corresponding to ONTIMEINTERVAL subscriptions in the database (only if not ngsi9 mode) */
  if (!ngsi9Only)
     recoverOntimeIntervalThreads();

  LM_M(("Listening on port %d", port));
  if (ngsi9Only)
    restInit(localIp, port, restServiceNgsi9V);
  else
    restInit(localIp, port, restServiceV);

  /* Set start time */
  startTime      = getCurrentTime();
  statisticsTime = startTime;

  /* Initialize the semaphore used by mongoBackend */
  semInit();

  int r;
  if ((r = restStart()) != 0)
     LM_X(1, ("restStart: error %d", r));

  // Give the rest library the correct version string of this executable
  versionSet(ORION_VERSION);

  while (1)
     sleep(10);
}
