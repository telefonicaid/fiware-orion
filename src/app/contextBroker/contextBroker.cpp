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

/* ****************************************************************************
*
* Some notes on HTTPS
*
* Lots of info found in http://www.gnu.org/software/libmicrohttpd/tutorial.html
*
* When https is used, the broker must be started with options '-key' and '-cert'.
* Both these two options have a file path associated to it:
*   -key:  path to a file containing the private key for the server
*   -cert: path to a file containing a certificate describing the server in human readable tokens
*
* These files are generated before starting the broker:
* 
* o private key:
*     % openssl genrsa -out server.key 1024
*
* o certificate:
*     % openssl req -days 365 -out server.pem -new -x509 -key server.key
*
* After creating these two files, the context broker can be started like this:
*   % contextBroker -fg -https -key server.key -cert server.pem
*
* The clients need to use the 'server.pem' file in the request:
* curl --cacert server.pem
*
*
* To override the security added with the certificate, curl can always be called using the
* CLI option '--insecure'.
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
#include <curl/curl.h>

#include "mongoBackend/MongoGlobal.h"

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "parseArgs/paBuiltin.h"
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
#include "serviceRoutines/putIndividualContextEntityAttribute.h"
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

#include "common/string.h"


/* ****************************************************************************
*
* Option variables
*/
bool            fg;
char            bindAddress[MAX_LEN_IP];
int             port;
char            dbHost[64];
char            rplSet[64];
char            dbName[64];
char            user[64];
char            pwd[64];
char            pidPath[256];
char            fwdHost[64];
int             fwdPort;
bool            ngsi9Only;
bool            harakiri;
bool            useOnlyIPv4;
bool            useOnlyIPv6;
char            httpsKeyFile[1024];
char            httpsCertFile[1024];
bool            https;
char            mtenant[32];
char            rush[256];


#define PIDPATH _i "/tmp/contextBroker.pid"
/* ****************************************************************************
*
* parse arguments
*/
PaArgument paArgs[] =
{
  { "-fg",           &fg,           "FOREGROUND",      PaBool,   PaOpt, false,          false,  true,  "don't start as daemon"                     },
  { "-localIp",      bindAddress,   "LOCALIP",         PaString, PaOpt, _i "0.0.0.0",   PaNL,   PaNL,  "IP to receive new connections"             },
  { "-port",         &port,         "PORT",            PaInt,    PaOpt, 1026,           PaNL,   PaNL,  "port to receive new connections"           },
  { "-pidpath",      pidPath,       "PID_PATH",        PaString, PaOpt, PIDPATH,        PaNL,   PaNL,  "pid file path"                             },

  { "-dbhost",       dbHost,        "DB_HOST",         PaString, PaOpt, _i "localhost", PaNL,   PaNL,  "database host"                             },
  { "-rplSet",       rplSet,        "RPL_SET",         PaString, PaOpt, _i "",          PaNL,   PaNL,  "replicat set"                              },
  { "-dbuser",       user,          "DB_USER",         PaString, PaOpt, _i "",          PaNL,   PaNL,  "database user"                             },
  { "-dbpwd",        pwd,           "DB_PASSWORD",     PaString, PaOpt, _i "",          PaNL,   PaNL,  "database password"                         },
  { "-db",           dbName,        "DB",              PaString, PaOpt, _i "orion",     PaNL,   PaNL,  "database name"                             },

  { "-fwdHost",      fwdHost,       "FWD_HOST",        PaString, PaOpt, _i "localhost", PaNL,   PaNL,  "host for forwarding NGSI9 regs"            },
  { "-fwdPort",      &fwdPort,      "FWD_PORT",        PaInt,    PaOpt, 0,              0,      65000, "port for forwarding NGSI9 regs"            },
  { "-ngsi9",        &ngsi9Only,    "CONFMAN",         PaBool,   PaOpt, false,          false,  true,  "run as Configuration Manager"              },
  { "-ipv4",         &useOnlyIPv4,  "USEIPV4",         PaBool,   PaOpt, false,          false,  true,  "use ip v4 only"                            },
  { "-ipv6",         &useOnlyIPv6,  "USEIPV6",         PaBool,   PaOpt, false,          false,  true,  "use ip v6 only"                            },
  { "-harakiri",     &harakiri,     "HARAKIRI",        PaBool,   PaHid, false,          false,  true,  "commits harakiri on request"               },

  { "-https",        &https,        "HTTPS",           PaBool,   PaOpt, false,          false,  true,  "use the https 'protocol'"                  },
  { "-key",          httpsKeyFile,  "HTTPS_KEY_FILE",  PaString, PaOpt, _i "",          PaNL,   PaNL,  "private server key file (for https)"       },
  { "-cert",         httpsCertFile, "HTTPS_CERT_FILE", PaString, PaOpt, _i "",          PaNL,   PaNL,  "certificate key file (for https)"          },

  { "-rush",         rush,          "RUSH",            PaString, PaOpt, _i "",          PaNL,   PaNL,  "rush host (IP:port)"                         },
  { "-multiservice", mtenant,       "MULTI_SERVICE",   PaString, PaOpt, _i "off",       PaNL,   PaNL,  "service multi tenancy mode (off|url|header)" },


  PA_END_OF_ARGS
};

/* ****************************************************************************
*
* restService* - vectors of REST services for the context broker
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


/* ****************************************************************************
*
* restServiceMTenant - services for BROKER (ngsi9/10) and tenants 
*
* This service vector (configuration) is used if the broker is started with
* the the -multiservice option (but not the -ngsi9 option)
*/
RestService restServiceMTenant[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },
  { "POST",   NotifyContextAvailability,                   2, { "ngsi9",  "notifyContextAvailability"                         }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability             },
  { "*",      NotifyContextAvailability,                   2, { "ngsi9",  "notifyContextAvailability"                         }, "notifyContextAvailabilityRequest",             badVerbPostOnly                           },

  // NGSI-9 Requests with tenant
  { "POST",   RegisterContext,                             3, { "*", "ngsi9",  "registerContext"                              }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             3, { "*", "ngsi9",  "registerContext"                              }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 3, { "*", "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 3, { "*", "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                3, { "*", "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                3, { "*", "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              3, { "*", "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              3, { "*", "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       3, { "*", "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       3, { "*", "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },
  { "POST",   NotifyContextAvailability,                   3, { "*", "ngsi9",  "notifyContextAvailability"                    }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability             },
  { "*",      NotifyContextAvailability,                   3, { "*", "ngsi9",  "notifyContextAvailability"                    }, "notifyContextAvailabilityRequest",             badVerbPostOnly                           },


  // NGSI-10 Requests
  { "POST",   UpdateContext,                               2, { "ngsi10", "updateContext"                                     }, "updateContextRequest",                         postUpdateContext                         },
  { "*",      UpdateContext,                               2, { "ngsi10", "updateContext"                                     }, "updateContextRequest",                         badVerbPostOnly                           },
  { "POST",   QueryContext,                                2, { "ngsi10", "queryContext"                                      }, "queryContextRequest",                          postQueryContext                          },
  { "*",      QueryContext,                                2, { "ngsi10", "queryContext"                                      }, "queryContextRequest",                          badVerbPostOnly                           },
  { "POST",   SubscribeContext,                            2, { "ngsi10", "subscribeContext"                                  }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            2, { "ngsi10", "subscribeContext"                                  }, "subscribeContextRequest",                      badVerbPostOnly                           },
  { "POST",   UpdateContextSubscription,                   2, { "ngsi10", "updateContextSubscription"                         }, "updateContextSubscriptionRequest",             postUpdateContextSubscription             },
  { "*",      UpdateContextSubscription,                   2, { "ngsi10", "updateContextSubscription"                         }, "updateContextSubscriptionRequest",             badVerbPostOnly                           },
  { "POST",   UnsubscribeContext,                          2, { "ngsi10", "unsubscribeContext"                                }, "unsubscribeContextRequest",                    postUnsubscribeContext                    },
  { "*",      UnsubscribeContext,                          2, { "ngsi10", "unsubscribeContext"                                }, "unsubscribeContextRequest",                    badVerbPostOnly                           },
  { "POST",   NotifyContext,                               2, { "ngsi10", "notifyContext"                                     }, "notifyContextRequest",                         postNotifyContext                         },
  { "*",      NotifyContext,                               2, { "ngsi10", "notifyContext"                                     }, "notifyContextRequest",                         badVerbPostOnly                           },

  // NGSI-10 Requests with tenant
  { "POST",   UpdateContext,                               3, { "*", "ngsi10", "updateContext"                                }, "updateContextRequest",                         postUpdateContext                         },
  { "*",      UpdateContext,                               3, { "*", "ngsi10", "updateContext"                                }, "updateContextRequest",                         badVerbPostOnly                           },
  { "POST",   QueryContext,                                3, { "*", "ngsi10", "queryContext"                                 }, "queryContextRequest",                          postQueryContext                          },
  { "*",      QueryContext,                                3, { "*", "ngsi10", "queryContext"                                 }, "queryContextRequest",                          badVerbPostOnly                           },
  { "POST",   SubscribeContext,                            3, { "*", "ngsi10", "subscribeContext"                             }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            3, { "*", "ngsi10", "subscribeContext"                             }, "subscribeContextRequest",                      badVerbPostOnly                           },
  { "POST",   UpdateContextSubscription,                   3, { "*", "ngsi10", "updateContextSubscription"                    }, "updateContextSubscriptionRequest",             postUpdateContextSubscription             },
  { "*",      UpdateContextSubscription,                   3, { "*", "ngsi10", "updateContextSubscription"                    }, "updateContextSubscriptionRequest",             badVerbPostOnly                           },
  { "POST",   UnsubscribeContext,                          3, { "*", "ngsi10", "unsubscribeContext"                           }, "unsubscribeContextRequest",                    postUnsubscribeContext                    },
  { "*",      UnsubscribeContext,                          3, { "*", "ngsi10", "unsubscribeContext"                           }, "unsubscribeContextRequest",                    badVerbPostOnly                           },
  { "POST",   NotifyContext,                               3, { "*", "ngsi10", "notifyContext"                                }, "notifyContextRequest",                         postNotifyContext                         },
  { "*",      NotifyContext,                               3, { "*", "ngsi10", "notifyContext"                                }, "notifyContextRequest",                         badVerbPostOnly                           },

  // NGSI-9 Convenience operations
  { "GET",    ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             badVerbGetPostOnly                        },

  { "POST",   SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             badVerbPutDeleteOnly                      },

  // NGSI-9 Convenience operations with tenant
  { "GET",    ContextEntitiesByEntityId,                   4, { "*", "ngsi9", "contextEntities", "*"                          } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   4, { "*", "ngsi9", "contextEntities", "*"                          } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   4, { "*", "ngsi9", "contextEntities", "*"                          } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     5, { "*", "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     5, { "*", "ngsi9", "contextEntities", "*", "attributes"            } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     5, { "*", "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   6, { "*", "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   6, { "*", "ngsi9", "contextEntities", "*", "attributes", "*"       } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   6, { "*", "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          4, { "*", "ngsi9", "contextEntityTypes", "*"                       } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          4, { "*", "ngsi9", "contextEntityTypes", "*"                       } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          4, { "*", "ngsi9", "contextEntityTypes", "*"                       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         5, { "*", "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         5, { "*", "ngsi9", "contextEntityTypes", "*", "attributes"         } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         5, { "*", "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  6, { "*", "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  6, { "*", "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  6, { "*", "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             badVerbGetPostOnly                        },

  { "POST",   SubscribeContextAvailability,                3, { "*", "ngsi9", "contextAvailabilitySubscriptions"              }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                3, { "*", "ngsi9", "contextAvailabilitySubscriptions"              }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    4, { "*", "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    4, { "*", "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    4, { "*", "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             badVerbPutDeleteOnly                      },


  // NGSI-10 Convenience operations
  { "GET",    IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "",                                             getIndividualContextEntity                },
  { "PUT",    IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "updateContextElementRequest",                  putIndividualContextEntity                },
  { "POST",   IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "appendContextElementRequest",                  postIndividualContextEntity               },
  { "DELETE", IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "",                                             deleteIndividualContextEntity             },
  { "*",      IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "",                                             getIndividualContextEntityAttributes      },
  { "PUT",    IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "updateContextElementRequest",                  putIndividualContextEntityAttributes      },
  { "POST",   IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "appendContextElementRequest",                  postIndividualContextEntityAttributes     },
  { "DELETE", IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "",                                             deleteIndividualContextEntityAttributes   },
  { "*",      IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "",                                             getIndividualContextEntityAttribute       },
  { "PUT",    IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "updateContextAttributeRequest",                putIndividualContextEntityAttribute       },
  { "POST",   IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "updateContextAttributeRequest",                postIndividualContextEntityAttribute      },
  { "DELETE", IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "",                                             deleteIndividualContextEntityAttribute    },
  { "*",      IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "",                                             badVerbGetPostDeleteOnly                  },
  
  { "GET",    AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "",                                             getAttributeValueInstance                 },
  { "PUT",    AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "updateContextAttributeRequest",                putAttributeValueInstance                 },
  { "DELETE", AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "",                                             deleteAttributeValueInstance              },
  { "*",      AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "",                                             badVerbGetPutDeleteOnly                   },

  { "GET",    Ngsi10ContextEntityTypes,                    3, { "ngsi10", "contextEntityTypes", "*"                           }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypes,                    3, { "ngsi10", "contextEntityTypes", "*"                           }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttributeContainer,  4, { "ngsi10", "contextEntityTypes", "*", "attributes"             }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypesAttributeContainer,  4, { "ngsi10", "contextEntityTypes", "*", "attributes"             }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttribute,           5, { "ngsi10", "contextEntityTypes", "*", "attributes", "*"        }, "",                                             getNgsi10ContextEntityTypesAttribute      },
  { "*",      Ngsi10ContextEntityTypesAttribute,           5, { "ngsi10", "contextEntityTypes", "*", "attributes", "*"        }, "",                                             badVerbGetOnly                            },
  
  { "POST",   SubscribeContext,                            2, { "ngsi10", "contextSubscriptions"                              }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            2, { "ngsi10", "contextSubscriptions"                              }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                         }, "updateContextSubscriptionRequest",             putSubscriptionConvOp                     },
  { "DELETE", Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                         }, "",                                             deleteSubscriptionConvOp                  },
  { "*",      Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                         }, "",                                             badVerbPutDeleteOnly                      },

  // NGSI-10 Convenience operations with tenant
  { "GET",    IndividualContextEntity,                     4, { "*", "ngsi10", "contextEntities", "*"                         }, "",                                             getIndividualContextEntity                },
  { "PUT",    IndividualContextEntity,                     4, { "*", "ngsi10", "contextEntities", "*"                         }, "updateContextElementRequest",                  putIndividualContextEntity                },
  { "POST",   IndividualContextEntity,                     4, { "*", "ngsi10", "contextEntities", "*"                         }, "appendContextElementRequest",                  postIndividualContextEntity               },
  { "DELETE", IndividualContextEntity,                     4, { "*", "ngsi10", "contextEntities", "*"                         }, "",                                             deleteIndividualContextEntity             },
  { "*",      IndividualContextEntity,                     4, { "*", "ngsi10", "contextEntities", "*"                         }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttributes,           5, { "*", "ngsi10", "contextEntities", "*", "attributes"           }, "",                                             getIndividualContextEntityAttributes      },
  { "PUT",    IndividualContextEntityAttributes,           5, { "*", "ngsi10", "contextEntities", "*", "attributes"           }, "updateContextElementRequest",                  putIndividualContextEntityAttributes      },
  { "POST",   IndividualContextEntityAttributes,           5, { "*", "ngsi10", "contextEntities", "*", "attributes"           }, "appendContextElementRequest",                  postIndividualContextEntityAttributes     },
  { "DELETE", IndividualContextEntityAttributes,           5, { "*", "ngsi10", "contextEntities", "*", "attributes"           }, "",                                             deleteIndividualContextEntityAttributes   },
  { "*",      IndividualContextEntityAttributes,           5, { "*", "ngsi10", "contextEntities", "*", "attributes"           }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttribute,            6, { "*", "ngsi10", "contextEntities", "*", "attributes", "*"      }, "",                                             getIndividualContextEntityAttribute       },
  { "PUT",    IndividualContextEntityAttribute,            6, { "*", "ngsi10", "contextEntities", "*", "attributes", "*"      }, "updateContextAttributeRequest",                putIndividualContextEntityAttribute       },
  { "POST",   IndividualContextEntityAttribute,            6, { "*", "ngsi10", "contextEntities", "*", "attributes", "*"      }, "updateContextAttributeRequest",                postIndividualContextEntityAttribute      },
  { "DELETE", IndividualContextEntityAttribute,            6, { "*", "ngsi10", "contextEntities", "*", "attributes", "*"      }, "",                                             deleteIndividualContextEntityAttribute    },
  { "*",      IndividualContextEntityAttribute,            6, { "*", "ngsi10", "contextEntities", "*", "attributes", "*"      }, "",                                             badVerbGetPostDeleteOnly                  },
  
  { "GET",    AttributeValueInstance,                      7, { "*", "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                                             getAttributeValueInstance                 },
  { "PUT",    AttributeValueInstance,                      7, { "*", "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "updateContextAttributeRequest",                putAttributeValueInstance                 },
  { "DELETE", AttributeValueInstance,                      7, { "*", "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                                             deleteAttributeValueInstance              },
  { "*",      AttributeValueInstance,                      7, { "*", "ngsi10", "contextEntities", "*", "attributes", "*", "*" }, "",                                             badVerbGetPutDeleteOnly                   },

  { "GET",    Ngsi10ContextEntityTypes,                    4, { "*", "ngsi10", "contextEntityTypes", "*"                      }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypes,                    4, { "*", "ngsi10", "contextEntityTypes", "*"                      }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttributeContainer,  5, { "*", "ngsi10", "contextEntityTypes", "*", "attributes"        }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypesAttributeContainer,  5, { "*", "ngsi10", "contextEntityTypes", "*", "attributes"        }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttribute,           6, { "*", "ngsi10", "contextEntityTypes", "*", "attributes", "*"   }, "",                                             getNgsi10ContextEntityTypesAttribute      },
  { "*",      Ngsi10ContextEntityTypesAttribute,           6, { "*", "ngsi10", "contextEntityTypes", "*", "attributes", "*"   }, "",                                             badVerbGetOnly                            },
  
  { "POST",   SubscribeContext,                            3, { "*", "ngsi10", "contextSubscriptions"                         }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            3, { "*", "ngsi10", "contextSubscriptions"                         }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi10SubscriptionsConvOp,                   4, { "*", "ngsi10", "contextSubscriptions", "*"                    }, "updateContextSubscriptionRequest",             putSubscriptionConvOp                     },
  { "DELETE", Ngsi10SubscriptionsConvOp,                   4, { "*", "ngsi10", "contextSubscriptions", "*"                    }, "",                                             deleteSubscriptionConvOp                  },
  { "*",      Ngsi10SubscriptionsConvOp,                   4, { "*", "ngsi10", "contextSubscriptions", "*"                    }, "",                                             badVerbPutDeleteOnly                      },

  // log requests

  // The documentation (Installation and Admin Guide) says /log/trace ...
  { "GET",    LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "trace"                                                }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             badVerbAllFour                            },

  // ... and to maintain backward compatibility we keep supporting /log/traceLevel too
  { "GET",    LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             badVerbAllFour                            },

  // version request
  { "GET",    VersionRequest,                              1, { "version"                                                     }, "",                                             versionTreat                              },
  { "*",      VersionRequest,                              1, { "version"                                                     }, "",                                             badVerbGetOnly                            },

  // statistics request
  { "GET",    StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "DELETE", StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "*",      StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             badVerbGetDeleteOnly                      },

#ifdef DEBUG
  { "GET",    ExitRequest,                                 2, { "exit", "*"                                                   }, "",                                             exitTreat                                 },
  { "GET",    ExitRequest,                                 1, { "exit"                                                        }, "",                                             exitTreat                                 },
  { "GET",    LeakRequest,                                 2, { "leak", "*"                                                   }, "",                                             leakTreat                                 },
  { "GET",    LeakRequest,                                 1, { "leak"                                                        }, "",                                             leakTreat                                 },
#endif

  // Bad requests
  { "*",      InvalidRequest,                              2, { "ngsi9",  "*"                                                 }, "",                                             badNgsi9Request                           },
  { "*",      InvalidRequest,                              2, { "ngsi10", "*"                                                 }, "",                                             badNgsi10Request                          },
  { "*",      InvalidRequest,                              0, { "*", "*", "*", "*", "*", "*"                                  }, "",                                             badRequest                                },

  // End marker for the array
  { "",       InvalidRequest,                              0, {                                                               }, "",                                             NULL                                      }
};



/* ****************************************************************************
*
* restServiceV - services for BROKER (ngsi9/10) without tenants
*
* This is the default service vector, that is used if the broker is started without
* the -ngsi9 and -multiservice options
*/
RestService restServiceV[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },
  { "POST",   NotifyContextAvailability,                   2, { "ngsi9",  "notifyContextAvailability"                         }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability             },
  { "*",      NotifyContextAvailability,                   2, { "ngsi9",  "notifyContextAvailability"                         }, "notifyContextAvailabilityRequest",             badVerbPostOnly                           },

  // NGSI-10 Requests
  { "POST",   UpdateContext,                               2, { "ngsi10", "updateContext"                                     }, "updateContextRequest",                         postUpdateContext                         },
  { "*",      UpdateContext,                               2, { "ngsi10", "updateContext"                                     }, "updateContextRequest",                         badVerbPostOnly                           },
  { "POST",   QueryContext,                                2, { "ngsi10", "queryContext"                                      }, "queryContextRequest",                          postQueryContext                          },
  { "*",      QueryContext,                                2, { "ngsi10", "queryContext"                                      }, "queryContextRequest",                          badVerbPostOnly                           },
  { "POST",   SubscribeContext,                            2, { "ngsi10", "subscribeContext"                                  }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            2, { "ngsi10", "subscribeContext"                                  }, "subscribeContextRequest",                      badVerbPostOnly                           },
  { "POST",   UpdateContextSubscription,                   2, { "ngsi10", "updateContextSubscription"                         }, "updateContextSubscriptionRequest",             postUpdateContextSubscription             },
  { "*",      UpdateContextSubscription,                   2, { "ngsi10", "updateContextSubscription"                         }, "updateContextSubscriptionRequest",             badVerbPostOnly                           },
  { "POST",   UnsubscribeContext,                          2, { "ngsi10", "unsubscribeContext"                                }, "unsubscribeContextRequest",                    postUnsubscribeContext                    },
  { "*",      UnsubscribeContext,                          2, { "ngsi10", "unsubscribeContext"                                }, "unsubscribeContextRequest",                    badVerbPostOnly                           },
  { "POST",   NotifyContext,                               2, { "ngsi10", "notifyContext"                                     }, "notifyContextRequest",                         postNotifyContext                         },
  { "*",      NotifyContext,                               2, { "ngsi10", "notifyContext"                                     }, "notifyContextRequest",                         badVerbPostOnly                           },

  // NGSI-9 Convenience operations
  { "GET",    ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             badVerbGetPostOnly                        },

  { "POST",   SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             badVerbPutDeleteOnly                      },

  // NGSI-10 Convenience operations
  { "GET",    IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "",                                             getIndividualContextEntity                },
  { "PUT",    IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "updateContextElementRequest",                  putIndividualContextEntity                },
  { "POST",   IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "appendContextElementRequest",                  postIndividualContextEntity               },
  { "DELETE", IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "",                                             deleteIndividualContextEntity             },
  { "*",      IndividualContextEntity,                     3, { "ngsi10", "contextEntities", "*"                              }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "",                                             getIndividualContextEntityAttributes      },
  { "PUT",    IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "updateContextElementRequest",                  putIndividualContextEntityAttributes      },
  { "POST",   IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "appendContextElementRequest",                  postIndividualContextEntityAttributes     },
  { "DELETE", IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "",                                             deleteIndividualContextEntityAttributes   },
  { "*",      IndividualContextEntityAttributes,           4, { "ngsi10", "contextEntities", "*", "attributes"                }, "",                                             badVerbAllFour                            },

  { "GET",    IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "",                                             getIndividualContextEntityAttribute       },
  { "PUT",    IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "updateContextAttributeRequest",                putIndividualContextEntityAttribute       },
  { "POST",   IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "updateContextAttributeRequest",                postIndividualContextEntityAttribute      },
  { "DELETE", IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "",                                             deleteIndividualContextEntityAttribute    },
  { "*",      IndividualContextEntityAttribute,            5, { "ngsi10", "contextEntities", "*", "attributes", "*"           }, "",                                             badVerbGetPostDeleteOnly                  },
  
  { "GET",    AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "",                                             getAttributeValueInstance                 },
  { "PUT",    AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "updateContextAttributeRequest",                putAttributeValueInstance                 },
  { "DELETE", AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "",                                             deleteAttributeValueInstance              },
  { "*",      AttributeValueInstance,                      6, { "ngsi10", "contextEntities", "*", "attributes", "*", "*"      }, "",                                             badVerbGetPutDeleteOnly                   },

  { "GET",    Ngsi10ContextEntityTypes,                    3, { "ngsi10", "contextEntityTypes", "*"                           }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypes,                    3, { "ngsi10", "contextEntityTypes", "*"                           }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttributeContainer,  4, { "ngsi10", "contextEntityTypes", "*", "attributes"             }, "",                                             getNgsi10ContextEntityTypes               },
  { "*",      Ngsi10ContextEntityTypesAttributeContainer,  4, { "ngsi10", "contextEntityTypes", "*", "attributes"             }, "",                                             badVerbGetOnly                            },

  { "GET",    Ngsi10ContextEntityTypesAttribute,           5, { "ngsi10", "contextEntityTypes", "*", "attributes", "*"        }, "",                                             getNgsi10ContextEntityTypesAttribute      },
  { "*",      Ngsi10ContextEntityTypesAttribute,           5, { "ngsi10", "contextEntityTypes", "*", "attributes", "*"        }, "",                                             badVerbGetOnly                            },
  
  { "POST",   SubscribeContext,                            2, { "ngsi10", "contextSubscriptions"                              }, "subscribeContextRequest",                      postSubscribeContext                      },
  { "*",      SubscribeContext,                            2, { "ngsi10", "contextSubscriptions"                              }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                         }, "updateContextSubscriptionRequest",             putSubscriptionConvOp                     },
  { "DELETE", Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                         }, "",                                             deleteSubscriptionConvOp                  },
  { "*",      Ngsi10SubscriptionsConvOp,                   3, { "ngsi10", "contextSubscriptions", "*"                         }, "",                                             badVerbPutDeleteOnly                      },

  // log request
  { "GET",    LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "trace"                                                }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             badVerbAllFour                            },

  { "GET",    LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             badVerbAllFour                            },


  // version request
  { "GET",    VersionRequest,                              1, { "version"                                                     }, "",                                             versionTreat                              },
  { "*",      VersionRequest,                              1, { "version"                                                     }, "",                                             badVerbGetOnly                            },

  // statistics request
  { "GET",    StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "DELETE", StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "*",      StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             badVerbGetDeleteOnly                      },

#ifdef DEBUG
  { "GET",    ExitRequest,                                 2, { "exit", "*"                                                   }, "",                                             exitTreat                                 },
  { "GET",    ExitRequest,                                 1, { "exit"                                                        }, "",                                             exitTreat                                 },
  { "GET",    LeakRequest,                                 2, { "leak", "*"                                                   }, "",                                             leakTreat                                 },
  { "GET",    LeakRequest,                                 1, { "leak"                                                        }, "",                                             leakTreat                                 },
#endif

  // Bad requests
  { "*",      InvalidRequest,                              2, { "ngsi9",  "*"                                                 }, "",                                             badNgsi9Request                           },
  { "*",      InvalidRequest,                              2, { "ngsi10", "*"                                                 }, "",                                             badNgsi10Request                          },
  { "*",      InvalidRequest,                              0, { "*", "*", "*", "*", "*", "*"                                  }, "",                                             badRequest                                },

  // End marker for the array
  { "",       InvalidRequest,                              0, {                                                               }, "",                                             NULL                                      }
};



/* ****************************************************************************
*
* restServiceNgsi9 - services for CONF MAN without tenants
*
* This service vector (configuration) is used if the broker is started as
* CONFIGURATION MANAGER (using the -ngsi9 option) and without using the
* -multiservice option.
*/
RestService restServiceNgsi9[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },


  // NGSI-9 Convenience operations
  { "GET",    ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             badVerbGetPostOnly                        },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             badVerbPutDeleteOnly                      },


  // log request

  // The documentation (Installation and Admin Guide) says /log/trace ...
  { "GET",    LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "trace"                                                }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             badVerbAllFour                            },

  // ... and to maintain backward compatibility we keep supporting /log/traceLevel too
  { "GET",    LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             badVerbAllFour                            },

  // version request
  { "GET",    VersionRequest,                              1, { "version"                                                     }, "",                                             versionTreat                              },
  { "*",      VersionRequest,                              1, { "version"                                                     }, "",                                             badVerbGetOnly                            },

  // statistics request
  { "GET",    StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "DELETE", StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "*",      StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             badVerbGetDeleteOnly                      },

#ifdef DEBUG
  { "GET",    ExitRequest,                                 2, { "exit", "*"                                                   }, "",                                             exitTreat                                 },
  { "GET",    ExitRequest,                                 1, { "exit"                                                        }, "",                                             exitTreat                                 },
  { "GET",    LeakRequest,                                 2, { "leak", "*"                                                   }, "",                                             leakTreat                                 },
  { "GET",    LeakRequest,                                 1, { "leak"                                                        }, "",                                             leakTreat                                 },
#endif

  // Bad requests
  { "*",      InvalidRequest,                              2, { "ngsi9",  "*"                                                 }, "",                                             badNgsi9Request                           },
  { "*",      InvalidRequest,                              0, { "*", "*", "*", "*", "*", "*"                                  }, "",                                             badRequest                                },

  // End marker for the array
  { "",       InvalidRequest,                              0, {                                                               }, "",                                             NULL                                      }
};



/* ****************************************************************************
*
* restServiceNgsi9MTenant - services for CONF MAN with tenants
*
* This service vector (configuration) is used if the broker is started as
* CONFIGURATION MANAGER (using the -ngsi9 option) and also with the
* -multiservice option.
*/
RestService restServiceNgsi9MTenant[] =
{
  // NGSI-9 Requests
  { "POST",   RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             2, { "ngsi9",  "registerContext"                                   }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 2, { "ngsi9",  "discoverContextAvailability"                       }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9",  "subscribeContextAvailability"                      }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              2, { "ngsi9",  "unsubscribeContextAvailability"                    }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       2, { "ngsi9",  "updateContextAvailabilitySubscription"             }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },


  // NGSI-9 Requests with tenant
  { "POST",   RegisterContext,                             3, { "*", "ngsi9",  "registerContext"                              }, "registerContextRequest",                       postRegisterContext                       },
  { "*",      RegisterContext,                             3, { "*", "ngsi9",  "registerContext"                              }, "registerContextRequest",                       badVerbPostOnly                           },
  { "POST",   DiscoverContextAvailability,                 3, { "*", "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability           },
  { "*",      DiscoverContextAvailability,                 3, { "*", "ngsi9",  "discoverContextAvailability"                  }, "discoverContextAvailabilityRequest",           badVerbPostOnly                           },
  { "POST",   SubscribeContextAvailability,                3, { "*", "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                3, { "*", "ngsi9",  "subscribeContextAvailability"                 }, "subscribeContextAvailabilityRequest",          badVerbPostOnly                           },
  { "POST",   UnsubscribeContextAvailability,              3, { "*", "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability        },
  { "*",      UnsubscribeContextAvailability,              3, { "*", "ngsi9",  "unsubscribeContextAvailability"               }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly                           },
  { "POST",   UpdateContextAvailabilitySubscription,       3, { "*", "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription },
  { "*",      UpdateContextAvailabilitySubscription,       3, { "*", "ngsi9",  "updateContextAvailabilitySubscription"        }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly                           },
  { "POST",   NotifyContextAvailability,                   3, { "*", "ngsi9",  "notifyContextAvailability"                    }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability             },
  { "*",      NotifyContextAvailability,                   3, { "*", "ngsi9",  "notifyContextAvailability"                    }, "notifyContextAvailabilityRequest",             badVerbPostOnly                           },


  // NGSI-9 Convenience operations
  { "GET",    ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   3, { "ngsi9", "contextEntities", "*"                               } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     4, { "ngsi9", "contextEntities", "*", "attributes"                 } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   5, { "ngsi9", "contextEntities", "*", "attributes", "*"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          3, { "ngsi9", "contextEntityTypes", "*"                            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         4, { "ngsi9", "contextEntityTypes", "*", "attributes"              } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  5, { "ngsi9", "contextEntityTypes", "*", "attributes", "*"         }, "",                                             badVerbGetPostOnly                        },
  { "POST",   SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                2, { "ngsi9", "contextAvailabilitySubscriptions"                   }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    3, { "ngsi9", "contextAvailabilitySubscriptions", "*"              }, "",                                             badVerbPutDeleteOnly                      },


  // NGSI-9 Convenience operations with tenant
  { "GET",    ContextEntitiesByEntityId,                   4, { "*", "ngsi9", "contextEntities", "*"                          } , "",                                            getContextEntitiesByEntityId              },
  { "POST",   ContextEntitiesByEntityId,                   4, { "*", "ngsi9", "contextEntities", "*"                          } , "registerProviderRequest",                     postContextEntitiesByEntityId             },
  { "*",      ContextEntitiesByEntityId,                   4, { "*", "ngsi9", "contextEntities", "*"                          } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityAttributes,                     5, { "*", "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            getContextEntityAttributes                },
  { "POST",   ContextEntityAttributes,                     5, { "*", "ngsi9", "contextEntities", "*", "attributes"            } , "registerProviderRequest",                     postContextEntityAttributes               },
  { "*",      ContextEntityAttributes,                     5, { "*", "ngsi9", "contextEntities", "*", "attributes"            } , "",                                            badVerbGetPostOnly                        },

  { "GET",    EntityByIdAttributeByName,                   6, { "*", "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            getEntityByIdAttributeByName              },
  { "POST",   EntityByIdAttributeByName,                   6, { "*", "ngsi9", "contextEntities", "*", "attributes", "*"       } , "registerProviderRequest",                     postEntityByIdAttributeByName             },
  { "*",      EntityByIdAttributeByName,                   6, { "*", "ngsi9", "contextEntities", "*", "attributes", "*"       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypes,                          4, { "*", "ngsi9", "contextEntityTypes", "*"                       } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypes,                          4, { "*", "ngsi9", "contextEntityTypes", "*"                       } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypes,                          4, { "*", "ngsi9", "contextEntityTypes", "*"                       } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttributeContainer,         5, { "*", "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            getContextEntityTypes                     },
  { "POST",   ContextEntityTypeAttributeContainer,         5, { "*", "ngsi9", "contextEntityTypes", "*", "attributes"         } , "registerProviderRequest",                     postContextEntityTypes                    },
  { "*",      ContextEntityTypeAttributeContainer,         5, { "*", "ngsi9", "contextEntityTypes", "*", "attributes"         } , "",                                            badVerbGetPostOnly                        },

  { "GET",    ContextEntityTypeAttribute,                  6, { "*", "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             getContextEntityTypeAttribute             },
  { "POST",   ContextEntityTypeAttribute,                  6, { "*", "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "registerProviderRequest",                      postContextEntityTypeAttribute            },
  { "*",      ContextEntityTypeAttribute,                  6, { "*", "ngsi9", "contextEntityTypes", "*", "attributes", "*"    }, "",                                             badVerbGetPostOnly                        },

  { "POST",   SubscribeContextAvailability,                3, { "*", "ngsi9", "contextAvailabilitySubscriptions"              }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability          },
  { "*",      SubscribeContextAvailability,                3, { "*", "ngsi9", "contextAvailabilitySubscriptions"              }, "",                                             badVerbPostOnly                           },

  { "PUT",    Ngsi9SubscriptionsConvOp,                    4, { "*", "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp         },
  { "DELETE", Ngsi9SubscriptionsConvOp,                    4, { "*", "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             deleteAvailabilitySubscriptionConvOp      },
  { "*",      Ngsi9SubscriptionsConvOp,                    4, { "*", "ngsi9", "contextAvailabilitySubscriptions", "*"         }, "",                                             badVerbPutDeleteOnly                      },


  // log request
  { "GET",    LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "trace"                                                }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "trace"                                                }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "trace", "*"                                           }, "",                                             badVerbAllFour                            },

  { "GET",    LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "PUT",    LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             logTraceTreat                             },
  { "DELETE", LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                             },
  { "*",      LogRequest,                                  2, { "log", "traceLevel"                                           }, "",                                             badVerbAllFour                            },
  { "*",      LogRequest,                                  3, { "log", "traceLevel", "*"                                      }, "",                                             badVerbAllFour                            },

  // version request
  { "GET",    VersionRequest,                              1, { "version"                                                     }, "",                                             versionTreat                              },
  { "*",      VersionRequest,                              1, { "version"                                                     }, "",                                             badVerbGetOnly                            },

  // statistics request
  { "GET",    StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "DELETE", StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             statisticsTreat                           },
  { "*",      StatisticsRequest,                           1, { "statistics"                                                  }, "",                                             badVerbGetDeleteOnly                      },

#ifdef DEBUG
  { "GET",    ExitRequest,                                 2, { "exit", "*"                                                   }, "",                                             exitTreat                                 },
  { "GET",    ExitRequest,                                 1, { "exit"                                                        }, "",                                             exitTreat                                 },
  { "GET",    LeakRequest,                                 2, { "leak", "*"                                                   }, "",                                             leakTreat                                 },
  { "GET",    LeakRequest,                                 1, { "leak"                                                        }, "",                                             leakTreat                                 },
#endif

  // Bad requests
  { "*",      InvalidRequest,                              2, { "ngsi9",  "*"                                                 }, "",                                             badNgsi9Request                           },
  { "*",      InvalidRequest,                              0, { "*", "*", "*", "*", "*", "*"                                  }, "",                                             badRequest                                },

  // End marker for the array
  { "",       InvalidRequest,                              0, {                                                               }, "",                                             NULL                                      }
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
  {
    LM_E(("PID File (open '%s': %s", pidPath, strerror(errno)));
    return -1;
  }

  pid = getpid();

  sprintf(buffer, "%d", pid);
  sz = strlen(buffer);
  nb = write(fd, buffer, sz);
  if (nb != sz)
  {
    LM_E(("PID File (written %d bytes and not %d to '%s': %s)", nb, sz, pidPath, strerror(errno)));
    return -2;
  }

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
    LM_X(1, ("Fatal Error (fork: %s)", strerror(errno)));

  // Exiting father process
  if (pid > 0)
    exit(0);

  // Change the file mode mask */
  umask(0);

  // Removing the controlling terminal
  sid = setsid();
  if (sid == -1)
    LM_X(1, ("Fatal Error (setsid: %s)", strerror(errno)));

  // Change current working directory.
  // This prevents the current directory from being locked; hence not being able to remove it.
  if (chdir("/") == -1)
    LM_X(1, ("Fatal Error (chdir: %s)", strerror(errno)));

  // We have to call this after a fork, see: http://api.mongodb.org/cplusplus/2.2.2/classmongo_1_1_o_i_d.html
  OID::justForked();
}

/* ****************************************************************************
*
* sigHandler - 
*/
void sigHandler(int sigNo)
{
  LM_I(("Signal Handler (caught signal %d)", sigNo));

  switch (sigNo)
  {
  case SIGINT:
  case SIGTERM:
    LM_I(("Orion context broker exiting due to receiving a signal"));
    exit(0);
    break;
  }
}

/* ****************************************************************************
*
* orionExit - 
*/
void orionExit(int code, const std::string& reason)
{
  if (code == 0)
    LM_I(("Orion context broker exits in an ordered manner (%s)", reason.c_str()));
  else
    LM_E(("Fatal Error (reason: %s)", reason.c_str()));

  exit(code);
}

/* ****************************************************************************
*
* exitFunc - 
*/
void exitFunc(void)
{
  curl_global_cleanup();

  if (unlink(pidPath) != 0)
    LM_T(LmtSoftError, ("error removing PID file '%s': %s", pidPath, strerror(errno)));
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
* contextBrokerInit -
*/
static void contextBrokerInit(bool ngsi9Only, std::string dbPrefix, bool multitenant)
{
  /* Set notifier object (singleton) */
  setNotifier(new Notifier());

  /* Launch threads corresponding to ONTIMEINTERVAL subscriptions in the database (unless ngsi9 only mode) */
  if (!ngsi9Only) {
    recoverOntimeIntervalThreads("");
    if (multitenant) {
        /* We get tenant database names and recover ontime interval threads on each one */
        std::vector<std::string> orionDbs;
        getOrionDatabases(orionDbs);
        for (unsigned int ix = 0; ix < orionDbs.size(); ++ix) {
            std::string orionDb = orionDbs[ix];
            std::string tenant = orionDb.substr(dbPrefix.length() + 1);   // + 1 for the "_" in "orion_tenantA"
            recoverOntimeIntervalThreads(tenant);
        }
    }
  }
  else
    LM_I(("Running in NGSI9 only mode"));
}

/* ****************************************************************************
*
* mongoInit - 
*/
static void mongoInit(const char* dbHost, const char* rplSet, std::string dbName, const char* user, const char* pwd)
{
   std::string multitenant = mtenant;

   if (!mongoConnect(dbHost, dbName.c_str(), rplSet, user, pwd, multitenant != "off"))
    LM_X(1, ("Fatal Error (MongoDB error)"));

  if (user[0] != 0) 
    LM_I(("Connected to mongo at %s:%s as user '%s'", dbHost, dbName.c_str(), user));
  else
    LM_I(("Connected to mongo at %s:%s", dbHost, dbName.c_str()));

  setDbPrefix(dbName);
  setEntitiesCollectionName("entities");
  setRegistrationsCollectionName("registrations");
  setSubscribeContextCollectionName("csubs");
  setSubscribeContextAvailabilityCollectionName("casubs");
  setAssociationsCollectionName("associations");

  /* Note that index creation operation is idempotent. From http://docs.mongodb.org/manual/reference/method/db.collection.ensureIndex/,
   * "If you call multiple ensureIndex() methods with the same index specification at the same time, only the first operation will
   * succeed, all other operations will have no effect." */
  ensureLocationIndex("");
  if (multitenant != "off") {
      /* We get tenant database names and apply ensure the location index in each one */
      std::vector<std::string> orionDbs;
      getOrionDatabases(orionDbs);
      for (unsigned int ix = 0; ix < orionDbs.size(); ++ix) {
          std::string orionDb = orionDbs[ix];
          std::string tenant = orionDb.substr(dbName.length() + 1);   // + 1 for the "_" in "orion_tenantA"
          ensureLocationIndex(tenant);
      }
  }
}



/* ****************************************************************************
*
* loadFile - 
*/
static int loadFile(char* path, char* out, int outSize)
{
  struct stat  statBuf;
  int          nb;
  int          fd = open(path, O_RDONLY);

  if (fd == -1)
  {
    LM_E(("HTTPS Error (error opening '%s': %s)", path, strerror(errno)));
    return -1;
  }

  if (stat(path, &statBuf) != 0)
  {
    close(fd);
    LM_E(("HTTPS Error (error 'stating' '%s': %s)", path, strerror(errno)));
    return -1;
  }

  if (statBuf.st_size > outSize)
  {
    close(fd);
    LM_E(("HTTPS Error (file '%s' is TOO BIG (%d) - max size is %d bytes)", path, outSize));
    return -1;
  }

  nb = read(fd, out, statBuf.st_size);
  close(fd);

  if (nb == -1)
  {
    LM_E(("HTTPS Error (reading from '%s': %s)", path, strerror(errno)));
    return -1;
  }

  if (nb != statBuf.st_size)
  {
    LM_E(("HTTPS Error (invalid size read from '%s': %d, wanted %d)", path, nb, statBuf.st_size));
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* rushParse - parse rush host and port from CLI argument
*
* The '-rush' CLI argument has the format "host:port" and this function
* splits that argument into rushHost and rushPort.
* If there is a syntax error in the argument, the function exists the program
* with an error message
*/
static void rushParse(char* rush, std::string* rushHostP, unsigned short* rushPortP)
{
  char* colon = strchr(rush, ':');
  char* copy  = strdup(rush);

  if (colon == NULL)
    LM_X(1, ("Fatal Error (Bad syntax of '-rush' value: '%s' - expected syntax: 'host:port')", rush));

  *colon = 0;
  ++colon;

  *rushHostP = rush;
  *rushPortP = atoi(colon);

  if ((*rushHostP == "") || (*rushPortP == 0))
    LM_X(1, ("Fatal Error (bad syntax of '-rush' value: '%s' - expected syntax: 'host:port')", copy));

  free(copy);
}



/* ****************************************************************************
*
* main - 
*/
int main(int argC, char* argV[])
{
  strncpy(transactionId, "N/A", sizeof(transactionId));

  unsigned short rushPort = 0;
  std::string    rushHost = "";

  signal(SIGINT,  sigHandler);
  signal(SIGTERM, sigHandler);

  atexit(exitFunc);

  paConfig("remove builtin", "-d");
  paConfig("remove builtin", "-r");
  paConfig("remove builtin", "-w");
  paConfig("remove builtin", "-F");
  paConfig("remove builtin", "-B");
  paConfig("remove builtin", "-b");
  paConfig("remove builtin", "-?");
  paConfig("remove builtin", "-toDo");
  paConfig("remove builtin", "-lmnc");
  paConfig("remove builtin", "-lmca");
  paConfig("remove builtin", "-lmkl");
  paConfig("remove builtin", "-lmll");
  paConfig("remove builtin", "-assert");
  paConfig("remove builtin", "-version");
  paConfig("remove builtin", "-h");
  paConfig("remove builtin", "-help");
  paConfig("remove builtin", "-v");
  paConfig("remove builtin", "-vv");
  paConfig("remove builtin", "-vvv");
  paConfig("remove builtin", "-vvvv");
  paConfig("remove builtin", "-vvvvv");

  paConfig("man synopsis",                  (void*) "[options]");
  paConfig("man shortdescription",          (void*) "Options:");
  paConfig("man description",               (void*) description);
  paConfig("man author",                    (void*) "Telefonica I+D");
  paConfig("man exitstatus",                (void*) "The orion broker is a daemon. If it exits, something is wrong ...");
  paConfig("man version",                   (void*) ORION_VERSION);
  paConfig("log to screen",                 (void*) true);
  paConfig("log to file",                   (void*) true);
  paConfig("log file line format",          (void*) "time=DATE | lvl=TYPE | trans=TRANS_ID | function=FUNC | comp=Orion | msg=FILE[LINE]: TEXT");
  paConfig("screen line format",            (void*) "TYPE@TIME  FILE[LINE]: TEXT");
  paConfig("builtin prefix",                (void*) "ORION_");
  paConfig("usage and exit on any warning", (void*) true);
  paConfig("no preamble",                   NULL);
  paConfig("log file time format",          (void*) "%Y-%m-%dT%H:%M:%S");

  paParse(paArgs, argC, (char**) argV, 1, false);
  lmTimeFormat(0, (char*) "%Y-%m-%dT%H:%M:%S");
  LM_I(("Orion Context Broker is running"));

  std::string multitenant = mtenant;
  if ((multitenant != "off") && (multitenant != "header") && (multitenant != "url"))
    LM_X(1, ("Fatal Error (bad value for -multiservice ['%s']. Allowed values: 'off', 'header' and 'url')", mtenant));

  if (useOnlyIPv6 && useOnlyIPv4)
    LM_X(1, ("Fatal Error (-ipv4 and -ipv6 can not be activated at the same time. They are incompatible)"));

  if (https)
  {
    if (httpsKeyFile[0] == 0)
      LM_X(1, ("Fatal Error (when option '-https' is used, option '-key' is mandatory)"));
    if (httpsCertFile[0] == 0)
      LM_X(1, ("Fatal Error (when option '-https' is used, option '-cert' is mandatory)"));
  }  

  if (fg == false)
    daemonize();

#if 0
  //
  // This 'almost always outdeffed' piece of code is used whenever a change is done to the 
  // valgrind test suite, just to make sure that the tool actually detects memory leaks,
  //
  char* x = (char*) malloc(100000);
  sprintf(x, "A hundred thousand bytes lost here");
  LM_M(("x: '%s'", x));
  x = (char*) "LOST";
  LM_M(("x: '%s'", x));
#endif

  RestService* rsP = restServiceV;
  if      (ngsi9Only  && multitenant == "url") rsP = restServiceNgsi9MTenant;
  else if (!ngsi9Only && multitenant == "url") rsP = restServiceMTenant;
  else if (ngsi9Only)                          rsP = restServiceNgsi9;

  IpVersion    ipVersion = IPDUAL;
  if      (useOnlyIPv4)    ipVersion = IPV4;
  else if (useOnlyIPv6)    ipVersion = IPV6;

  pidFile();
  orionInit(orionExit, ORION_VERSION);
  mongoInit(dbHost, rplSet, dbName, user, pwd);
  contextBrokerInit(ngsi9Only, dbName, multitenant != "off");
  curl_global_init(CURL_GLOBAL_NOTHING);

  if (rush[0] != 0)
  {
    rushParse(rush, &rushHost, &rushPort);
    LM_T(LmtRush, ("rush host: '%s', rush port: %d", rushHost.c_str(), rushPort));
  }

  if (https)
  {
    char* httpsPrivateServerKey = (char*) malloc(2048);
    char* httpsCertificate      = (char*) malloc(2048);
    
    if (loadFile(httpsKeyFile, httpsPrivateServerKey, 2048) != 0)
      LM_X(1, ("Fatal Error (loading private server key from '%s')", httpsKeyFile));
    if (loadFile(httpsCertFile, httpsCertificate, 2048) != 0)
      LM_X(1, ("Fatal Error (loading certificate from '%s')", httpsCertFile));

    LM_T(LmtHttps, ("httpsKeyFile:  '%s'", httpsKeyFile));
    LM_T(LmtHttps, ("httpsCertFile: '%s'", httpsCertFile));

    restInit(rsP, ipVersion, bindAddress, port, mtenant, rushHost, rushPort, httpsPrivateServerKey, httpsCertificate);

    free(httpsPrivateServerKey);
    free(httpsCertificate);
  }
  else
    restInit(rsP, ipVersion, bindAddress, port, mtenant, rushHost, rushPort);

  LM_I(("Startup completed"));

  while (1)
    sleep(10);
}
