/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "mongo/client/dbclient.h"                                  // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kbase/kMacros.h"                                          // K_FT
#include "kbase/kTime.h"                                            // kTimeGet
#include "kalloc/kaStrdup.h"                                        // kaStrdup
#include "kjson/KjNode.h"                                           // KjNode
#include "kjson/kjLookup.h"                                         // kjLookup
#include "kjson/kjBuilder.h"                                        // kjObject, ...
}

#include "logMsg/logMsg.h"                                          // LM_*
#include "logMsg/traceLevels.h"                                     // Lmt*

#include "mongoBackend/MongoGlobal.h"                               // getMongoConnection, releaseMongoConnection, ...

#include "orionld/common/numberToDate.h"                            // numberToDate
#include "orionld/common/eqForDot.h"                                // eqForDot
#include "orionld/common/performance.h"                             // REQUEST_PERFORMANCE
#include "orionld/db/dbCollectionPathGet.h"                         // dbCollectionPathGet
#include "orionld/db/dbConfiguration.h"                             // dbDataToKjTree
#include "orionld/context/orionldContextItemAliasLookup.h"          // orionldContextItemAliasLookup
#include "orionld/mongoCppLegacy/mongoCppLegacyEntityRetrieve.h"    // Own interface



// -----------------------------------------------------------------------------
//
// kjChildPrepend - FIXME: move to kjson library
//
static void kjChildPrepend(KjNode* container, KjNode* child)
{
  child->next = container->value.firstChildP;
  container->value.firstChildP = child;
}



// -----------------------------------------------------------------------------
//
// timestampToString -
//
static bool timestampToString(KjNode* nodeP)
{
  char*   dateBuf = kaAlloc(&orionldState.kalloc, 64);
  double  timestamp;

  if (nodeP->type == KjFloat)
    timestamp = nodeP->value.f;
  else if (nodeP->type == KjInt)
    timestamp = nodeP->value.i;
  else
  {
    LM_E(("Internal Error (not a number: %s)", kjValueType(nodeP->type)));
    return false;
  }

  if (numberToDate(timestamp, dateBuf, 64) == false)
  {
    LM_E(("Database Error (numberToDate failed)"));
    return false;
  }

  nodeP->type    = KjString;
  nodeP->value.s = dateBuf;

  return true;
}



// -----------------------------------------------------------------------------
//
// presentationAttributeFix -
//
// 1. Remove 'createdAt' and 'modifiedAt' is options=sysAttrs is not set
//
static bool presentationAttributeFix(KjNode* attrP, const char* entityId, bool sysAttrs, bool keyValues)
{
  if (keyValues == true)
  {
    KjNode*  typeP    = kjLookup(attrP, "type");
    KjNode*  valueP;

    if (typeP == NULL)
    {
      LM_E(("No 'type' field found"));
      return false;
    }
    else if (typeP->type != KjString)
    {
      LM_E(("'type' field not a string"));
      return false;
    }

    //
    // FIXME: Here I need to know what to look for!!!
    //        "value" or "object"
    //
    valueP = kjLookup(attrP, "value");
    if (valueP == NULL)
      valueP = kjLookup(attrP, "object");

    if (valueP == NULL)
    {
      LM_E(("Database Error (the %s '%s' has no value)", typeP->value.s, attrP->name));
      return false;
    }

    // Inherit the value field
    attrP->type      = valueP->type;
    attrP->value     = valueP->value;
    attrP->lastChild = valueP->lastChild;
  }
  else if (sysAttrs == false)
  {
    KjNode* createdAtP  = kjLookup(attrP, "createdAt");
    KjNode* modifiedAtP = kjLookup(attrP, "modifiedAt");

    if (createdAtP != NULL)
      kjChildRemove(attrP, createdAtP);

    if (modifiedAtP != NULL)
      kjChildRemove(attrP, modifiedAtP);
  }
  else
  {
    KjNode* createdAtP  = kjLookup(attrP, "createdAt");
    KjNode* modifiedAtP = kjLookup(attrP, "modifiedAt");

    if (createdAtP != NULL)
      timestampToString(createdAtP);

    if (modifiedAtP != NULL)
      timestampToString(modifiedAtP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// datamodelAttributeFix -
//
// Due to the different database model (the database model of NGSIv1 is used for NGSI-LD),
// a few changes to the original attributes must be done:
//
// 1. Change "value" to "object" for all attributes that are "Relationship".
//    Note that the "object" field of a Relationship is stored in the database under the field "value".
//    That fact is fixed here, by renaming the "value" to "object" for attr with type == Relationship.
//    This depends on the database model and thus should be fixed in the database layer.
//
// 2. Change 'creDate' to 'createdAt' and 'modDate' to 'modifiedAt', but only of sysAttrs == true
//    If sysAttrs == false, then 'creDate' and 'modDate' are removed
//
// 3. mdNames must go
//
// 4. All metadata in 'md' must be placed one level higher
//
static bool datamodelAttributeFix(KjNode* attrP, const char* entityId, bool sysAttrs)
{
  //
  // 1. Change "value" to "object" for all attributes that are "Relationship"
  //
  KjNode* typeP = kjLookup(attrP, "type");

  if (typeP == NULL)
  {
    LM_E(("Database Error (field 'type' not found for attribute '%s' of entity '%s')", attrP->name, entityId));
    return false;
  }

  if (typeP->type != KjString)
  {
    LM_E(("Database Error (field 'type' not a String for attribute '%s' of entity '%s')", attrP->name, entityId));
    return false;
  }

  if (strcmp(typeP->value.s, "Relationship") == 0)
  {
    KjNode* objectP = kjLookup(attrP, "value");

    if (objectP == NULL)
    {
      LM_E(("Database Error (field 'value' not found for attribute '%s' of entity '%s')", attrP->name, entityId));
      return false;
    }

    objectP->name = (char*) "object";
  }

  //
  // 2. sysAttrs
  //
  KjNode* creDateP = kjLookup(attrP, "creDate");
  KjNode* modDateP = kjLookup(attrP, "modDate");

  if (creDateP != NULL)
  {
    if (orionldState.uriParamOptions.sysAttrs == false)
      kjChildRemove(attrP, creDateP);
    else
      creDateP->name = (char*) "createdAt";
  }

  if (modDateP != NULL)
  {
    if (orionldState.uriParamOptions.sysAttrs == false)
      kjChildRemove(attrP, modDateP);
    else
      modDateP->name = (char*) "modifiedAt";
  }

  //
  // 3. mdNames must go
  //
  KjNode* mdNamesP = kjLookup(attrP, "mdNames");

  if (mdNamesP != NULL)
    kjChildRemove(attrP, mdNamesP);

  //
  // 4. All metadata in 'md' must be placed one level higher
  //
  KjNode* mdP = kjLookup(attrP, "md");

  if (mdP != NULL)
  {
    for (KjNode* metadataP = mdP->value.firstChildP; metadataP != NULL; metadataP = metadataP->next)
    {
      char* mdName = kaStrdup(&orionldState.kalloc, metadataP->name);

      // FIXME: due to a bug, I expand observedAt in either PATCH Entity or PATCH Attribute :(
      //        Because of this, I must do the compaction BEFORE I check for observedAt/unitCode
      //        That's unnecessary time-consuming and this bug must be fixed +
      //        the call to eqForDot+orionldContextItemAliasLookup moved to after checking for
      //        special attributes (observedAt/unitCode).
      //
      eqForDot(mdName);
      metadataP->name = orionldContextItemAliasLookup(orionldState.contextP, mdName, NULL, NULL);

      //
      // Special fields:
      // - observedAt
      // - unitCode
      //
      if (strcmp(metadataP->name, "observedAt") == 0)
      {
        if (metadataP->type == KjObject)
        {
          metadataP->type  = metadataP->value.firstChildP->type;
          metadataP->value = metadataP->value.firstChildP->value;
        }

        if ((metadataP->type == KjInt) || (metadataP->type == KjFloat))
          timestampToString(metadataP);

        continue;
      }
      else if (strcmp(metadataP->name, "unitCode") == 0)
      {
        if (metadataP->type == KjObject)
        {
          metadataP->type  = metadataP->value.firstChildP->type;
          metadataP->value = metadataP->value.firstChildP->value;
        }

        continue;
      }

      // If Relationship - change 'value' for 'object'
      KjNode* typeP = kjLookup(metadataP, "type");
      if (typeP == NULL)
      {
        LM_E(("Database Error (field 'type' not found for metadata '%s' of attribute '%s' of entity '%s')", metadataP->name, attrP->name, entityId));
        return false;
      }
      else if (typeP->type != KjString)
      {
        LM_E(("Database Error (field 'type' not a String for metadata '%s' of attribute '%s' of entity '%s')", metadataP->name, attrP->name, entityId));
        return false;
      }

      if (strcmp(typeP->value.s, "Relationship") == 0)
      {
        KjNode* objectP = kjLookup(metadataP, "value");

        if (objectP != NULL)
          objectP->name = (char*) "object";
      }
    }

    attrP->lastChild->next = mdP->value.firstChildP;
    attrP->lastChild       = mdP->lastChild;

    kjChildRemove(attrP, mdP);
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// mongoCppLegacyEntityRetrieve -
//
// FIXME: Move database model relevant code to some other function (for reusal)
//
// PARAMETERS
//   entityId        ID of the entity to be retrieved
//   attrs           array of attribute names, terminated by a NULL pointer
//   attrMandatory   If true - the entity is found only if any of the attributes in 'attrs'
//                   is present in the entity
//   sysAttrs        include 'createdAt' and 'modifiedAt'
//   keyValues       short representation of the attributes
//
KjNode* mongoCppLegacyEntityRetrieve(const char* entityId, char** attrs, bool attrMandatory, bool sysAttrs, bool keyValues, const char* datasetId)
{
  char    collectionPath[256];
  KjNode* attrTree  = NULL;
  KjNode* dbTree    = NULL;

  dbCollectionPathGet(collectionPath, sizeof(collectionPath), "entities");


  //
  // Populate 'queryBuilder' - only Entity ID for this operation
  //
  mongo::BSONObjBuilder  queryBuilder;
  queryBuilder.append("_id.id", entityId);


  // 1. Get _id - make a kj tree
  // 2. Get attrs - make a kj tree
  // 3. Change "value" to "object" for all attributes that are "Relationship"
  // 4. Get @dataset - make a tree
  // 5. Move every dataset attribute values to its corresponding attribute in attrs
  // 6. Merge _id and attrs into one single tree
  //


  mongo::DBClientBase*                  connectionP = getMongoConnection();
  std::auto_ptr<mongo::DBClientCursor>  cursorP;
  mongo::Query                          query(queryBuilder.obj());
  mongo::BSONObj                        retFieldsObj;
  mongo::BSONObjBuilder                 retFieldsBuilder;

  // _id is there by default
  retFieldsBuilder.append("attrs", 1);
  retFieldsBuilder.append("@datasets", 1);

  if (sysAttrs == true)
  {
    retFieldsBuilder.append("creDate", 1);
    retFieldsBuilder.append("modDate", 1);
  }

  retFieldsObj = retFieldsBuilder.obj();

  //
  // Querying mongo and retrieving the results
  //
#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.dbStart);
#endif
  cursorP = connectionP->query(collectionPath, query, 0, 0, &retFieldsObj);
#ifdef REQUEST_PERFORMANCE
  kTimeGet(&timestamps.dbEnd);
#endif

  while (cursorP->more())
  {
    mongo::BSONObj  bsonObj = cursorP->nextSafe();
    char*           title;
    char*           details;

    dbTree = dbDataToKjTree(&bsonObj, false, &title, &details);
    if (dbTree == NULL)
      LM_E(("%s: %s", title, details));
    break;
  }

  releaseMongoConnection(connectionP);

  if (dbTree == NULL)  // Entity not found
    return NULL;


  KjNode*  dbAttrsP           = kjLookup(dbTree, "attrs");      // Must be there
  KjNode*  dbDataSetsP        = kjLookup(dbTree, "@datasets");  // May not be there

  if (dbAttrsP == NULL)
  {
    LM_E(("Internal Error (field 'attrs' not found for entity '%s')", entityId));
    return NULL;
  }

  //
  // Attributes may be found both in dbAttrsP and in dbDataSetsP
  //
  // If 'attrs' given:
  // - include those attributes that are found in (either 'dbAttrsP' or 'dbDataSetsP')  AND in 'attrs'
  //
  // Else (no 'attrs' given):
  //
  //
  if ((attrs == NULL) || (attrs[0] == NULL))     // Include all attributes in the response
  {
    if (keyValues == false)
    {
      for (KjNode* attrP = dbAttrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
      {
        datamodelAttributeFix(attrP, entityId, sysAttrs);
      }
    }

    if ((dbDataSetsP != NULL) && (dbDataSetsP->value.firstChildP != NULL))
    {
      //
      // Go over 'dbAttrsP' and incorporate all attributes from there into 'dbDataSetsP'
      //
      // If an attribute from 'dbAttrsP' is found in 'dbDataSetsP', it is inserted as
      // 'yet another instance' of the attribute array in datasets
      //
      // If not found in 'dbDataSetsP', it is inserted as a new attibute in 'dbDataSetsP'
      // I.e. one level higher, as the first and only instance of the attribute
      //
      KjNode* attrP = dbAttrsP->value.firstChildP;
      KjNode* next;

      while (attrP != NULL)
      {
        next = attrP->next;

        kjChildRemove(dbAttrsP, attrP);

        KjNode* didAttrP = kjLookup(dbDataSetsP, attrP->name);

        if (didAttrP)  // Attribute found in 'datasets' - add attribute instance to the datasets array for the attribute
        {
          // I want the "non datasetId instance" at the start of the array - can't use kjChildAdd for this
          kjChildPrepend(didAttrP, attrP);
        }
        else
          kjChildAdd(dbDataSetsP, attrP);

        attrP = next;
      }

      attrTree = dbDataSetsP;
    }
    else  // No datasets - simply use dbAttrsP
      attrTree = dbAttrsP;
  }
  else  // Filter attributes according to the 'attrs' URI param
  {
    attrTree = kjObject(orionldState.kjsonP, NULL);

    KjNode* attrP;
    KjNode* datasetP;
    int     includedAttributes = 0;
    int     ix                 = 0;

    while (attrs[ix] != NULL)
    {
      attrP    = kjLookup(dbAttrsP, attrs[ix]);
      datasetP = (dbDataSetsP == NULL)? NULL : kjLookup(dbDataSetsP, attrs[ix]);

      if (attrP != NULL)
      {
        if (keyValues == false)
          datamodelAttributeFix(attrP, entityId, sysAttrs);
      }

      if ((datasetP != NULL) && (attrP != NULL))
      {
        kjChildRemove(dbDataSetsP, datasetP);
        kjChildAdd(attrTree, datasetP);

        kjChildRemove(dbAttrsP, attrP);
        kjChildPrepend(datasetP, attrP);
        ++includedAttributes;
      }
      else if (attrP != NULL)
      {
        kjChildRemove(dbAttrsP, attrP);
        kjChildAdd(attrTree, attrP);
        ++includedAttributes;
      }
      else if (datasetP != NULL)
      {
        kjChildRemove(dbDataSetsP, datasetP);
        kjChildAdd(attrTree, datasetP);
        ++includedAttributes;
      }

      ++ix;
    }

    if ((includedAttributes == 0) && (attrMandatory == true))
    {
      // 404 not found ...
      // The Entity exists, but it doesn't have ANY of the attributes in attrsP
      return NULL;
    }
  }


  //
  // The data from the database must be altered to fit the NGSI-LD data model
  //
  for (KjNode* attrP = attrTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    bool special = false;

    if (strcmp(attrP->name, "location") == 0)
      special = true;

    if (special == false)
    {
      char* attrName            = kaStrdup(&orionldState.kalloc, attrP->name);
      bool  valueMayBeCompacted = false;
      eqForDot(attrName);

      attrP->name = orionldContextItemAliasLookup(orionldState.contextP, attrName, &valueMayBeCompacted, NULL);

      if (valueMayBeCompacted == true)
      {
        KjNode* valueP = kjLookup(attrP, "value");

        if (valueP != NULL)
        {
          if (valueP->type == KjString)
            valueP->value.s = orionldContextItemAliasLookup(orionldState.contextP, valueP->value.s, NULL, NULL);
          else if (valueP->type == KjArray)
          {
            for (KjNode* arrItemP = valueP->value.firstChildP; arrItemP != NULL; arrItemP = arrItemP->next)
            {
              if (arrItemP->type == KjString)
                arrItemP->value.s = orionldContextItemAliasLookup(orionldState.contextP, arrItemP->value.s, NULL, NULL);
            }
          }
        }
      }
    }

    if (attrP->type == KjObject)
    {
      if (presentationAttributeFix(attrP, entityId, sysAttrs, keyValues) == false)
      {
        LM_E(("Internal Error (presentationAttributeFix failed)"));
        return NULL;
      }
    }
    else  // KjArray
    {
      int instances = 0;
      for (KjNode* aP = attrP->value.firstChildP; aP != NULL; aP = aP->next)
      {
        if (presentationAttributeFix(aP, entityId, sysAttrs, keyValues) == false)
        {
          LM_E(("presentationAttributeFix failed"));
          return NULL;
        }
        ++instances;
      }

      if (instances == 1)  // No array needed
      {
        attrP->value.firstChildP = attrP->value.firstChildP->value.firstChildP;
        attrP->lastChild         = attrP->value.firstChildP->lastChild;
        attrP->type              = KjObject;
      }
    }
  }


  KjNode* idP = kjLookup(dbTree, "_id");

  if (idP == NULL)
  {
    LM_E(("Internal Error (field '_id' not found for entity '%s')", entityId));
    return NULL;
  }

  KjNode* typeP        = kjLookup(idP, "type");
  KjNode* servicePathP = kjLookup(idP, "servicePath");

  if (typeP == NULL)
  {
    LM_E(("Internal Error (field '_id.type' not found for entity '%s')", entityId));
    return NULL;
  }

  if (servicePathP != NULL)
    kjChildRemove(idP, servicePathP);

  typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);

  if (sysAttrs == true)
  {
    KjNode*  creDateP = kjLookup(dbTree, "creDate");
    KjNode*  modDateP = kjLookup(dbTree, "modDate");

    if (creDateP != NULL)
    {
      kjChildRemove(dbTree, creDateP);
      kjChildAdd(idP, creDateP);
      creDateP->name = (char*) "createdAt";
      timestampToString(creDateP);
    }

    if (modDateP != NULL)
    {
      kjChildRemove(dbTree, modDateP);
      kjChildAdd(idP, modDateP);
      modDateP->name = (char*) "modifiedAt";
      timestampToString(modDateP);
    }
  }

  // Merge idP and attrTree
  if ((attrTree != NULL) && (attrTree->value.firstChildP != NULL))
  {
    idP->lastChild->next = attrTree->value.firstChildP;
    idP->lastChild       = attrTree->lastChild;
  }

  return idP;
}
