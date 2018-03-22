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
#include "serviceRoutines/getEntityTypes.h"
#include "serviceRoutines/getAttributesForEntityType.h"
#include "serviceRoutines/getAllContextEntities.h"
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
#include "serviceRoutines/postSubscribeContextConvOp.h"
#include "serviceRoutines/postSubscribeContextAvailabilityConvOp.h"
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
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutines/postIndividualContextEntity.h"
#include "serviceRoutines/deleteIndividualContextEntity.h"
#include "serviceRoutines/badVerbAllFour.h"
#include "serviceRoutines/badVerbAllFive.h"
#include "serviceRoutines/badVerbPutOnly.h"
#include "serviceRoutines/putIndividualContextEntityAttribute.h"
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
#include "serviceRoutines/getAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/postAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/putAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/deleteAllEntitiesWithTypeAndId.h"
#include "serviceRoutines/getIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/postIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/putIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/deleteIndividualContextEntityAttributeWithTypeAndId.h"
#include "serviceRoutines/getAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/deleteAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/postAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/putAttributeValueInstanceWithTypeAndId.h"
#include "serviceRoutines/getContextEntitiesByEntityIdAndType.h"
#include "serviceRoutines/postContextEntitiesByEntityIdAndType.h"
#include "serviceRoutines/getEntityByIdAttributeByNameWithTypeAndId.h"
#include "serviceRoutines/postEntityByIdAttributeByNameWithTypeAndId.h"
#include "serviceRoutines/badVerbGetPutDeleteOnly.h"
#include "serviceRoutines/badVerbGetPostDeleteOnly.h"
#include "serviceRoutines/badVerbGetOnly.h"
#include "serviceRoutines/badVerbGetDeleteOnly.h"
#include "serviceRoutinesV2/badVerbGetPutOnly.h"
#include "serviceRoutinesV2/badVerbGetDeletePatchOnly.h"
#include "serviceRoutines/badNgsi9Request.h"
#include "serviceRoutines/badNgsi10Request.h"
#include "serviceRoutines/badRequest.h"
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
#include "serviceRoutines/optionsVersionRequest.h"

#include "rest/RestService.h"
#include "rest/rest.h"
#include "contextBroker/orionRestServices.h"



/* ****************************************************************************
*
* getServiceV - 
*/
static RestService getServiceV[] =
{
  { EntryPointsRequest,                            1, { "v2"                                                                           }, "",  entryPointsTreat                                 },
  { EntitiesRequest,                               2, { "v2", "entities"                                                               }, "",  getEntities                                      },
  { EntityRequest,                                 3, { "v2", "entities", "*"                                                          }, "",  getEntity                                        },
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                                 }, "",  getEntity                                        },
  { EntityAttributeValueRequest,                   6, { "v2", "entities", "*", "attrs", "*", "value"                                   }, "",  getEntityAttributeValue                          },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                            }, "",  getEntityAttribute                               },
  { EntityTypeRequest,                             3, { "v2", "types", "*"                                                             }, "",  getEntityType                                    },
  { EntityAllTypesRequest,                         2, { "v2", "types"                                                                  }, "",  getEntityAllTypes                                },
  { SubscriptionsRequest,                          2, { "v2", "subscriptions"                                                          }, "",  getAllSubscriptions                              },
  { IndividualSubscriptionRequest,                 3, { "v2", "subscriptions", "*"                                                     }, "",  getSubscription                                  },
  { ContextEntitiesByEntityId,                     3, { "ngsi9", "contextEntities", "*"                                                }, "",  getContextEntitiesByEntityId                     },
  { ContextEntityAttributes,                       4, { "ngsi9",          "contextEntities", "*", "attributes"                         }, "",  getContextEntityAttributes                       },
  { EntityByIdAttributeByName,                     5, { "ngsi9",          "contextEntities", "*", "attributes", "*"                    }, "",  getEntityByIdAttributeByName                     },
  { ContextEntityTypes,                            3, { "ngsi9",          "contextEntityTypes", "*"                                    }, "",  getContextEntityTypes                            },
  { ContextEntityTypeAttributeContainer,           4, { "ngsi9",          "contextEntityTypes", "*", "attributes"                      }, "",  getContextEntityTypes                            },
  { ContextEntityTypeAttribute,                    5, { "ngsi9",          "contextEntityTypes", "*", "attributes", "*"                 }, "",  getContextEntityTypeAttribute                    },
  { ContextEntitiesByEntityId,                     4, { "v1", "registry", "contextEntities", "*"                                       }, "",  getContextEntitiesByEntityId                     },
  { ContextEntityAttributes,                       5, { "v1", "registry", "contextEntities", "*", "attributes"                         }, "",  getContextEntityAttributes                       },
  { EntityByIdAttributeByName,                     6, { "v1", "registry", "contextEntities", "*", "attributes", "*"                    }, "",  getEntityByIdAttributeByName                     },
  { ContextEntityTypes,                            4, { "v1", "registry", "contextEntityTypes", "*"                                    }, "",  getContextEntityTypes                            },
  { ContextEntityTypeAttributeContainer,           5, { "v1", "registry", "contextEntityTypes", "*", "attributes"                      }, "",  getContextEntityTypes                            },
  { ContextEntityTypeAttribute,                    6, { "v1", "registry", "contextEntityTypes", "*", "attributes", "*"                 }, "",  getContextEntityTypeAttribute                    },
  { IndividualContextEntity,                       3, { "ngsi10",  "contextEntities", "*"                                              }, "",  getIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "ngsi10",  "contextEntities", "*", "attributes"                                }, "",  getIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "ngsi10",  "contextEntities", "*", "attributes", "*"                           }, "",  getIndividualContextEntityAttribute              },
  { AttributeValueInstance,                        6, { "ngsi10",  "contextEntities", "*", "attributes", "*", "*"                      }, "",  getAttributeValueInstance                        },
  { Ngsi10ContextEntityTypes,                      3, { "ngsi10",  "contextEntityTypes", "*"                                           }, "",  getNgsi10ContextEntityTypes                      },
  { Ngsi10ContextEntityTypesAttributeContainer,    4, { "ngsi10",  "contextEntityTypes", "*", "attributes"                             }, "",  getNgsi10ContextEntityTypes                      },
  { Ngsi10ContextEntityTypesAttribute,             5, { "ngsi10",  "contextEntityTypes", "*", "attributes", "*"                        }, "",  getNgsi10ContextEntityTypesAttribute             },
  { IndividualContextEntity,                       3, { "v1",      "contextEntities", "*"                                              }, "",  getIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "v1",      "contextEntities", "*", "attributes"                                }, "",  getIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "v1",      "contextEntities", "*", "attributes", "*"                           }, "",  getIndividualContextEntityAttribute              },
  { AttributeValueInstance,                        6, { "v1",      "contextEntities", "*", "attributes", "*", "*"                      }, "",  getAttributeValueInstance                        },
  { Ngsi10ContextEntityTypes,                      3, { "v1",      "contextEntityTypes", "*"                                           }, "",  getNgsi10ContextEntityTypes                      },
  { Ngsi10ContextEntityTypesAttributeContainer,    4, { "v1",      "contextEntityTypes", "*", "attributes"                             }, "",  getNgsi10ContextEntityTypes                      },
  { Ngsi10ContextEntityTypesAttribute,             5, { "v1",      "contextEntityTypes", "*", "attributes", "*"                        }, "",  getNgsi10ContextEntityTypesAttribute             },
  { EntityTypes,                                   2, { "v1", "contextTypes"                                                           }, "",  getEntityTypes                                   },
  { AttributesForEntityType,                       3, { "v1", "contextTypes", "*"                                                      }, "",  getAttributesForEntityType                       },
  { AllContextEntities,                            2, { "v1", "contextEntities"                                                        }, "",  getAllContextEntities                            },
  { AllEntitiesWithTypeAndId,                      6, { "v1", "contextEntities", "type", "*", "id", "*"                                }, "",  getAllEntitiesWithTypeAndId                      },
  { IndividualContextEntityAttributeWithTypeAndId, 8, { "v1", "contextEntities", "type", "*", "id", "*", "attributes", "*"             }, "",  getIndividualContextEntityAttributeWithTypeAndId },
  { AttributeValueInstanceWithTypeAndId,           9, { "v1",      "contextEntities", "type", "*", "id", "*", "attributes", "*", "*"   }, "",  getAttributeValueInstanceWithTypeAndId           },
  { ContextEntitiesByEntityIdAndType,              7, { "v1", "registry", "contextEntities", "type", "*", "id", "*"                    }, "",  getContextEntitiesByEntityIdAndType              },
  { EntityByIdAttributeByNameIdAndType,            9, { "v1", "registry", "contextEntities", "type", "*", "id", "*", "attributes", "*" }, "",  getEntityByIdAttributeByNameWithTypeAndId        },
  { LogTraceRequest,                               2, { "log", "trace"                                                                 }, "",  logTraceTreat                                    },
  { LogTraceRequest,                               2, { "log", "traceLevel"                                                            }, "",  logTraceTreat                                    },
  { LogTraceRequest,                               4, { "v1", "admin", "log", "trace"                                                  }, "",  logTraceTreat                                    },
  { LogTraceRequest,                               4, { "v1", "admin", "log", "traceLevel"                                             }, "",  logTraceTreat                                    },
  { StatisticsRequest,                             1, { "statistics"                                                                   }, "",  statisticsTreat                                  },
  { StatisticsRequest,                             3, { "v1", "admin", "statistics"                                                    }, "",  statisticsTreat                                  },
  { StatisticsRequest,                             2, { "cache", "statistics"                                                          }, "",  statisticsCacheTreat                             },
  { StatisticsRequest,                             4, { "v1", "admin", "cache", "statistics"                                           }, "",  statisticsCacheTreat                             },
  { VersionRequest,                                1, { "version"                                                                      }, "",  versionTreat                                     },
  { LogLevelRequest,                               2, { "admin", "log"                                                                 }, "",  getLogLevel                                      },
  { SemStateRequest,                               2, { "admin", "sem"                                                                 }, "",  semStateTreat                                    },
  { MetricsRequest,                                2, { "admin", "metrics"                                                             }, "",  getMetrics                                       },

#ifdef DEBUG
  { ExitRequest,                                   2, { "exit", "*"                                                                    }, "",  exitTreat                                        },
  { ExitRequest,                                   1, { "exit"                                                                         }, "",  exitTreat                                        },
  { LeakRequest,                                   2, { "leak", "*"                                                                    }, "",  leakTreat                                        },
  { LeakRequest,                                   1, { "leak"                                                                         }, "",  leakTreat                                        },
#endif

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* postServiceV - 
*/
static RestService postServiceV[] =
{
  { EntitiesRequest,                               2, { "v2", "entities"                                                               }, "",                                             postEntities                                      },
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                                 }, "",                                             postEntity                                        },
  { SubscriptionsRequest,                          2, { "v2", "subscriptions"                                                          }, "",                                             postSubscriptions                                 },
  { BatchQueryRequest,                             3, { "v2", "op", "query"                                                            }, "",                                             postBatchQuery                                    },
  { BatchUpdateRequest,                            3, { "v2", "op", "update"                                                           }, "",                                             postBatchUpdate                                   },
  { RegisterContext,                               2, { "ngsi9",          "registerContext"                                            }, "registerContextRequest",                       postRegisterContext                               },
  { DiscoverContextAvailability,                   2, { "ngsi9",          "discoverContextAvailability"                                }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability                   },
  { SubscribeContextAvailability,                  2, { "ngsi9",          "subscribeContextAvailability"                               }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability                  },
  { UnsubscribeContextAvailability,                2, { "ngsi9",          "unsubscribeContextAvailability"                             }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability                },
  { UpdateContextAvailabilitySubscription,         2, { "ngsi9",          "updateContextAvailabilitySubscription"                      }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription         },
  { NotifyContextAvailability,                     2, { "ngsi9",          "notifyContextAvailability"                                  }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability                     },
  { RegisterContext,                               3, { "v1", "registry", "registerContext"                                            }, "registerContextRequest",                       postRegisterContext                               },
  { DiscoverContextAvailability,                   3, { "v1", "registry", "discoverContextAvailability"                                }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability                   },
  { SubscribeContextAvailability,                  3, { "v1", "registry", "subscribeContextAvailability"                               }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability                  },
  { UnsubscribeContextAvailability,                3, { "v1", "registry", "unsubscribeContextAvailability"                             }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability                },
  { UpdateContextAvailabilitySubscription,         3, { "v1", "registry", "updateContextAvailabilitySubscription"                      }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription         },
  { NotifyContextAvailability,                     3, { "v1", "registry", "notifyContextAvailability"                                  }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability                     },
  { RegisterContext,                               2, { "ngsi9",          "registerContext"                                            }, "registerContextRequest",                       postRegisterContext                               },
  { DiscoverContextAvailability,                   2, { "ngsi9",          "discoverContextAvailability"                                }, "discoverContextAvailabilityRequest",           postDiscoverContextAvailability                   },
  { SubscribeContextAvailability,                  2, { "ngsi9",          "subscribeContextAvailability"                               }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability                  },
  { UnsubscribeContextAvailability,                2, { "ngsi9",          "unsubscribeContextAvailability"                             }, "unsubscribeContextAvailabilityRequest",        postUnsubscribeContextAvailability                },
  { UpdateContextAvailabilitySubscription,         2, { "ngsi9",          "updateContextAvailabilitySubscription"                      }, "updateContextAvailabilitySubscriptionRequest", postUpdateContextAvailabilitySubscription         },
  { NotifyContextAvailability,                     2, { "ngsi9",          "notifyContextAvailability"                                  }, "notifyContextAvailabilityRequest",             postNotifyContextAvailability                     },
  { UpdateContext,                                 2, { "v1",      "updateContext"                                                     }, "updateContextRequest",                         (RestTreat) postUpdateContext                     },
  { QueryContext,                                  2, { "v1",      "queryContext"                                                      }, "queryContextRequest",                          postQueryContext                                  },
  { SubscribeContext,                              2, { "v1",      "subscribeContext"                                                  }, "subscribeContextRequest",                      postSubscribeContext                              },
  { UpdateContextSubscription,                     2, { "v1",      "updateContextSubscription"                                         }, "updateContextSubscriptionRequest",             postUpdateContextSubscription                     },
  { UnsubscribeContext,                            2, { "v1",      "unsubscribeContext"                                                }, "unsubscribeContextRequest",                    postUnsubscribeContext                            },
  { NotifyContext,                                 2, { "v1",      "notifyContext"                                                     }, "notifyContextRequest",                         postNotifyContext                                 },
  { ContextEntitiesByEntityId,                     3, { "ngsi9",          "contextEntities", "*"                                       }, "registerProviderRequest",                      postContextEntitiesByEntityId                     },
  { ContextEntityAttributes,                       4, { "ngsi9",          "contextEntities", "*", "attributes"                         }, "registerProviderRequest",                      postContextEntityAttributes                       },
  { EntityByIdAttributeByName,                     5, { "ngsi9",          "contextEntities", "*", "attributes", "*"                    }, "registerProviderRequest",                      postEntityByIdAttributeByName                     },
  { ContextEntityTypes,                            3, { "ngsi9",          "contextEntityTypes", "*"                                    }, "registerProviderRequest",                      postContextEntityTypes                            },
  { ContextEntityTypeAttributeContainer,           4, { "ngsi9",          "contextEntityTypes", "*", "attributes"                      }, "registerProviderRequest",                      postContextEntityTypes                            },
  { ContextEntityTypeAttribute,                    5, { "ngsi9",          "contextEntityTypes", "*", "attributes", "*"                 }, "registerProviderRequest",                      postContextEntityTypeAttribute                    },
  { SubscribeContextAvailability,                  2, { "ngsi9",          "contextAvailabilitySubscriptions"                           }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailabilityConvOp            },
  { ContextEntitiesByEntityId,                     4, { "v1", "registry", "contextEntities", "*"                                       }, "registerProviderRequest",                      postContextEntitiesByEntityId                     },
  { ContextEntityAttributes,                       5, { "v1", "registry", "contextEntities", "*", "attributes"                         }, "registerProviderRequest",                      postContextEntityAttributes                       },
  { EntityByIdAttributeByName,                     6, { "v1", "registry", "contextEntities", "*", "attributes", "*"                    }, "registerProviderRequest",                      postEntityByIdAttributeByName                     },
  { ContextEntityTypes,                            4, { "v1", "registry", "contextEntityTypes", "*"                                    }, "registerProviderRequest",                      postContextEntityTypes                            },
  { ContextEntityTypeAttributeContainer,           5, { "v1", "registry", "contextEntityTypes", "*", "attributes"                      }, "registerProviderRequest",                      postContextEntityTypes                            },
  { ContextEntityTypeAttribute,                    6, { "v1", "registry", "contextEntityTypes", "*", "attributes", "*"                 }, "registerProviderRequest",                      postContextEntityTypeAttribute                    },
  { SubscribeContextAvailability,                  3, { "v1", "registry", "contextAvailabilitySubscriptions"                           }, "subscribeContextAvailabilityRequest",          postSubscribeContextAvailability                  },
  { IndividualContextEntity,                       3, { "ngsi10",  "contextEntities", "*"                                              }, "appendContextElementRequest",                  postIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "ngsi10",  "contextEntities", "*", "attributes"                                }, "appendContextElementRequest",                  postIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "ngsi10",  "contextEntities", "*", "attributes", "*"                           }, "updateContextAttributeRequest",                postIndividualContextEntityAttribute              },
  { SubscribeContext,                              2, { "ngsi10",  "contextSubscriptions"                                              }, "subscribeContextRequest",                      postSubscribeContextConvOp                        },
  { IndividualContextEntity,                       3, { "v1",      "contextEntities", "*"                                              }, "appendContextElementRequest",                  postIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "v1",      "contextEntities", "*", "attributes"                                }, "appendContextElementRequest",                  postIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "v1",      "contextEntities", "*", "attributes", "*"                           }, "updateContextAttributeRequest",                postIndividualContextEntityAttribute              },
  { SubscribeContext,                              2, { "v1",      "contextSubscriptions"                                              }, "subscribeContextRequest",                      postSubscribeContextConvOp                        },
  { AllContextEntities,                            2, { "v1", "contextEntities"                                                        }, "appendContextElementRequest",                  postIndividualContextEntity                       },
  { AllEntitiesWithTypeAndId,                      6, { "v1", "contextEntities", "type", "*", "id", "*"                                }, "appendContextElementRequest",                  postAllEntitiesWithTypeAndId                      },
  { IndividualContextEntityAttributeWithTypeAndId, 8, { "v1", "contextEntities", "type", "*", "id", "*", "attributes", "*"             }, "updateContextAttributeRequest",                postIndividualContextEntityAttributeWithTypeAndId },
  { AttributeValueInstanceWithTypeAndId,           9, { "v1", "contextEntities", "type", "*", "id", "*", "attributes", "*", "*"        }, "updateContextAttributeRequest",                postAttributeValueInstanceWithTypeAndId           },
  { ContextEntitiesByEntityIdAndType,              7, { "v1", "registry", "contextEntities", "type", "*", "id", "*"                    }, "registerProviderRequest",                      postContextEntitiesByEntityIdAndType              },
  { EntityByIdAttributeByNameIdAndType,            9, { "v1", "registry", "contextEntities", "type", "*", "id", "*", "attributes", "*" }, "registerProviderRequest",                      postEntityByIdAttributeByNameWithTypeAndId        },
  { UpdateContext,                                 2, { "ngsi10",  "updateContext"                                                     }, "updateContextRequest",                         (RestTreat) postUpdateContext                     },
  { QueryContext,                                  2, { "ngsi10",  "queryContext"                                                      }, "queryContextRequest",                          postQueryContext                                  },
  { SubscribeContext,                              2, { "ngsi10",  "subscribeContext"                                                  }, "subscribeContextRequest",                      postSubscribeContext                              },
  { UpdateContextSubscription,                     2, { "ngsi10",  "updateContextSubscription"                                         }, "updateContextSubscriptionRequest",             postUpdateContextSubscription                     },
  { UnsubscribeContext,                            2, { "ngsi10",  "unsubscribeContext"                                                }, "unsubscribeContextRequest",                    postUnsubscribeContext                            },
  { NotifyContext,                                 2, { "ngsi10",  "notifyContext"                                                     }, "notifyContextRequest",                         postNotifyContext                                 },

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* putServiceV - 
*/
static RestService putServiceV[] =
{
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                               }, "",                                             putEntity                                        },
  { EntityAttributeValueRequest,                   6, { "v2", "entities", "*", "attrs", "*", "value"                                 }, "",                                             putEntityAttributeValue                          },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                          }, "",                                             putEntityAttribute                               },
  { Ngsi9SubscriptionsConvOp,                      3, { "ngsi9",          "contextAvailabilitySubscriptions", "*"                    }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp                },
  { Ngsi9SubscriptionsConvOp,                      4, { "v1", "registry", "contextAvailabilitySubscriptions", "*"                    }, "updateContextAvailabilitySubscriptionRequest", putAvailabilitySubscriptionConvOp                },
  { IndividualContextEntity,                       3, { "ngsi10",  "contextEntities", "*"                                            }, "updateContextElementRequest",                  putIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "ngsi10",  "contextEntities", "*", "attributes"                              }, "updateContextElementRequest",                  putIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "ngsi10",  "contextEntities", "*", "attributes", "*"                         }, "updateContextAttributeRequest",                putIndividualContextEntityAttribute              },
  { AttributeValueInstance,                        6, { "ngsi10",  "contextEntities", "*", "attributes", "*", "*"                    }, "updateContextAttributeRequest",                putAttributeValueInstance                        },
  { Ngsi10SubscriptionsConvOp,                     3, { "ngsi10",  "contextSubscriptions", "*"                                       }, "updateContextSubscriptionRequest",             putSubscriptionConvOp                            },
  { IndividualContextEntity,                       3, { "v1",      "contextEntities", "*"                                            }, "updateContextElementRequest",                  putIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "v1",      "contextEntities", "*", "attributes"                              }, "updateContextElementRequest",                  putIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "v1",      "contextEntities", "*", "attributes", "*"                         }, "updateContextAttributeRequest",                putIndividualContextEntityAttribute              },
  { AttributeValueInstance,                        6, { "v1",      "contextEntities", "*", "attributes", "*", "*"                    }, "updateContextAttributeRequest",                putAttributeValueInstance                        },
  { Ngsi10SubscriptionsConvOp,                     3, { "v1",      "contextSubscriptions", "*"                                       }, "updateContextSubscriptionRequest",             putSubscriptionConvOp                            },
  { AllEntitiesWithTypeAndId,                      6, { "v1", "contextEntities", "type", "*", "id", "*"                              }, "updateContextElementRequest",                  putAllEntitiesWithTypeAndId                      },
  { IndividualContextEntityAttributeWithTypeAndId, 8, { "v1", "contextEntities", "type", "*", "id", "*", "attributes", "*"           }, "updateContextAttributeRequest",                putIndividualContextEntityAttributeWithTypeAndId },
  { AttributeValueInstanceWithTypeAndId,           9, { "v1",      "contextEntities", "type", "*", "id", "*", "attributes", "*", "*" }, "updateContextAttributeRequest",                putAttributeValueInstanceWithTypeAndId           },
  { LogTraceRequest,                               3, { "log", "trace",      "*"                                                     }, "",                                             logTraceTreat                                    },
  { LogTraceRequest,                               3, { "log", "traceLevel", "*"                                                     }, "",                                             logTraceTreat                                    },
  { LogTraceRequest,                               5, { "v1", "admin", "log", "trace",      "*"                                      }, "",                                             logTraceTreat                                    },
  { LogTraceRequest,                               5, { "v1", "admin", "log", "traceLevel", "*"                                      }, "",                                             logTraceTreat                                    },
  { LogLevelRequest,                               2, { "admin", "log"                                                               }, "",                                             changeLogLevel                                   },

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* patchServiceV - 
*/
static RestService patchServiceV[] =
{
  { EntityRequest,                 4, { "v2", "entities", "*", "attrs" }, "", patchEntity       },
  { IndividualSubscriptionRequest, 3, { "v2", "subscriptions", "*"     }, "", patchSubscription },

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* deleteServiceV - 
*/
static RestService deleteServiceV[] =
{
  { EntityRequest,                                 3, { "v2", "entities", "*"                                                        }, "", deleteEntity                                        },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                          }, "", deleteEntity                                        },
  { IndividualSubscriptionRequest,                 3, { "v2", "subscriptions", "*"                                                   }, "", deleteSubscription                                  },
  { Ngsi9SubscriptionsConvOp,                      3, { "ngsi9",          "contextAvailabilitySubscriptions", "*"                    }, "", deleteAvailabilitySubscriptionConvOp                },
  { Ngsi9SubscriptionsConvOp,                      4, { "v1", "registry", "contextAvailabilitySubscriptions", "*"                    }, "", deleteAvailabilitySubscriptionConvOp                },
  { IndividualContextEntity,                       3, { "ngsi10",  "contextEntities", "*"                                            }, "", deleteIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "ngsi10",  "contextEntities", "*", "attributes"                              }, "", deleteIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "ngsi10",  "contextEntities", "*", "attributes", "*"                         }, "", deleteIndividualContextEntityAttribute              },
  { AttributeValueInstance,                        6, { "ngsi10",  "contextEntities", "*", "attributes", "*", "*"                    }, "", deleteAttributeValueInstance                        },
  { Ngsi10SubscriptionsConvOp,                     3, { "ngsi10",  "contextSubscriptions", "*"                                       }, "", deleteSubscriptionConvOp                            },
  { IndividualContextEntity,                       3, { "v1",      "contextEntities", "*"                                            }, "", deleteIndividualContextEntity                       },
  { IndividualContextEntityAttributes,             4, { "v1",      "contextEntities", "*", "attributes"                              }, "", deleteIndividualContextEntity                       },
  { IndividualContextEntityAttribute,              5, { "v1",      "contextEntities", "*", "attributes", "*"                         }, "", deleteIndividualContextEntityAttribute              },
  { AttributeValueInstance,                        6, { "v1",      "contextEntities", "*", "attributes", "*", "*"                    }, "", deleteAttributeValueInstance                        },
  { Ngsi10SubscriptionsConvOp,                     3, { "v1",      "contextSubscriptions", "*"                                       }, "", deleteSubscriptionConvOp                            },
  { AllEntitiesWithTypeAndId,                      6, { "v1", "contextEntities", "type", "*", "id", "*"                              }, "", deleteAllEntitiesWithTypeAndId                      },
  { IndividualContextEntityAttributeWithTypeAndId, 8, { "v1", "contextEntities", "type", "*", "id", "*", "attributes", "*"           }, "", deleteIndividualContextEntityAttributeWithTypeAndId },
  { AttributeValueInstanceWithTypeAndId,           9, { "v1",      "contextEntities", "type", "*", "id", "*", "attributes", "*", "*" }, "", deleteAttributeValueInstanceWithTypeAndId           },
  { LogTraceRequest,                               2, { "log", "trace"                                                               }, "", logTraceTreat                                       },
  { LogTraceRequest,                               3, { "log", "trace",      "*"                                                     }, "", logTraceTreat                                       },
  { LogTraceRequest,                               2, { "log", "traceLevel"                                                          }, "", logTraceTreat                                       },
  { LogTraceRequest,                               3, { "log", "traceLevel", "*"                                                     }, "", logTraceTreat                                       },
  { LogTraceRequest,                               4, { "v1", "admin", "log", "trace"                                                }, "", logTraceTreat                                       },
  { LogTraceRequest,                               5, { "v1", "admin", "log", "trace",      "*"                                      }, "", logTraceTreat                                       },
  { LogTraceRequest,                               4, { "v1", "admin", "log", "traceLevel"                                           }, "", logTraceTreat                                       },
  { LogTraceRequest,                               5, { "v1", "admin", "log", "traceLevel", "*"                                      }, "", logTraceTreat                                       },
  { StatisticsRequest,                             1, { "statistics"                                                                 }, "", statisticsTreat                                     },
  { StatisticsRequest,                             3, { "v1", "admin", "statistics"                                                  }, "", statisticsTreat                                     },
  { StatisticsRequest,                             2, { "cache", "statistics"                                                        }, "", statisticsCacheTreat                                },
  { StatisticsRequest,                             4, { "v1", "admin", "cache", "statistics"                                         }, "", statisticsCacheTreat                                },
  { MetricsRequest,                                2, { "admin", "metrics"                                                           }, "", deleteMetrics                                       },

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* badVerbV - 
*/
static RestService badVerbV[] =
{
  { EntryPointsRequest,                            1, { "v2"                                                                           }, "",                                             badVerbGetOnly            },
  { EntitiesRequest,                               2, { "v2", "entities"                                                               }, "",                                             badVerbGetPostOnly        },
  { EntityRequest,                                 3, { "v2", "entities", "*"                                                          }, "",                                             badVerbGetDeleteOnly      },
  { EntityRequest,                                 4, { "v2", "entities", "*", "attrs"                                                 }, "",                                             badVerbAllNotDelete       },
  { EntityAttributeValueRequest,                   6, { "v2", "entities", "*", "attrs", "*", "value"                                   }, "",                                             badVerbGetPutOnly         },
  { EntityAttributeRequest,                        5, { "v2", "entities", "*", "attrs", "*"                                            }, "",                                             badVerbGetPutDeleteOnly   },
  { EntityTypeRequest,                             3, { "v2", "types", "*"                                                             }, "",                                             badVerbGetOnly            },
  { EntityAllTypesRequest,                         2, { "v2", "types"                                                                  }, "",                                             badVerbGetOnly            },
  { SubscriptionsRequest,                          2, { "v2", "subscriptions"                                                          }, "",                                             badVerbGetPostOnly        },
  { IndividualSubscriptionRequest,                 3, { "v2", "subscriptions", "*"                                                     }, "",                                             badVerbGetDeletePatchOnly },
  { BatchQueryRequest,                             3, { "v2", "op", "query"                                                            }, "",                                             badVerbPostOnly           },
  { BatchUpdateRequest,                            3, { "v2", "op", "update"                                                           }, "",                                             badVerbPostOnly           },
  { RegisterContext,                               2, { "ngsi9",          "registerContext"                                            }, "registerContextRequest",                       badVerbPostOnly           },
  { DiscoverContextAvailability,                   2, { "ngsi9",          "discoverContextAvailability"                                }, "discoverContextAvailabilityRequest",           badVerbPostOnly           },
  { SubscribeContextAvailability,                  2, { "ngsi9",          "subscribeContextAvailability"                               }, "subscribeContextAvailabilityRequest",          badVerbPostOnly           },
  { UnsubscribeContextAvailability,                2, { "ngsi9",          "unsubscribeContextAvailability"                             }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly           },
  { UpdateContextAvailabilitySubscription,         2, { "ngsi9",          "updateContextAvailabilitySubscription"                      }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly           },
  { NotifyContextAvailability,                     2, { "ngsi9",          "notifyContextAvailability"                                  }, "notifyContextAvailabilityRequest",             badVerbPostOnly           },
  { RegisterContext,                               3, { "v1", "registry", "registerContext"                                            }, "registerContextRequest",                       badVerbPostOnly           },
  { DiscoverContextAvailability,                   3, { "v1", "registry", "discoverContextAvailability"                                }, "discoverContextAvailabilityRequest",           badVerbPostOnly           },
  { SubscribeContextAvailability,                  3, { "v1", "registry", "subscribeContextAvailability"                               }, "subscribeContextAvailabilityRequest",          badVerbPostOnly           },
  { UnsubscribeContextAvailability,                3, { "v1", "registry", "unsubscribeContextAvailability"                             }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly           },
  { UpdateContextAvailabilitySubscription,         3, { "v1", "registry", "updateContextAvailabilitySubscription"                      }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly           },
  { NotifyContextAvailability,                     3, { "v1", "registry", "notifyContextAvailability"                                  }, "notifyContextAvailabilityRequest",             badVerbPostOnly           },
  { RegisterContext,                               2, { "ngsi9",          "registerContext"                                            }, "registerContextRequest",                       badVerbPostOnly           },
  { DiscoverContextAvailability,                   2, { "ngsi9",          "discoverContextAvailability"                                }, "discoverContextAvailabilityRequest",           badVerbPostOnly           },
  { SubscribeContextAvailability,                  2, { "ngsi9",          "subscribeContextAvailability"                               }, "subscribeContextAvailabilityRequest",          badVerbPostOnly           },
  { UnsubscribeContextAvailability,                2, { "ngsi9",          "unsubscribeContextAvailability"                             }, "unsubscribeContextAvailabilityRequest",        badVerbPostOnly           },
  { UpdateContextAvailabilitySubscription,         2, { "ngsi9",          "updateContextAvailabilitySubscription"                      }, "updateContextAvailabilitySubscriptionRequest", badVerbPostOnly           },
  { NotifyContextAvailability,                     2, { "ngsi9",          "notifyContextAvailability"                                  }, "notifyContextAvailabilityRequest",             badVerbPostOnly           },
  { UpdateContext,                                 2, { "v1",             "updateContext"                                              }, "updateContextRequest",                         badVerbPostOnly           },
  { QueryContext,                                  2, { "v1",             "queryContext"                                               }, "queryContextRequest",                          badVerbPostOnly           },
  { SubscribeContext,                              2, { "v1",             "subscribeContext"                                           }, "subscribeContextRequest",                      badVerbPostOnly           },
  { UpdateContextSubscription,                     2, { "v1",             "updateContextSubscription"                                  }, "updateContextSubscriptionRequest",             badVerbPostOnly           },
  { UnsubscribeContext,                            2, { "v1",             "unsubscribeContext"                                         }, "unsubscribeContextRequest",                    badVerbPostOnly           },
  { NotifyContext,                                 2, { "v1",             "notifyContext"                                              }, "notifyContextRequest",                         badVerbPostOnly           },
  { ContextEntitiesByEntityId,                     3, { "ngsi9",          "contextEntities", "*"                                       }, "",                                             badVerbGetPostOnly        },
  { ContextEntityAttributes,                       4, { "ngsi9",          "contextEntities", "*", "attributes"                         }, "",                                             badVerbGetPostOnly        },
  { EntityByIdAttributeByName,                     5, { "ngsi9",          "contextEntities", "*", "attributes", "*"                    }, "",                                             badVerbGetPostOnly        },
  { ContextEntityTypes,                            3, { "ngsi9",          "contextEntityTypes", "*"                                    }, "",                                             badVerbGetPostOnly        },
  { ContextEntityTypeAttributeContainer,           4, { "ngsi9",          "contextEntityTypes", "*", "attributes"                      }, "",                                             badVerbGetPostOnly        },
  { ContextEntityTypeAttribute,                    5, { "ngsi9",          "contextEntityTypes", "*", "attributes", "*"                 }, "",                                             badVerbGetPostOnly        },
  { SubscribeContextAvailability,                  2, { "ngsi9",          "contextAvailabilitySubscriptions"                           }, "",                                             badVerbPostOnly           },
  { Ngsi9SubscriptionsConvOp,                      3, { "ngsi9",          "contextAvailabilitySubscriptions", "*"                      }, "",                                             badVerbPutDeleteOnly      },
  { ContextEntitiesByEntityId,                     4, { "v1", "registry", "contextEntities", "*"                                       }, "",                                             badVerbGetPostOnly        },
  { ContextEntityAttributes,                       5, { "v1", "registry", "contextEntities", "*", "attributes"                         }, "",                                             badVerbGetPostOnly        },
  { EntityByIdAttributeByName,                     6, { "v1", "registry", "contextEntities", "*", "attributes", "*"                    }, "",                                             badVerbGetPostOnly        },
  { ContextEntityTypes,                            4, { "v1", "registry", "contextEntityTypes", "*"                                    }, "",                                             badVerbGetPostOnly        },
  { ContextEntityTypeAttributeContainer,           5, { "v1", "registry", "contextEntityTypes", "*", "attributes"                      }, "",                                             badVerbGetPostOnly        },
  { ContextEntityTypeAttribute,                    6, { "v1", "registry", "contextEntityTypes", "*", "attributes", "*"                 }, "",                                             badVerbGetPostOnly        },
  { SubscribeContextAvailability,                  3, { "v1", "registry", "contextAvailabilitySubscriptions"                           }, "",                                             badVerbPostOnly           },
  { Ngsi9SubscriptionsConvOp,                      4, { "v1", "registry", "contextAvailabilitySubscriptions", "*"                      }, "",                                             badVerbPutDeleteOnly      },
  { IndividualContextEntity,                       3, { "ngsi10",  "contextEntities", "*"                                              }, "",                                             badVerbAllFour            },
  { IndividualContextEntityAttributes,             4, { "ngsi10",  "contextEntities", "*", "attributes"                                }, "",                                             badVerbAllFour            },
  { IndividualContextEntityAttribute,              5, { "ngsi10",  "contextEntities", "*", "attributes", "*"                           }, "",                                             badVerbAllFour            },
  { AttributeValueInstance,                        6, { "ngsi10",  "contextEntities", "*", "attributes", "*", "*"                      }, "",                                             badVerbGetPutDeleteOnly   },
  { Ngsi10ContextEntityTypes,                      3, { "ngsi10",  "contextEntityTypes", "*"                                           }, "",                                             badVerbGetOnly            },
  { Ngsi10ContextEntityTypesAttributeContainer,    4, { "ngsi10",  "contextEntityTypes", "*", "attributes"                             }, "",                                             badVerbGetOnly            },
  { Ngsi10ContextEntityTypesAttribute,             5, { "ngsi10",  "contextEntityTypes", "*", "attributes", "*"                        }, "",                                             badVerbGetOnly            },
  { SubscribeContext,                              2, { "ngsi10",  "contextSubscriptions"                                              }, "",                                             badVerbPostOnly           },
  { Ngsi10SubscriptionsConvOp,                     3, { "ngsi10",  "contextSubscriptions", "*"                                         }, "",                                             badVerbPutDeleteOnly      },
  { IndividualContextEntity,                       3, { "v1",      "contextEntities", "*"                                              }, "",                                             badVerbAllFour            },
  { IndividualContextEntityAttributes,             4, { "v1",      "contextEntities", "*", "attributes"                                }, "",                                             badVerbAllFour            },
  { IndividualContextEntityAttribute,              5, { "v1",      "contextEntities", "*", "attributes", "*"                           }, "",                                             badVerbAllFour            },
  { AttributeValueInstance,                        6, { "v1",      "contextEntities", "*", "attributes", "*", "*"                      }, "",                                             badVerbGetPutDeleteOnly   },
  { Ngsi10ContextEntityTypes,                      3, { "v1",      "contextEntityTypes", "*"                                           }, "",                                             badVerbGetOnly            },
  { Ngsi10ContextEntityTypesAttributeContainer,    4, { "v1",      "contextEntityTypes", "*", "attributes"                             }, "",                                             badVerbGetOnly            },
  { Ngsi10ContextEntityTypesAttribute,             5, { "v1",      "contextEntityTypes", "*", "attributes", "*"                        }, "",                                             badVerbGetOnly            },
  { SubscribeContext,                              2, { "v1",      "contextSubscriptions"                                              }, "",                                             badVerbPostOnly           },
  { Ngsi10SubscriptionsConvOp,                     3, { "v1",      "contextSubscriptions", "*"                                         }, "",                                             badVerbPutDeleteOnly      },
  { EntityTypes,                                   2, { "v1", "contextTypes"                                                           }, "",                                             badVerbGetOnly            },
  { AttributesForEntityType,                       3, { "v1", "contextTypes", "*"                                                      }, "",                                             badVerbGetOnly            },
  { AllContextEntities,                            2, { "v1", "contextEntities"                                                        }, "",                                             badVerbGetPostOnly        },
  { AllEntitiesWithTypeAndId,                      6, { "v1", "contextEntities", "type", "*", "id", "*"                                }, "",                                             badVerbAllFour            },
  { IndividualContextEntityAttributeWithTypeAndId, 8, { "v1", "contextEntities", "type", "*", "id", "*", "attributes", "*"             }, "",                                             badVerbAllFour            },
  { AttributeValueInstanceWithTypeAndId,           9, { "v1",      "contextEntities", "type", "*", "id", "*", "attributes", "*", "*"   }, "",                                             badVerbAllFour            },
  { ContextEntitiesByEntityIdAndType,              7, { "v1", "registry", "contextEntities", "type", "*", "id", "*"                    }, "",                                             badVerbGetPostOnly        },
  { EntityByIdAttributeByNameIdAndType,            9, { "v1", "registry", "contextEntities", "type", "*", "id", "*", "attributes", "*" }, "",                                             badVerbGetPostOnly        },
  { LogTraceRequest,                               2, { "log", "trace"                                                                 }, "",                                             badVerbGetDeleteOnly      },
  { LogTraceRequest,                               3, { "log", "trace",      "*"                                                       }, "",                                             badVerbPutDeleteOnly      },
  { LogTraceRequest,                               2, { "log", "traceLevel"                                                            }, "",                                             badVerbGetDeleteOnly      },
  { LogTraceRequest,                               3, { "log", "traceLevel", "*"                                                       }, "",                                             badVerbPutDeleteOnly      },
  { LogTraceRequest,                               4, { "v1", "admin", "log", "trace"                                                  }, "",                                             badVerbGetDeleteOnly      },
  { LogTraceRequest,                               5, { "v1", "admin", "log", "trace",      "*"                                        }, "",                                             badVerbPutDeleteOnly      },
  { LogTraceRequest,                               4, { "v1", "admin", "log", "traceLevel"                                             }, "",                                             badVerbGetDeleteOnly      },
  { LogTraceRequest,                               5, { "v1", "admin", "log", "traceLevel", "*"                                        }, "",                                             badVerbPutDeleteOnly      },
  { StatisticsRequest,                             1, { "statistics"                                                                   }, "",                                             badVerbGetDeleteOnly      },
  { StatisticsRequest,                             3, { "v1", "admin", "statistics"                                                    }, "",                                             badVerbGetDeleteOnly      },
  { StatisticsRequest,                             2, { "cache", "statistics"                                                          }, "",                                             badVerbGetDeleteOnly      },
  { StatisticsRequest,                             4, { "v1", "admin", "cache", "statistics"                                           }, "",                                             badVerbGetDeleteOnly      },
  { VersionRequest,                                1, { "version"                                                                      }, "",                                             badVerbGetOnly            },
  { LogLevelRequest,                               2, { "admin", "log"                                                                 }, "",                                             badVerbPutOnly            },
  { SemStateRequest,                               2, { "admin", "sem"                                                                 }, "",                                             badVerbGetOnly            },
  { MetricsRequest,                                2, { "admin", "metrics"                                                             }, "",                                             badVerbGetDeleteOnly      },
  { UpdateContext,                                 2, { "ngsi10",  "updateContext"                                                     }, "updateContextRequest",                         badVerbPostOnly           },
  { QueryContext,                                  2, { "ngsi10",  "queryContext"                                                      }, "queryContextRequest",                          badVerbPostOnly           },
  { SubscribeContext,                              2, { "ngsi10",  "subscribeContext"                                                  }, "subscribeContextRequest",                      badVerbPostOnly           },
  { UpdateContextSubscription,                     2, { "ngsi10",  "updateContextSubscription"                                         }, "updateContextSubscriptionRequest",             badVerbPostOnly           },
  { UnsubscribeContext,                            2, { "ngsi10",  "unsubscribeContext"                                                }, "unsubscribeContextRequest",                    badVerbPostOnly           },
  { NotifyContext,                                 2, { "ngsi10",  "notifyContext"                                                     }, "notifyContextRequest",                         badVerbPostOnly           },
  { InvalidRequest,                                2, { "ngsi9",   "*"                                                                 }, "",                                             badNgsi9Request           },
  { InvalidRequest,                                2, { "ngsi10",  "*"                                                                 }, "",                                             badNgsi10Request          },
  { InvalidRequest,                                0, { "*", "*", "*", "*", "*", "*"                                                   }, "",                                             badRequest                },
  { InvalidRequest,                                0, {                                                                                }, "",                                             NULL                      },

  ORION_REST_SERVICE_END
};



/* ****************************************************************************
*
* optionsV - 
*
*/
static RestService optionsV[] =
{
  { EntryPointsRequest,            1, { "v2"                                         }, "", optionsGetOnly            },
  { EntitiesRequest,               2, { "v2", "entities"                             }, "", optionsGetPostOnly        },
  { EntityRequest,                 3, { "v2", "entities", "*"                        }, "", optionsGetDeleteOnly      },
  { EntityRequest,                 4, { "v2", "entities", "*", "attrs"               }, "", optionsAllNotDelete       },
  { EntityAttributeValueRequest,   6, { "v2", "entities", "*", "attrs", "*", "value" }, "", optionsGetPutOnly         },
  { EntityAttributeRequest,        5, { "v2", "entities", "*", "attrs", "*"          }, "", optionsGetPutDeleteOnly   },
  { EntityTypeRequest,             3, { "v2", "types", "*"                           }, "", optionsGetOnly            },
  { EntityAllTypesRequest,         2, { "v2", "types"                                }, "", optionsGetOnly            },
  { SubscriptionsRequest,          2, { "v2", "subscriptions"                        }, "", optionsGetPostOnly        },
  { IndividualSubscriptionRequest, 3, { "v2", "subscriptions", "*"                   }, "", optionsGetDeletePatchOnly },
  { BatchQueryRequest,             3, { "v2", "op", "query"                          }, "", optionsPostOnly           },
  { BatchUpdateRequest,            3, { "v2", "op", "update"                         }, "", optionsPostOnly           },
  { VersionRequest,                1, { "version"                                    }, "", optionsVersionRequest     },

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
   const std::string&  rushHost,
   unsigned short      rushPort,
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
           rushHost,
           rushPort,
           allowedOrigin,
           corsMaxAge,
           mhdTimeoutInSeconds,
           httpsKey,
           httpsCert);
}
