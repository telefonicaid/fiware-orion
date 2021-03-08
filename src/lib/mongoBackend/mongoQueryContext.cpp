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
* Author: Fermin Galan Marquez
*/
#include <string>
#include <vector>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/string.h"
#include "common/sem.h"
#include "alarmMgr/alarmMgr.h"
#include "ngsi/ContextRegistrationResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"



/* ****************************************************************************
*
* someContextElementNotFound -
*
* Returns true if some attribute with 'found' set to 'false' is found in the CER vector passed
* as argument
*/
static bool someContextElementNotFound(const ContextElementResponseVector& cerV)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if (someContextElementNotFound(*cerV[ix]))
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* fillContextProviders -
*
* Looks in the elements of the CER vector passed as argument, searching for a suitable CPr in the CRR
* vector passed as argument. If a suitable CPr is found, it is added to the CER (and the 'found' field
* is changed to true)
*/
static void fillContextProviders(ContextElementResponseVector& cerV, const ContextRegistrationResponseVector& crrV)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    fillContextProviders(cerV[ix], crrV);
  }
}



/* ****************************************************************************
*
* addContextProviderEntity -
*/
static void addContextProviderEntity
(
  ContextElementResponseVector&  cerV,
  EntityId*                      enP,
  const ProvidingApplication&    pa
)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if ((cerV[ix]->entity.id == enP->id) && (cerV[ix]->entity.type == enP->type))
    {
      cerV[ix]->entity.providingApplicationList.push_back(pa);
      return;    /* by construction, no more than one CER with the same entity information should exist in the CERV) */
    }
  }

  /* Reached this point, it means that the cerV doesn't contain a proper CER, so we create it */
  ContextElementResponse* cerP = new ContextElementResponse();

  cerP->entity.fill(enP->id, enP->type, enP->isPattern);
  cerP->entity.providingApplicationList.push_back(pa);

  cerP->statusCode.fill(SccOk);
  cerV.push_back(cerP);
}



/* ****************************************************************************
*
* addContextProviderAttribute -
*
* The 'limitReached' parameter is to prevent the addition of new entities, which is needed in case
* the pagination limit has been reached with local entities.
*/
static void addContextProviderAttribute
(
  ContextElementResponseVector&   cerV,
  EntityId*                       enP,
  ContextRegistrationAttribute*   craP,
  const ProvidingApplication&     pa,
  bool                            limitReached
)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if ((cerV[ix]->entity.id != enP->id) || (cerV[ix]->entity.type != enP->type))
    {
     continue;
    }

    for (unsigned int jx = 0; jx < cerV[ix]->entity.attributeVector.size(); ++jx)
    {
      std::string attrName = cerV[ix]->entity.attributeVector[jx]->name;

      if (attrName == craP->name)
      {
        /* In this case, the attribute has been already found in local database. CPr is unnecessary */

        // FIXME PR: This breaks the test - the providerFormat is never set and the default value (currently V1) is still valid
        return;
      }
    }

    /* Reached this point, no attribute was found, so adding it with corresponding CPr info */
    ContextAttribute* caP = new ContextAttribute(craP->name, "", "");

    caP->providingApplication = pa;
    cerV[ix]->entity.attributeVector.push_back(caP);
    return;
  }

  if (!limitReached)
  {
    /* Reached this point, it means that the cerV doesn't contain a proper CER, so we create it */
    ContextElementResponse* cerP            = new ContextElementResponse();

    cerP->entity.fill(enP->id, enP->type, enP->isPattern);
    cerP->statusCode.fill(SccOk);

    ContextAttribute* caP = new ContextAttribute(craP->name, "", "");

    caP->providingApplication = pa;
    cerP->entity.attributeVector.push_back(caP);
    cerV.push_back(cerP);
  }
}



/* ****************************************************************************
*
* matchEntityInCrr -
*/
static bool matchEntityInCrr(const ContextRegistration& cr, const EntityId* enP)
{
  for (unsigned int ix = 0; ix < cr.entityIdVector.size(); ++ix)
  {
    EntityId* crEnP = cr.entityIdVector[ix];

    if (matchEntity(crEnP, enP))
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* addContextProviders -
*
* This function takes a CRR vector and adds the Context Providers in the CER vector
* (except the ones corresponding to some locally found attribute, i.e. info already in the
* CER vector)
*
* The limitReached parameter is to prevent the addition of new entities, which is needed in the case of the pagination
* limit has been reached with local entities.
*
* The enP parameter is optional. If not NULL, then before adding a CPr the function checks that the
* containting CRR matches the entity (this is used for funcionality related to  "generic queries", see
* processGenericEntities() function).
*/
static void addContextProviders
(
  ContextElementResponseVector&       cerV,
  ContextRegistrationResponseVector&  crrV,
  bool                                limitReached,
  const EntityId*                     enP = NULL
)
{
  for (unsigned int ix = 0; ix < crrV.size(); ++ix)
  {
    ContextRegistration cr = crrV[ix]->contextRegistration;
    cr.providingApplication.setRegId(crrV[ix]->regId);

    /* In case a "filtering" entity was provided, check that the current CRR matches or skip to next CRR */
    if (enP != NULL && !matchEntityInCrr(cr, enP))
    {
      continue;
    }

    if (cr.contextRegistrationAttributeVector.size() == 0)
    {
      if (!limitReached)
      {
        /* Registration without attributes */
        for (unsigned int eIx = 0; eIx < cr.entityIdVector.size(); ++eIx)
        {
          addContextProviderEntity(cerV, cr.entityIdVector[eIx], cr.providingApplication);
        }
      }
    }
    else
    {
      /* Registration with attributes */
      for (unsigned int eIx = 0; eIx < cr.entityIdVector.size(); ++eIx)
      {
        for (unsigned int aIx = 0; aIx < cr.contextRegistrationAttributeVector.size(); ++aIx)
        {
          addContextProviderAttribute(cerV,
                                      cr.entityIdVector[eIx],
                                      cr.contextRegistrationAttributeVector[aIx],
                                      cr.providingApplication,
                                      limitReached);
        }
      }
    }
  }
}



/* ****************************************************************************
*
* processGenericEntities -
*
* If the request included some "generic" entity, some additional CPr could be needed in the CER array. There are
* three cases of "generic" entities: 1) not pattern + null type, 2) pattern + not null type, 3) pattern + null type
*
* The limitReached parameter is to prevent the addition of new entities, which is needed in the case of the pagination
* limit has been reached with local entities.
*/
static void processGenericEntities
(
  const EntityIdVector&               enV,
  ContextElementResponseVector&       cerV,
  ContextRegistrationResponseVector&  crrV,
  bool                                limitReached
)
{
  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    const EntityId* enP = enV[ix];
    if (enP->type.empty() || isTrue(enP->isPattern))
    {
      addContextProviders(cerV, crrV, limitReached, enP);
    }
  }
}



/* ****************************************************************************
*
* mongoQueryContext -
*
* NOTE
*   If the in/out-parameter countP is non-NULL then the number of matching entities
*   must be returned in *countP.
*
*   This replaces the 'uriParams[URI_PARAM_PAGINATION_DETAILS]' way of passing this information.
*   The old method was one-way, using the new method
*/
HttpStatusCode mongoQueryContext
(
  QueryContextRequest*                 requestP,
  QueryContextResponse*                responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,
  std::map<std::string, bool>&         options,
  long long*                           countP,
  ApiVersion                           apiVersion
)
{
  int         offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  int         limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());

  std::string sortOrderList  = uriParams[URI_PARAM_SORTED];

  LM_T(LmtMongo, ("QueryContext Request"));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Count: %s", offset, limit, (countP != NULL)? "true" : "false"));

  /* FIXME: restriction not supported for the moment */
  if (!requestP->restriction.attributeExpression.isEmpty())
  {
    alarmMgr.badInput(clientIp, "restriction found, but restrictions are not supported by mongo backend");
  }

  std::string                  err;
  bool                         ok;
  bool                         limitReached = false;
  bool                         reqSemTaken;
  ContextElementResponseVector rawCerV;

  reqSemTake(__FUNCTION__, "ngsi10 query request", SemReadOp, &reqSemTaken);

  ok = entitiesQuery(requestP->entityIdVector,
                     requestP->attributeList,
                     requestP->restriction,
                     &rawCerV,
                     &err,
                     true,
                     tenant,
                     servicePathV,
                     offset,
                     limit,
                     &limitReached,
                     countP,
                     sortOrderList,
                     apiVersion);

  if (!ok)
  {
    responseP->errorCode.fill(SccReceiverInternalError, err);
    rawCerV.release();
    reqSemGive(__FUNCTION__, "ngsi10 query request", reqSemTaken);

    return SccOk;
  }

  ContextRegistrationResponseVector crrV;

  /* In the case of empty response, if only generic processing is needed */
  if (rawCerV.size() == 0)
  {
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, ngsiv2::ForwardQuery, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      if (crrV.size() > 0)
      {
        processGenericEntities(requestP->entityIdVector, rawCerV, crrV, limitReached);
      }
    }

    crrV.release();
  }

  /* First CPr lookup (in the case some CER is not found): looking in E-A registrations */
  if (someContextElementNotFound(rawCerV))
  {
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, ngsiv2::ForwardQuery, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      if (crrV.size() > 0)
      {
        fillContextProviders(rawCerV, crrV);
        processGenericEntities(requestP->entityIdVector, rawCerV, crrV, limitReached);
      }
    }

    crrV.release();
  }

  /* Second CPr lookup (in the case some elements still not being found): looking in E-<null> registrations */
  StringList attrNullList;

  if (someContextElementNotFound(rawCerV))
  {
    if (registrationsQuery(requestP->entityIdVector, attrNullList, ngsiv2::ForwardQuery, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      if (crrV.size() > 0)
      {
        fillContextProviders(rawCerV, crrV);
      }
    }

    crrV.release();
  }

  /* Special case: request with <null> attributes. In that case, entitiesQuery() may have captured some local attribute, but
   * the list needs to be completed. Note that in the case of having this request someContextElementNotFound() is always false
   * so we efficient not invoking registrationQuery() too much times
   */
  if (requestP->attributeList.size() == 0)
  {
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, ngsiv2::ForwardQuery, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      if (crrV.size() > 0)
      {
        addContextProviders(rawCerV, crrV, limitReached);
      }
    }

    crrV.release();
  }

  /* Prune "not found" CERs */
  pruneContextElements(apiVersion, requestP->attrsList, rawCerV, &responseP->contextElementResponseVector);

  /* Pagination stuff */
  if (responseP->contextElementResponseVector.size() == 0)
  {
    //
    // If the query has an empty response, we have to fill in the status code part in the response.
    //
    // However, if the response was empty due to a too high pagination offset,
    // and if the user has asked for 'details' (as URI parameter, then the response should include information about
    // the number of hits without pagination.
    //

    if ((countP != NULL) && (*countP > 0) && (offset >= *countP))
    {
      char details[256];

      snprintf(details, sizeof(details), "Number of matching entities: %lld. Offset is %d", *countP, offset);
      responseP->errorCode.fill(SccContextElementNotFound, details);
    }
    else
    {
      responseP->errorCode.fill(SccContextElementNotFound);
    }
  }
  else if (countP != NULL)
  {
    //
    // If all was OK, but the details URI param was set to 'on', then the responses error code details
    // 'must' contain the total count of hits.
    //

    char details[64];

    snprintf(details, sizeof(details), "Count: %lld", *countP);
    responseP->errorCode.fill(SccOk, details);
  }

  rawCerV.release();

  reqSemGive(__FUNCTION__, "ngsi10 query request", reqSemTaken);
  return SccOk;
}
