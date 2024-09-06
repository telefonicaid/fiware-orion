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
* Looks in the elements of the CER vector passed as argument, searching for a suitable CPr in the Registrations
* vector passed as argument. If a suitable CPr is found, it is added to the CER (and the 'found' field
* is changed to true)
*/
static void fillContextProviders(ContextElementResponseVector& cerV, const std::vector<ngsiv2::Registration>& regV)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    fillContextProviders(cerV[ix], regV);
  }
}




/* ****************************************************************************
*
* lookupProvider -
*/
static bool lookupProvider(const std::vector<ngsiv2::Provider>& providerV, const ngsiv2::Provider &provider)
{
  for (unsigned int ix = 0; ix < providerV.size(); ++ix)
  {
    if (providerV[ix].http.url == provider.http.url)
    {
      return true;
    }
  }
  return false;
}



/* ****************************************************************************
*
* addContextProviderEntity -
*/
static void addContextProviderEntity
(
  ContextElementResponseVector&  cerV,
  const EntityId&                regEn,
  const ngsiv2::Provider&        provider,
  const std::string&             regId
)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if (cerV[ix]->entity.entityId == regEn)
    {
      // Avoid duplicate Provider in the vector
      if (!lookupProvider(cerV[ix]->entity.providerList, provider))
      {
        cerV[ix]->entity.providerList.push_back(provider);
        cerV[ix]->entity.providerRegIdList.push_back(regId);
      }
      return;    /* by construction, no more than one CER with the same entity information should exist in the CERV) */
    }
  }

  /* Reached this point, it means that the cerV doesn't contain a proper CER, so we create it */
  ContextElementResponse* cerP = new ContextElementResponse();

  EntityId enId(regEn.id, regEn.idPattern, regEn.type, regEn.typePattern);
  cerP->entity.fill(enId);
  cerP->entity.providerList.push_back(provider);
  cerP->entity.providerRegIdList.push_back(regId);

  cerP->error.fill(SccOk);
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
  EntityId                        regEn,
  const std::string&              regAttr,
  const ngsiv2::Provider&         provider,
  const std::string&              regId,
  bool                            limitReached
)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if ((cerV[ix]->entity.entityId.id != regEn.id) || (cerV[ix]->entity.entityId.type != regEn.type))
    {
     continue;
    }

    for (unsigned int jx = 0; jx < cerV[ix]->entity.attributeVector.size(); ++jx)
    {
      std::string attrName = cerV[ix]->entity.attributeVector[jx]->name;

      if (attrName == regAttr)
      {
        /* In this case, the attribute has been already found in local database. CPr is unnecessary */

        // FIXME PR: This breaks the test - the providerFormat is never set and the default value (currently V1) is still valid
        return;
      }
    }

    /* Reached this point, no attribute was found, so adding it with corresponding CPr info */
    ContextAttribute* caP = new ContextAttribute(regAttr, "", "");

    caP->provider = provider;
    caP->providerRegId = regId;
    cerV[ix]->entity.attributeVector.push_back(caP);
    return;
  }

  if (!limitReached)
  {
    /* Reached this point, it means that the cerV doesn't contain a proper CER, so we create it */
    ContextElementResponse* cerP            = new ContextElementResponse();

    EntityId enId(regEn.id, regEn.idPattern, regEn.type, regEn.typePattern);
    cerP->entity.fill(enId);
    cerP->error.fill(SccOk);

    ContextAttribute* caP = new ContextAttribute(regAttr, "", "");

    caP->provider = provider;
    caP->providerRegId = regId;
    cerP->entity.attributeVector.push_back(caP);
    cerV.push_back(cerP);
  }
}



/* ****************************************************************************
*
* matchEntityInRegistration -
*/
static bool matchEntityInRegistration(const ngsiv2::Registration& reg, const EntityId* enP)
{
  for (unsigned int ix = 0; ix < reg.dataProvided.entities.size(); ++ix)
  {
    EntityId entId = reg.dataProvided.entities[ix];

    if (matchEntity(enP, entId))
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
* This function takes a Registrations vector and adds the Context Providers in the CER vector
* (except the ones corresponding to some locally found attribute, i.e. info already in the
* CER vector)
*
* The limitReached parameter is to prevent the addition of new entities, which is needed in the case of the pagination
* limit has been reached with local entities.
*
* The enP parameter is optional. If not NULL, then before adding a CPr the function checks that the
* containting Registrations matches the entity (this is used for funcionality related to  "generic queries", see
* processGenericEntities() function).
*/
static void addContextProviders
(
  ContextElementResponseVector&            cerV,
  const std::vector<ngsiv2::Registration>& regV,
  bool                                     limitReached,
  const EntityId*                          enP = NULL
)
{
  for (unsigned int ix = 0; ix < regV.size(); ++ix)
  {
    ngsiv2::Registration reg = regV[ix];

    /* In case a "filtering" entity was provided, check that the current CRR matches or skip to next CRR */
    if (enP != NULL && !matchEntityInRegistration(reg, enP))
    {
      continue;
    }

    if (reg.dataProvided.attributes.size() == 0)
    {
      if (!limitReached)
      {
        /* Registration without attributes */
        for (unsigned int eIx = 0; eIx < reg.dataProvided.entities.size(); ++eIx)
        {
          addContextProviderEntity(cerV, reg.dataProvided.entities[eIx], reg.provider, reg.id);
        }
      }
    }
    else
    {
      /* Registration with attributes */
      for (unsigned int eIx = 0; eIx < reg.dataProvided.entities.size(); ++eIx)
      {
        for (unsigned int aIx = 0; aIx < reg.dataProvided.attributes.size(); ++aIx)
        {
          addContextProviderAttribute(cerV,
                                      reg.dataProvided.entities[eIx],
                                      reg.dataProvided.attributes[aIx],
                                      reg.provider,
                                      reg.id,
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
  const EntityIdVector&                    enV,
  ContextElementResponseVector&            cerV,
  const std::vector<ngsiv2::Registration>& regV,
  bool                                     limitReached
)
{
  for (unsigned int ix = 0; ix < enV.size(); ++ix)
  {
    const EntityId* enP = enV[ix];
    if (enP->type.empty() || !enP->idPattern.empty())
    {
      addContextProviders(cerV, regV, limitReached, enP);
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
*/
HttpStatusCode mongoQueryContext
(
  QueryContextRequest*                 requestP,
  QueryContextResponse*                responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,
  std::map<std::string, bool>&         options,
  long long*                           countP
)
{
  int         offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
  int         limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());

  std::string sortOrderList  = uriParams[URI_PARAM_SORTED];

  LM_T(LmtMongo, ("QueryContext Request"));
  LM_T(LmtPagination, ("Offset: %d, Limit: %d, Count: %s", offset, limit, (countP != NULL)? "true" : "false"));

  std::string                  err;
  bool                         ok;
  bool                         limitReached = false;
  bool                         reqSemTaken;
  ContextElementResponseVector rawCerV;

  reqSemTake(__FUNCTION__, "query request", SemReadOp, &reqSemTaken);

  ok = entitiesQuery(requestP->entityIdVector,
                     requestP->attributeList,
                     requestP->scopeVector,
                     &rawCerV,
                     &responseP->error,
                     tenant,
                     servicePathV,
                     offset,
                     limit,
                     &limitReached,
                     countP,
                     sortOrderList);

  if (!ok)
  {
    rawCerV.release();
    reqSemGive(__FUNCTION__, "query request", reqSemTaken);

    return SccOk;
  }

  /* In the case of empty response, if only generic processing is needed */
  if (rawCerV.size() == 0)
  {
    std::vector<ngsiv2::Registration> regV;
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, ngsiv2::ForwardQuery, &regV, &err, tenant, servicePathV))
    {
      if (regV.size() > 0)
      {
        processGenericEntities(requestP->entityIdVector, rawCerV, regV, limitReached);
      }
    }
  }

  /* First CPr lookup (in the case some CER is not found): looking in E-A registrations */
  if (someContextElementNotFound(rawCerV))
  {
    std::vector<ngsiv2::Registration> regV;
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, ngsiv2::ForwardQuery, &regV, &err, tenant, servicePathV))
    {
      if (regV.size() > 0)
      {
        fillContextProviders(rawCerV, regV);
        processGenericEntities(requestP->entityIdVector, rawCerV, regV, limitReached);
      }
    }
  }

  /* Second CPr lookup (in the case some elements still not being found): looking in E-<null> registrations */
  StringList attrNullList;

  if (someContextElementNotFound(rawCerV))
  {
    std::vector<ngsiv2::Registration> regV;
    if (registrationsQuery(requestP->entityIdVector, attrNullList, ngsiv2::ForwardQuery, &regV, &err, tenant, servicePathV))
    {
      if (regV.size() > 0)
      {
        fillContextProviders(rawCerV, regV);
      }
    }
  }

  /* Special case: request with <null> attributes. In that case, entitiesQuery() may have captured some local attribute, but
   * the list needs to be completed. Note that in the case of having this request someContextElementNotFound() is always false
   * so we efficient not invoking registrationsQuery() too much times
   */
  if (requestP->attributeList.size() == 0)
  {
    std::vector<ngsiv2::Registration> regV;
    if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, ngsiv2::ForwardQuery, &regV, &err, tenant, servicePathV))
    {
      if (regV.size() > 0)
      {
        addContextProviders(rawCerV, regV, limitReached);
      }
    }
  }

  /* Prune "not found" CERs */
  pruneContextElements(requestP->attrsList, rawCerV, &responseP->contextElementResponseVector);

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

      snprintf(details, sizeof(details), "Number of matching entities: %lld. Offset is %d", *countP, offset); // FIXME PR: this could be removed??
      responseP->error.fill(SccContextElementNotFound, details);
    }
    else
    {
      responseP->error.fill(SccContextElementNotFound);
    }
  }
  else if (countP != NULL)
  {
    // FIXME PR: Count: is a NGSIv1 thing... remove this
    //
    // If all was OK, but the details URI param was set to 'on', then the responses error code details
    // 'must' contain the total count of hits.
    //

    char details[64];

    snprintf(details, sizeof(details), "Count: %lld", *countP);
    responseP->error.fill(SccOk, details);
  }

  rawCerV.release();

  reqSemGive(__FUNCTION__, "query request", reqSemTaken);
  return SccOk;
}
