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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/sem.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoQueryContext.h"

#include "ngsi/ContextRegistrationResponse.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"

/* ****************************************************************************
*
* someContextElementNotFound -
*
* Returns true if some attribut with 'found' set to 'false' is found in the CER vector passed
* as argument
*
*/
bool someContextElementNotFound(ContextElementResponseVector& cerV)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    for (unsigned int jx = 0; jx < cerV.get(ix)->contextElement.contextAttributeVector.size(); ++jx)
    {
      if (!cerV.get(ix)->contextElement.contextAttributeVector.get(jx)->found)
      {
        return true;
      }
    }
  }
  return false;
}


/* ****************************************************************************
*
* fillContextProviders -
*
* Looks in the elements of the CER vector passed as argument, searching for a suitable CPr in the CRR
* vector passes as argument. If a suitable CPr is found, it is added to the CER (and the 'found' field
* changed to true)
*
* FIXME P10: this function is very complex (9-10 nested blocks... wow!) It should be refactored into several
* functions one #787 is ready
*
*/
void fillContextProviders(ContextElementResponseVector& cerV, ContextRegistrationResponseVector& crrV)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    ContextElementResponse* cer = cerV.get(ix);
    std::string entityId   = cer->contextElement.entityId.id;
    std::string entityType = cer->contextElement.entityId.type;

    for (unsigned int jx = 0; jx < cer->contextElement.contextAttributeVector.size(); ++jx)
    {
      ContextAttribute* ca = cer->contextElement.contextAttributeVector.get(jx);
      if (!ca->found)
      {
        std::string attrName = ca->name;
        /* Search for some CPr in crrV */
        std::string perEntPa  = "";
        std::string perAttrPa = "";
        for (unsigned int kx = 0; kx < crrV.size(); ++kx)
        {
          ContextRegistrationResponse* crr = crrV.get(kx);
          /* Is there a matching entity in the CRR? */
          for (unsigned lx = 0; lx < crr->contextRegistration.entityIdVector.size(); ++lx)
          {
            std::string regEntityId   = crr->contextRegistration.entityIdVector.get(lx)->id;
            std::string regEntityType = crr->contextRegistration.entityIdVector.get(lx)->type;

            if (regEntityId == entityId && (regEntityType == entityType || regEntityType == ""))
            {
              if (crr->contextRegistration.contextRegistrationAttributeVector.size() == 0)
              {
                /* CRR without attributes */
                perEntPa = crr->contextRegistration.providingApplication.get();
                break; /* lx */
              }
              else {
                /* Is there a matching entity or the abcense of attributes? */
                for (unsigned mx = 0; mx < crr->contextRegistration.contextRegistrationAttributeVector.size(); ++mx)
                {
                  std::string regAttrName = crr->contextRegistration.contextRegistrationAttributeVector.get(mx)->name;
                  if (regAttrName == attrName)
                  {
                    perAttrPa = crr->contextRegistration.providingApplication.get();
                    break; /* mx */
                  }
                }
                if (perAttrPa != "")
                {
                  break; /* lx */
                }
              }
            }
          }
          if (perAttrPa != "")
          {
            break; /* kx */
          }
        }
        /* Looking results after crrV processing */
        ca->providingApplication = perAttrPa == ""? perEntPa : perAttrPa;
        ca->found = (ca->providingApplication != "");
      }
    }
  }
}

/* ****************************************************************************
*
* addContextProviderEntity -
*
*/
void addContextProviderEntity(ContextElementResponseVector& cerV, EntityId* enP, std::string pa)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if (cerV.get(ix)->contextElement.entityId.id == enP->id && cerV.get(ix)->contextElement.entityId.type == enP->type)
    {
      cerV.get(ix)->contextElement.providingApplicationList.push_back(pa);
      return;    /* by construction, no more than one CER with the same entity information should exist in the CERV) */
    }
  }
}

/* ****************************************************************************
*
* addContextProviderAttribute -
*
*
*/
void addContextProviderAttribute(ContextElementResponseVector& cerV, EntityId* enP, ContextRegistrationAttribute* craP, std::string pa)
{
  for (unsigned int ix = 0; ix < cerV.size(); ++ix)
  {
    if (cerV.get(ix)->contextElement.entityId.id == enP->id && cerV.get(ix)->contextElement.entityId.type == enP->type)
    {
      for (unsigned int jx = 0; jx < cerV.get(ix)->contextElement.contextAttributeVector.size(); ++jx)
      {
        std::string attrName = cerV.get(ix)->contextElement.contextAttributeVector.get(jx)->name;
        if (attrName == craP->name)
        {
          /* In this case, the attribute has been already found in local database. CPr is unnecessary */
          return;
        }
      }
      /* Reached this point, no attribute was found, so adding it with corresponding CPr info */
      ContextAttribute* caP = new ContextAttribute(craP->name, "", "");
      caP->providingApplication = pa;
      cerV.get(ix)->contextElement.contextAttributeVector.push_back(caP);
    }
  }
}


/* ****************************************************************************
*
* addContextProviders -
*
*
* This functions takes a CRR vector and adds the Context Providers in the CER vector
* (except the ones corresponding to some locally found attribute, i.e. info already in the
* CER vector)
*
*/
void addContextProviders(ContextElementResponseVector& cerV, ContextRegistrationResponseVector& crrV)
{
  for (unsigned int ix = 0; ix < crrV.size(); ++ix)
  {
    ContextRegistration cr = crrV.get(ix)->contextRegistration;
    if (cr.contextRegistrationAttributeVector.size() == 0)
    {
      /* Registration without attributes */
      for (unsigned int jx = 0; jx < cr.entityIdVector.size(); ++jx)
      {
        addContextProviderEntity(cerV, cr.entityIdVector.get(jx), cr.providingApplication.get());
      }
    }
    else
    {
      /* Registration with attributes */
      for (unsigned int jx = 0; jx < cr.entityIdVector.size(); ++jx)
      {
        for (unsigned int kx = 0; kx < cr.contextRegistrationAttributeVector.size(); ++kx)
        {
          addContextProviderAttribute(cerV, cr.entityIdVector.get(jx), cr.contextRegistrationAttributeVector.get(kx), cr.providingApplication.get());
        }
      }
    }
  }
}

/* ****************************************************************************
*
* mongoQueryContext - 
*/
HttpStatusCode mongoQueryContext
(
  QueryContextRequest*                 requestP,
  QueryContextResponse*                responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams
)
{
    int         offset         = atoi(uriParams[URI_PARAM_PAGINATION_OFFSET].c_str());
    int         limit          = atoi(uriParams[URI_PARAM_PAGINATION_LIMIT].c_str());
    std::string detailsString  = uriParams[URI_PARAM_PAGINATION_DETAILS];
    bool        details        = (strcasecmp("on", detailsString.c_str()) == 0)? true : false;

    LM_T(LmtMongo, ("QueryContext Request"));    
    LM_T(LmtPagination, ("Offset: %d, Limit: %d, Details: %s", offset, limit, (details == true)? "true" : "false"));

    /* FIXME: restriction not supported for the moment */
    if (!requestP->restriction.attributeExpression.isEmpty())
    {
      LM_W(("Bad Input (restriction found, but restrictions are not supported by mongo backend)"));
    }

    std::string err;
    bool        ok;
    long long   count = -1;

    ContextElementResponseVector rawCerV;

    reqSemTake(__FUNCTION__, "ngsi10 query request");
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
                       details,
                       &count);
    reqSemGive(__FUNCTION__, "ngsi10 query request");

    if (!ok)
    {
        responseP->errorCode.fill(SccReceiverInternalError, err);
        return SccOk;
    }

    ContextRegistrationResponseVector crrV;

    /* First CPr lookup (in the case some CER is not found): looking in E-A registrations */
    if (someContextElementNotFound(rawCerV) &&
        registrationsQuery(requestP->entityIdVector, requestP->attributeList, &crrV, &err, tenant, servicePathV, 0, 0, false) &&
        (crrV.size() > 0))
    {
      fillContextProviders(rawCerV, crrV);
    }
    else
    {
      /* Different from fails in DB at entitiesQuery(), DB fails at registrationsQuery() are not considered "critical" */
      LM_E(("Database Error (%s)", err.c_str()));
    }
    crrV.release();

    /* Second CPr lookup (in the case some element stills not being found): looking in E-<null> registrations */
    AttributeList attrNullList;
    if (someContextElementNotFound(rawCerV) &&
        registrationsQuery(requestP->entityIdVector, attrNullList, &crrV, &err, tenant, servicePathV, 0, 0, false)
        && (crrV.size() > 0))
    {
      fillContextProviders(rawCerV, crrV);
    }
    else
    {
      /* Different from fails in DB at entitiesQuery(), DB fails at registrationsQuery() are not considered "critical" */
      LM_E(("Database Error (%s)", err.c_str()));
    }
    crrV.release();

    /* Special case: request with <null> attributes. In that case, entitiesQuery() may have captured some local attribute, but
     * the list need to be completed. Note that in the case of having this request someContextElementNotFound() is always false
     * so we efficient not invoking registrationQuery() too much times */
    if (requestP->attributeList.size() == 0 &&
        registrationsQuery(requestP->entityIdVector, requestP->attributeList, &crrV, &err, tenant, servicePathV, 0, 0, false) &&
        (crrV.size() > 0))
    {
      addContextProviders(rawCerV, crrV);
    }
    else
    {
      /* Different from fails in DB at entitiesQuery(), DB fails at registrationsQuery() are not considered "critical" */
      LM_E(("Database Error (%s)", err.c_str()));
    }
    crrV.release();

    /* Prune "not found" CERs */
    pruneNotFoundContextElements(rawCerV, &responseP->contextElementResponseVector);

    // FIXME: details and pagination suff has to be re-thought */
    if (responseP->contextElementResponseVector.size() == 0)
    {

      // If the query has an empty response, we have to fill in the status code part in the response.
      //
      // However, if the response was empty due to a too high pagination offset,
      // and if the user has asked for 'details' (as URI parameter, then the response should include information about
      // the number of hits without pagination.
      //

      if (details && (count > 0) && (offset >= count))
      {
        char details[256];

        snprintf(details, sizeof(details), "Number of matching entities: %lld. Offset is %d", count, offset);
        responseP->errorCode.fill(SccContextElementNotFound, details);
      }
      else
      {
        responseP->errorCode.fill(SccContextElementNotFound);
      }
    }
    else if (details)
    {
      //
      // If all was OK, but the details URI param was set to 'on', then the responses error code details
      // 'must' contain the total count of hits.
      //

      char details[64];

      snprintf(details, sizeof(details), "Count: %lld", count);
      responseP->errorCode.fill(SccOk, details);
    }

    return SccOk;

#if 0

      //
      // If the query has an empty response, we have to fill in the status code part in the response.
      //
      // However, if the response was empty due to a too high pagination offset,
      // and if the user has asked for 'details' (as URI parameter, then the response should include information about
      // the number of hits without pagination.
      //

      if (details)
      {
        if ((count > 0) && (offset >= count))
        {
          char details[256];

          snprintf(details, sizeof(details), "Number of matching entities: %lld. Offset is %d", count, offset);
          responseP->errorCode.fill(SccContextElementNotFound, details);
          return SccOk;
        }
      }

      responseP->errorCode.fill(SccContextElementNotFound);
    }

    else if (responseP->contextElementResponseVector.size() == 0)
    {
      // Check a pontential Context Provider in the registrations collection
      ContextRegistrationResponseVector crrV;
      std::string err;      
      // For now, we use limit=1. That ensures that max one providing application is returned. In the future,
      // we should consider leaving this limit open and define an algorithm to pick the right one, and ordered list, etc.
      if (registrationsQuery(requestP->entityIdVector, requestP->attributeList, &crrV, &err, tenant, servicePathV, 0, 1, false))
      {
        if (crrV.size() > 0)
        {
          // Suitable CProvider has been found: creating response and returning
          std::string prApp = crrV[0]->contextRegistration.providingApplication.get();
          LM_T(LmtCtxProviders, ("context provide found: %s", prApp.c_str()));
          responseP->errorCode.fill(SccFound, prApp);
          crrV.release();
          return SccOk;
        }
      }      
      else
      {
        LM_E(("Database Error (%s)", err.c_str()));
        crrV.release();  // Just in case
      }

      //
      // If the query has an empty response, we have to fill in the status code part in the response.
      //
      // However, if the response was empty due to a too high pagination offset,
      // and if the user has asked for 'details' (as URI parameter, then the response should include information about
      // the number of hits without pagination.
      //

      if (details)
      {
        if ((count > 0) && (offset >= count))
        {
          char details[256];

          snprintf(details, sizeof(details), "Number of matching entities: %lld. Offset is %d", count, offset);
          responseP->errorCode.fill(SccContextElementNotFound, details);
          return SccOk;
        }
      }

      responseP->errorCode.fill(SccContextElementNotFound);
    }

    else if (details == true)
    {
      //
      // If all was OK, but the details URI param was set to 'on', then the responses error code details
      // 'must' contain the total count of hits.
      //

      char details[64];

      snprintf(details, sizeof(details), "Count: %lld", count);
      responseP->errorCode.fill(SccOk, details);
    }

    return SccOk;
#endif
}
