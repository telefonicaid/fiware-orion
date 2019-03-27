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
  const ProvidingApplication&    pa,
  const std::string&             forwardingMode
)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if ((cerV[ix]->entity.id == enP->id) && (cerV[ix]->entity.type == enP->type))
    {
      cerV[ix]->entity.providingApplicationList.push_back(pa);
      cerV[ix]->entity.forwardingMode = forwardingMode;
      return;    /* by construction, no more than one CER with the same entity information should exist in the CERV) */
    }
  }

  /* Reached this point, it means that the cerV doesn't contain a proper CER, so we create it */
  ContextElementResponse* cerP = new ContextElementResponse();

  cerP->entity.fill(enP->id, enP->type, "false");
  cerP->entity.providingApplicationList.push_back(pa);
  cerP->entity.forwardingMode = forwardingMode;

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
  bool                            limitReached,
  const std::string&              forwardingMode
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
        return;
      }
    }

    /* Reached this point, no attribute was found, so adding it with corresponding CPr info */
    ContextAttribute* caP = new ContextAttribute(craP->name, "", "");

    caP->providingApplication = pa;
    cerV[ix]->entity.attributeVector.push_back(caP);
    cerV[ix]->entity.forwardingMode = forwardingMode;
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
    cerP->entity.forwardingMode = forwardingMode;

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

    LM_T(LmtForward, ("cr %d. forwardingMode: '%s'", ix, crrV[ix]->forwardingMode.c_str()));
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
          LM_T(LmtForward, ("Calling addContextProviderEntity. forwardingMode == %s", crrV[ix]->forwardingMode.c_str()));
          addContextProviderEntity(cerV, cr.entityIdVector[eIx], cr.providingApplication, crrV[ix]->forwardingMode);
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
          LM_T(LmtForward, ("Calling addContextProviderAttribute. forwardingMode == %s", crrV[ix]->forwardingMode.c_str()));
          addContextProviderAttribute(cerV,
                                      cr.entityIdVector[eIx],
                                      cr.contextRegistrationAttributeVector[aIx],
                                      cr.providingApplication,
                                      limitReached,
                                      crrV[ix]->forwardingMode);
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
    if (enP->type == "" || isTrue(enP->isPattern))
    {
      addContextProviders(cerV, crrV, limitReached, enP);
    }
  }
}



/* ****************************************************************************
*
* cerVectorPresent -
*
* FIXME: temporal debugging routine to be removed once forwarding works 100% ok
*/
void cerVectorPresent(const char* what, const ContextElementResponseVector& cerV)
{
  LM_T(LmtForward, ("%s: got a CER vector of %d items", what, cerV.size()));
  LM_T(LmtForward, ("-------------------------------------------------------------"));

  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    Entity* eP = &cerV[ix]->entity;

    LM_T(LmtForward, ("CER %d:", ix));
    LM_T(LmtForward, ("  id: %s, type: %s", eP->id.c_str(), eP->type.c_str()));

    LM_T(LmtForward, ("  %d attributes:", eP->attributeVector.size()));
    for (unsigned int aIx = 0; aIx < eP->attributeVector.size(); aIx++)
    {
      ContextAttribute* aP = eP->attributeVector[aIx];

      LM_T(LmtForward, ("      o %s", aP->name.c_str()));
    }
    LM_T(LmtForward, ("============================================================================"));
  }
}



/* ****************************************************************************
*
* crrVectorPresent -
*
* FIXME: temporal debugging routine to be removed once forwarding works 100% ok
*/
void crrVectorPresent(const char* what, const ContextRegistrationResponseVector& crrV)
{
  LM_T(LmtForward, ("%s: got a CRR vector of %d items", what, crrV.size()));
  LM_T(LmtForward, ("-------------------------------------------------------------"));
  for (unsigned int ix = 0; ix < crrV.size(); ++ix)
  {
    ContextRegistration* crP = &crrV[ix]->contextRegistration;

    LM_T(LmtForward, ("For providingApplication %s:", crP->providingApplication.string.c_str()));
    LM_T(LmtForward, ("Forwarding Mode:         %s", crrV[ix]->forwardingMode.c_str()));
    LM_T(LmtForward, ("  - %d entities:", crP->entityIdVector.size()));
    for (unsigned int eIx = 0; eIx < crP->entityIdVector.size(); ++eIx)
    {
      EntityId* eP = crP->entityIdVector[eIx];

      LM_T(LmtForward, ("      o %s (type: '%s')", eP->id.c_str(), eP->type.c_str()));
    }

    LM_T(LmtForward, (" - - - - - -"));
    LM_T(LmtForward, ("  - %d attributes:", crP->contextRegistrationAttributeVector.size()));
    for (unsigned int aIx = 0; aIx < crP->contextRegistrationAttributeVector.size(); ++aIx)
    {
      ContextRegistrationAttribute* aP = crP->contextRegistrationAttributeVector[aIx];

      LM_T(LmtForward, ("      o %s", aP->name.c_str()));
    }
    LM_T(LmtForward, ("============================================================================"));
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

  LM_T(LmtForward, ("%d items in qcrsP->contextElementResponseVector", responseP->contextElementResponseVector.size()));
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

  LM_T(LmtForward, ("%d items in qcrsP->contextElementResponseVector", responseP->contextElementResponseVector.size()));
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

  LM_T(LmtForward, ("After entitiesQuery: %d items in qcrsP->contextElementResponseVector, %d items in rawCerV",
                    responseP->contextElementResponseVector.size(),
                    rawCerV.size()));
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
    LM_T(LmtForward, ("Calling registrationsQuery I"));
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      // FIXME: call to crrVectorPresent to be removed once forwarding works
      // crrVectorPresent("registrationsQuery I", crrV);
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
    LM_T(LmtForward, ("Calling registrationsQuery II"));
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      // FIXME: call to crrVectorPresent to be removed once forwarding works
      // crrVectorPresent("registrationsQuery II", crrV);
      if (crrV.size() > 0)
      {
        fillContextProviders(rawCerV, crrV);
        processGenericEntities(requestP->entityIdVector, rawCerV, crrV, limitReached);
      }
    }

    crrV.release();
  }

  LM_T(LmtForward, ("%d items in qcrsP->contextElementResponseVector", responseP->contextElementResponseVector.size()));
  /* Second CPr lookup (in the case some elements still not being found): looking in E-<null> registrations */
  StringList attrNullList;

  if (someContextElementNotFound(rawCerV))
  {
    LM_T(LmtForward, ("Calling registrationsQuery III"));
    if (registrationsQuery(requestP->entityIdVector, attrNullList, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      // FIXME: call to crrVectorPresent to be removed once forwarding works
      // crrVectorPresent("registrationsQuery III", crrV);
      if (crrV.size() > 0)
      {
        fillContextProviders(rawCerV, crrV);
      }
    }

    crrV.release();
  }

  LM_T(LmtForward, ("%d items in qcrsP->contextElementResponseVector", responseP->contextElementResponseVector.size()));
  /* Special case: request with <null> attributes. In that case, entitiesQuery() may have captured some local attribute, but
   * the list needs to be completed. Note that in the case of having this request someContextElementNotFound() is always false
   * so we efficient not invoking registrationQuery() too much times
   */
  if (requestP->attributeList.size() == 0)
  {
    LM_T(LmtForward, ("Calling registrationsQuery IV"));
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, &crrV, &err, tenant, servicePathV, 0, 0, false))
    {
      // FIXME: call to crrVectorPresent to be removed once forwarding works
      // crrVectorPresent("registrationsQuery IV", crrV);
      if (crrV.size() > 0)
      {
        addContextProviders(rawCerV, crrV, limitReached);
      }
    }

    crrV.release();
  }

  /* Prune "not found" CERs */
  LM_T(LmtForward, ("Before pruning, we have %d elements in rawCerV", rawCerV.size()));
  LM_T(LmtForward, ("Before pruning, we have %d items in qcrsP->contextElementResponseVector", responseP->contextElementResponseVector.size()));
  // FIXME: call to cerVectorPresent to be removed once forwarding works
  // cerVectorPresent("Before pruning", rawCerV);

  pruneContextElements(rawCerV, &responseP->contextElementResponseVector);

  LM_T(LmtForward, ("After pruning, we have %d elements in rawCerV", rawCerV.size()));
  LM_T(LmtForward, ("After pruning, we have %d items in qcrsP->contextElementResponseVector", responseP->contextElementResponseVector.size()));
  // FIXME: call to cerVectorPresent to be removed once forwarding works
  // cerVectorPresent("After pruning", rawCerV);

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

  LM_T(LmtForward, ("%d items in qcrsP->contextElementResponseVector", responseP->contextElementResponseVector.size()));
  rawCerV.release();

  reqSemGive(__FUNCTION__, "ngsi10 query request", reqSemTaken);
  return SccOk;
}
