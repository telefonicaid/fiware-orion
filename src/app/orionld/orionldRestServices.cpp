/*
*
* Copyright 2018 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include "orionld/serviceRoutines/orionldPostEntities.h"
#include "orionld/serviceRoutines/orionldPostEntity.h"
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"
#include "orionld/serviceRoutines/orionldPostRegistrations.h"
#include "orionld/serviceRoutines/orionldGetEntity.h"
#include "orionld/serviceRoutines/orionldGetEntities.h"
#include "orionld/serviceRoutines/orionldGetEntityTypes.h"
#include "orionld/serviceRoutines/orionldPostBatchDeleteEntities.h"
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"
#include "orionld/serviceRoutines/orionldGetSubscription.h"
#include "orionld/serviceRoutines/orionldGetRegistrations.h"
#include "orionld/serviceRoutines/orionldGetRegistration.h"
#include "orionld/serviceRoutines/orionldPatchEntity.h"
#include "orionld/serviceRoutines/orionldPatchAttribute.h"
#include "orionld/serviceRoutines/orionldPatchSubscription.h"
#include "orionld/serviceRoutines/orionldPatchRegistration.h"
#include "orionld/serviceRoutines/orionldDeleteEntity.h"
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"
#include "orionld/serviceRoutines/orionldDeleteSubscription.h"
#include "orionld/serviceRoutines/orionldDeleteRegistration.h"
#include "orionld/serviceRoutines/orionldGetContext.h"
#include "orionld/serviceRoutines/orionldGetContexts.h"
#include "orionld/serviceRoutines/orionldGetVersion.h"
#include "orionld/serviceRoutines/orionldNotImplemented.h"
#include "orionld/serviceRoutines/orionldPostBatchUpsert.h"
#include "orionld/serviceRoutines/orionldPostBatchCreate.h"
#include "orionld/serviceRoutines/orionldPostBatchUpdate.h"
#include "orionld/serviceRoutines/orionldGetEntityTypes.h"
#include "orionld/serviceRoutines/orionldGetTenants.h"
#include "orionld/serviceRoutines/orionldGetDbIndexes.h"
#include "orionld/serviceRoutines/orionldPostQuery.h"

#include "orionld/rest/OrionLdRestService.h"       // OrionLdRestServiceSimplified
#include "orionld/orionldRestServices.h"           // Own Interface



// -----------------------------------------------------------------------------
//
// getServiceV -
//
static OrionLdRestServiceSimplified getServiceV[] =
{
  { "/ngsi-ld/v1/entities/*",              orionldGetEntity          },
  { "/ngsi-ld/v1/entities",                orionldGetEntities        },
  { "/ngsi-ld/v1/types",                   orionldGetEntityTypes     },
  { "/ngsi-ld/v1/subscriptions/*",         orionldGetSubscription    },
  { "/ngsi-ld/v1/subscriptions",           orionldGetSubscriptions   },
  { "/ngsi-ld/v1/csourceRegistrations/*",  orionldGetRegistration    },
  { "/ngsi-ld/v1/csourceRegistrations",    orionldGetRegistrations   },
  { "/ngsi-ld/ex/v1/contexts/*",           orionldGetContext         },
  { "/ngsi-ld/ex/v1/contexts",             orionldGetContexts        },
  { "/ngsi-ld/ex/v1/version",              orionldGetVersion         },
  { "/ngsi-ld/ex/v1/tenants",              orionldGetTenants         },
  { "/ngsi-ld/ex/v1/dbIndexes",            orionldGetDbIndexes       },
  { "/ngsi-ld/v1/temporal/entities",       orionldNotImplemented     },
  { "/ngsi-ld/v1/temporal/entities/*",     orionldNotImplemented     }
};
static const int getServices = (sizeof(getServiceV) / sizeof(getServiceV[0]));



// ----------------------------------------------------------------------------
//
// postServiceV -
//
static OrionLdRestServiceSimplified postServiceV[] =
{
  { "/ngsi-ld/v1/entities/*/attrs",        orionldPostEntity                     },
  { "/ngsi-ld/v1/entities",                orionldPostEntities                   },
  { "/ngsi-ld/v1/entityOperations/create", orionldPostBatchCreate                },
  { "/ngsi-ld/v1/entityOperations/upsert", orionldPostBatchUpsert                },
  { "/ngsi-ld/v1/entityOperations/update", orionldPostBatchUpdate                },
  { "/ngsi-ld/v1/entityOperations/delete", orionldPostBatchDeleteEntities        },
  { "/ngsi-ld/v1/entityOperations/query",  orionldPostQuery                      },
  { "/ngsi-ld/v1/subscriptions",           orionldPostSubscriptions              },
  { "/ngsi-ld/v1/csourceRegistrations",    orionldPostRegistrations              },
  { "/ngsi-ld/v1/temporal/entities",       orionldNotImplemented                 },
  { "/ngsi-ld/v1/temporal/entities/*",     orionldNotImplemented                 }
};
static const int postServices = (sizeof(postServiceV) / sizeof(postServiceV[0]));



// ----------------------------------------------------------------------------
//
// patchServiceV -
//
static OrionLdRestServiceSimplified patchServiceV[] =
{
  { "/ngsi-ld/v1/entities/*/attrs/*",      orionldPatchAttribute     },
  { "/ngsi-ld/v1/entities/*/attrs",        orionldPatchEntity        },
  { "/ngsi-ld/v1/subscriptions/*",         orionldPatchSubscription  },
  { "/ngsi-ld/v1/csourceRegistrations/*",  orionldPatchRegistration  }
};
static const int patchServices = (sizeof(patchServiceV) / sizeof(patchServiceV[0]));



// ----------------------------------------------------------------------------
//
// deleteServiceV -
//
static OrionLdRestServiceSimplified deleteServiceV[] =
{
  { "/ngsi-ld/v1/entities/*/attrs/*",      orionldDeleteAttribute    },  // Very important that orionldDeleteAttribute comes before orionldDeleteEntity
  { "/ngsi-ld/v1/entities/*",              orionldDeleteEntity       },
  { "/ngsi-ld/v1/subscriptions/*",         orionldDeleteSubscription },
  { "/ngsi-ld/v1/csourceRegistrations/*",  orionldDeleteRegistration }
};
static const int deleteServices = (sizeof(deleteServiceV) / sizeof(deleteServiceV[0]));



// -----------------------------------------------------------------------------
//
// restServiceVV -
//
OrionLdRestServiceSimplifiedVector restServiceVV[] =
{
  { getServiceV,    getServices     },
  { NULL,           0               },
  { postServiceV,   postServices    },
  { deleteServiceV, deleteServices  },
  { patchServiceV,  patchServices   },
  { NULL,           0               },
  { NULL,           0               },
  { NULL,           0               },
  { NULL,           0               }
};
