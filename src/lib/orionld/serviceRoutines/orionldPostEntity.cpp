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
#include <string.h>                                              // strlen

extern "C"
{
#include "kjson/kjBuilder.h"                                     // kjChildRemove
#include "kjson/kjRender.h"                                      // kjRender
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "common/MimeType.h"                                     // MimeType
#include "mongoBackend/mongoUpdateContext.h"                     // mongoUpdateContext
#include "mongoBackend/mongoEntityExists.h"                      // mongoEntityExists
#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "ngsi10/UpdateContextResponse.h"                        // UpdateContextResponse
#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "orionld/common/CHECK.h"                                // CHECK
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldAttributeTreat.h"                // orionldAttributeTreat
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/eqForDot.h"                             // eqForDot
#include "orionld/db/dbEntityLookup.h"                           // dbEntityLookup
#include "orionld/db/dbEntityUpdate.h"                           // dbEntityUpdate
#include "orionld/context/orionldUriExpand.h"                    // orionldUriExpand
#include "orionld/context/orionldAliasLookup.h"                  // orionldAliasLookup
#include "orionld/context/orionldValueExpand.h"                  // orionldValueExpand
#include "orionld/serviceRoutines/orionldPostEntity.h"           // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPartialUpdateResponseCreate -
//
void orionldPartialUpdateResponseCreate(ConnectionInfo* ciP)
{
  //
  // Rob the incoming Request Tree - performance to be won!
  //
  orionldState.responseTree = orionldState.requestTree;
  orionldState.requestTree  = NULL;

  //
  // For all attrs in orionldState.responseTree, remove those that are found in orionldState.errorAttributeArrayP.
  // Remember, the format of orionldState.errorAttributeArrayP is:
  //
  //   |attrName|attrName|attrName|...
  //

  KjNode* attrNodeP = orionldState.responseTree->value.firstChildP;

  while (attrNodeP != NULL)
  {
    char*   match;
    KjNode* next = attrNodeP->next;
    bool    moved = false;

    if ((match = strstr(orionldState.errorAttributeArrayP, attrNodeP->name)) != NULL)
    {
      if ((match[-1] == '|') && (match[strlen(attrNodeP->name)] == '|'))
      {
        kjChildRemove(orionldState.responseTree, attrNodeP);
        attrNodeP = next;
        moved = true;
      }
    }

    if (moved == false)
      attrNodeP = attrNodeP->next;
  }
}



// -----------------------------------------------------------------------------
//
// kjTreeToContextElement -
//
bool kjTreeToContextElement(ConnectionInfo* ciP, KjNode* requestTreeP, ContextElement* ceP)
{
  // Iterate over the object, to get all attributes
  for (KjNode* kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    KjNode*           attrTypeNodeP = NULL;
    ContextAttribute* caP           = new ContextAttribute();

    if (strcmp(kNodeP->name, "createdAt") == 0)
      continue;

    if (strcmp(kNodeP->name, "modifiedAt") == 0)
      continue;

    LM_TMP(("NFY: calling orionldAttributeTreat for attribute '%s'", kNodeP->name));
    if (orionldAttributeTreat(ciP, kNodeP, caP, &attrTypeNodeP) == false)
    {
      LM_E(("orionldAttributeTreat failed"));
      delete caP;
      return false;
    }
    LM_TMP(("NFY: after orionldAttributeTreat for attribute '%s'", caP->name.c_str()));

    if (attrTypeNodeP != NULL)
      ceP->contextAttributeVector.push_back(caP);
    else
      delete caP;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeMergeAddNewAttrsIgnoreExisting -
//
bool kjTreeMergeAddNewAttrsIgnoreExisting(KjNode* sourceTree, KjNode* modTree)
{
  for (KjNode* modAttrP = modTree->value.firstChildP; modAttrP != NULL; modAttrP = modAttrP->next)
  {
    //
    // If modAttrP exists in sourceTree, then we ignore the attr and it stays in modTree
    //
    if (kjLookup(sourceTree, modAttrP->name) != NULL)
      continue;

    // Not there - remove modAttrP from modTree and add to sourceTree
    kjChildRemove(modTree, modAttrP);
    kjChildAdd(sourceTree, modAttrP);
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjNodeAttributeMerge
//
bool kjNodeAttributeMerge(KjNode* sourceP, KjNode* updateP)
{
  //
  // Go over the entire updateP tree and replace all those nodes in sourceP
  // Also, update the modDate node to the current date/time
  //
  KjNode*  modDateP  = kjLookup(sourceP, "modDate");
  KjNode*  mdP       = kjLookup(sourceP, "md");
  KjNode*  mdNamesP  = kjLookup(sourceP, "mdNames");
  int      ix        = 0;
  KjNode*  nodeP     = updateP->value.firstChildP;

  modDateP->value.i = time(NULL);

  if (mdNamesP == NULL)
  {
    mdNamesP = kjArray(orionldState.kjsonP, "mdNames");
    kjChildAdd(sourceP, mdNamesP);
  }

  LM_TMP(("NFY: In kjNodeAttributeMerge for attribute '%s'", updateP->name));

  while (nodeP != NULL)
  {
    KjNode* next = nodeP->next;

    //
    // Aware of "object" in a Relationship - needs to change name to "value"
    //
    if (strcmp(nodeP->name, "object") == 0)
      nodeP->name = (char*) "value";

    LM_TMP(("NFY: Checking item %d of updateP: %s (next at %p)", ix, nodeP->name, nodeP->next));
    KjNode* sameNodeInSourceP = kjLookup(sourceP, nodeP->name);

    if (sameNodeInSourceP != NULL)
    {
      LM_TMP(("NFY: Found '%s' member in source - removing it", sameNodeInSourceP->name));
      kjChildRemove(sourceP, sameNodeInSourceP);
      // NOT removing the name from "mdNames"
    }

    LM_TMP(("NFY: Adding '%s' member to toplevel/mdP/mdNamesP of SOURCE", nodeP->name));

    //
    // Should the item be added to right under the attribute, or as a metadata?
    // All items are treated at metadata except:
    // - type
    // - value
    // - creDate
    // - modDate
    // - Those that were found in top-level of the source attribute
    //
    if (strcmp(nodeP->name, "type") == 0)
      kjChildAdd(sourceP, nodeP);
    else if (strcmp(nodeP->name, "value") == 0)
      kjChildAdd(sourceP, nodeP);
    else if (strcmp(nodeP->name, "creDate") == 0)
      kjChildAdd(sourceP, nodeP);
    else if (strcmp(nodeP->name, "modDate") == 0)
      kjChildAdd(sourceP, nodeP);
    else if (sameNodeInSourceP != NULL)
      kjChildAdd(sourceP, nodeP);
    else
    {
      if (mdP == NULL)  // "md" is only created when needed whereas, "mdNames" must always be present
      {
        mdP = kjObject(orionldState.kjsonP, "md");
        kjChildAdd(sourceP, mdP);
      }

      char* nameWithDots = kaStrdup(&orionldState.kalloc, nodeP->name);

      LM_TMP(("DOT: replacing dots for EQs in '%s'", nodeP->name));
      dotForEq(nodeP->name);  // Changing DOTs for EQs for the name of the METADATA
      LM_TMP(("DOT: replaced dots for EQs in '%s'", nodeP->name));
      kjChildAdd(mdP, nodeP);
      if (sameNodeInSourceP == NULL)
        kjChildAdd(mdNamesP, kjString(orionldState.kjsonP, "", nameWithDots));
    }
    ++ix;
    LM_TMP(("NFY: Treated item %d of updateP: %s (next at %p)", ix, nodeP->name, nodeP->next));

    nodeP = next;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjSysAttrs - add sysAttrs (creDate + modDate) to an attribute
//
void kjSysAttrs(KjNode* attrP)
{
  KjNode* creDateP = NULL;
  KjNode* modDateP = NULL;

  for (KjNode* nodeP = attrP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE8(nodeP->name, 'm', 'o', 'd', 'D', 'a', 't', 'e', 0))
      modDateP = nodeP;
    else if (SCOMPARE8(nodeP->name, 'c', 'r', 'e', 'D', 'a', 't', 'e', 0))
      creDateP = nodeP;
  }

  if (creDateP == NULL)
  {
    creDateP = kjInteger(orionldState.kjsonP, "creDate", time(NULL));
    kjChildAdd(attrP, creDateP);
  }

  if (modDateP != NULL)
  {
    modDateP->value.i = time(NULL);
    modDateP->type    = KjInt;
  }
  else
  {
    modDateP = kjInteger(orionldState.kjsonP, "modDate", time(NULL));
    kjChildAdd(attrP, modDateP);
  }
}



// -----------------------------------------------------------------------------
//
// kjModDateSet -
//
void kjModDateSet(KjNode* attrP)
{
  for (KjNode* nodeP = attrP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (SCOMPARE8(nodeP->name, 'm', 'o', 'd', 'D', 'a', 't', 'e', 0))
    {
      nodeP->value.i = time(NULL);
      return;
    }
  }
}



#if 0
// -----------------------------------------------------------------------------
//
// kjTreeLog - DEBUGGING FUNCTION - To Be Removed
//
static void kjTreeLog(const char* comment, KjNode* nodeP)
{
  char buf[2048];

  kjRender(orionldState.kjsonP, nodeP, buf, sizeof(buf));

  LM_TMP(("NFY: %s: %s", comment, buf));
}
#endif



// -----------------------------------------------------------------------------
//
// objectToValue -
//
static void objectToValue(KjNode* attrP)
{
  KjNode* typeNodeP = kjLookup(attrP, "type");

  // If the attribute is a Relationship, then the "object" field should change name to "value"
  if ((typeNodeP != NULL) && (strcmp(typeNodeP->value.s, "Relationship") == 0))
  {
    KjNode* objectNodeP = kjLookup(attrP, "object");

    if (objectNodeP != NULL)
    {
      LM_TMP(("KZ: Changing 'object' for 'value' for Relationship '%s'", attrP->name));
      objectNodeP->name = (char*) "value";
    }
  }
}



// -----------------------------------------------------------------------------
//
// kjAttributePropertiesToMetadataVector -
//
// The API v1/v2 data model states that all properties of an attribute except special properties like creDate etc,
// go in the metadata vector "md", and their names, for faster lookups go to the vector "mdNames"
//
// When parsed, the tree doesn't look like that, as the incoming payload doesn't - this function moves the non-special
// properties of an attribute to the metadata vector
//
// The Special Properties are:
//   o type
//   o creDate
//   o modDate
//   o value
//   o md
//   o mdNames
//
// All other properties under "attribute toplevel" must be moved to under "md" and their names be included in "mdNames".
// This is the API v1/v2 data model
//
static void kjAttributePropertiesToMetadataVector(KjNode* attrP)
{
  KjNode* mdP      = NULL;
  KjNode* mdNamesP = NULL;

  for (KjNode* propP = attrP->value.firstChildP; propP != NULL; propP = propP->next)
  {
    LM_TMP(("NFY: Looking for md/mdNames - found '%s'", propP->name));
    if (strcmp(propP->name, "md") == 0)
      mdP = propP;
    else if (strcmp(propP->name, "mdNames") == 0)
      mdNamesP = propP;
  }

  if (mdNamesP == NULL)
  {
    mdNamesP = kjArray(orionldState.kjsonP, "mdNames");
    kjChildAdd(attrP, mdNamesP);
    LM_TMP(("NFY: Added 'mdNames'"));
  }

  //
  // Be very careful when looping over a linked list and removing items from the list
  //
  // If I did simply:
  //
  // for (KjNode* propP = attrP->value.firstChildP; propP != NULL; propP = propP->next)
  // {
  // }
  // ... it would fail, because when I remove propP from the list, and append it to some other list, I change its next-pointer
  // So, propP->next is either set to NULL (and the loop ends prematurely) or it is set to point to some other item is some other list ...
  //
  // VERY DANGEROUS!!!
  //
  // Only way to this safely is save the next pointer in the beginning of the loop, bafore any item is removed from the list
  //
  KjNode* next;
  KjNode* propP = attrP->value.firstChildP;

  while (propP != NULL)
  {
    char* propName = propP->name;

    next = propP->next;

    LM_TMP(("NFY: Should the attr-property '%s' be moved to md/mdNames?", propP->name));

    if (strcmp(propName, "type") == 0)
    {}
    else if (strcmp(propName, "creDate") == 0)
    {}
    else if (strcmp(propName, "modDate") == 0)
    {}
    else if (strcmp(propName, "value") == 0)
    {}
    else if (strcmp(propName, "object") == 0)
    {
      propP->name = (char*) "value";
    }
    else if (strcmp(propName, "md") == 0)
    {}
    else if (strcmp(propName, "mdNames") == 0)
    {}
    else
    {
      if (mdP == NULL)  // "md" is only created when needed whereas, "mdNames" must always be present
      {
        mdP = kjObject(orionldState.kjsonP, "md");
        kjChildAdd(attrP, mdP);
        LM_TMP(("NFY: Added 'md'"));
      }

      LM_TMP(("NFY: Yes - '%s' is moved to md/mdNames", propP->name));
      kjChildRemove(attrP, propP);
      objectToValue(propP);
      kjChildAdd(mdP, propP);
      kjChildAdd(mdNamesP, kjString(orionldState.kjsonP, "", propP->name));
    }

    propP = next;
  }
}



// -----------------------------------------------------------------------------
//
// kjTreeMergeAddNewAttrsOverwriteExisting -
//
bool kjTreeMergeAddNewAttrsOverwriteExisting(KjNode* sourceTree, KjNode* modTree, char** titleP, char** detailsP)
{
  LM_TMP(("NFY: In kjTreeMergeAddNewAttrsOverwriteExisting"));
  //
  // The data model of Orion is that all attributes go in toplevel::attrs
  // So, we need to reposition "sourceTree" so that it points to the sourceTree::atts
  //
  KjNode* attrNamesP = kjLookup(sourceTree, "attrNames");
  KjNode* attrsP     = kjLookup(sourceTree, "attrs");

  if (attrsP == NULL)
  {
    *titleP   = (char*) "Corrupt Database";
    *detailsP = (char*) "No /attrs/ member found in Entity DB data";
    return NULL;
  }

  if (attrNamesP == NULL)
  {
    *titleP   = (char*) "Corrupt Database";
    *detailsP = (char*) "No /attrNames/ member found in Entity DB data";
    return NULL;
  }

  KjNode* modAttrP = modTree->value.firstChildP;

  //
  // For every attribute in the incoming payload
  //
  while (modAttrP != NULL)
  {
    KjNode* next = modAttrP->next;

    if (modAttrP->type == KjObject)
      objectToValue(modAttrP);
    else
    {
      if (strcmp(modAttrP->name, "type") == 0)
        modAttrP = next;
      else if (strcmp(modAttrP->name, "value") == 0)
        modAttrP = next;
      else if (strcmp(modAttrP->name, "object") == 0)
        modAttrP = next;
      else
      {
        LM_E(("Error: attr '%s' in tree is not a JSON object", modAttrP->name));
        *titleP   = (char*) "Invalid JSON type";
        *detailsP = (char*) "Not a JSON object";
        return false;
      }

      continue;
    }

    //
    // If "modAttrP" exists in sourceTree, then we have to remove it - to later add the one from "modTree"
    //   - if found in sourceTree: merge sourceTreeAttrP with modAttrP
    //   - if NOT found:
    //     - remove modAttrP from modTree
    //     - add modAttrP to sourceTree
    //     - add slot in "attrNames"
    //
    KjNode* sourceTreeAttrP = NULL;
    LM_TMP(("NFY: Looking for '%s'", modAttrP->name));
    if ((sourceTreeAttrP = kjLookup(attrsP, modAttrP->name)) != NULL)
    {
      LM_TMP(("NFY: Found it - calling kjNodeAttributeMerge"));
      kjNodeAttributeMerge(sourceTreeAttrP, modAttrP);
      kjModDateSet(sourceTreeAttrP);
    }
    else
    {
      LM_TMP(("NFY: Did not find it ('%s') - adding as new", modAttrP->name));

      // kjTreeLog("Adding attribute", modAttrP);

      // Remove modAttrP from modTree and add to sourceTree
      LM_TMP(("NFY: Remove modAttrP '%s' from modTree and add to sourceTree", modAttrP->name));
      kjChildRemove(modTree, modAttrP);
      kjAttributePropertiesToMetadataVector(modAttrP);
      kjChildAdd(attrsP, modAttrP);
      kjSysAttrs(modAttrP);

      // Orion Data Model: Must add a mdNames: []
      KjNode* mdArrayP = kjArray(orionldState.kjsonP, "mdNames");
      kjChildAdd(modAttrP, mdArrayP);

      //
      // Add attribute name to "attrNames"
      //
      //
      // The dots in the attribute name have been replaced with '='
      // We need to change that back before adding to "attrNames"
      //
      char*    attrNameWithDots = kaStrdup(&orionldState.kalloc, modAttrP->name);
      KjNode*  attrNameNodeP;

      LM_TMP(("DOT: putting dots back for '%s', to add to 'attrNames'", attrNameWithDots));
      eqForDot(attrNameWithDots);
      LM_TMP(("DOT: put dots back for '%s', to add to 'attrNames'", attrNameWithDots));

      attrNameNodeP = kjString(orionldState.kjsonP, NULL, attrNameWithDots);
      kjChildAdd(attrNamesP, attrNameNodeP);
    }

    modAttrP = next;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// expandAttrNames -
//
static bool expandAttrNames(KjNode* treeP, char** detailsP)
{
  for (KjNode* attrP = treeP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    char expanded[512];  // Can't use kaAlloc until I know the length of the expanded name ...

    if (strcmp(attrP->name, "type") == 0)
      continue;
    if (strcmp(attrP->name, "value") == 0)   // FIXME: Only if "Property"
      continue;
    if (strcmp(attrP->name, "object") == 0)  // FIXME: Only if "Relationship"
      continue;

    //
    // FIXME: ignore also createdAt, modifiedAt, ... ?
    //


    bool  valueToBeExpanded  = false;

    LM_TMP(("NFY: expanding name of attribute '%s'", attrP->name));
    if (orionldUriExpand(orionldState.contextP, attrP->name, expanded, sizeof(expanded), &valueToBeExpanded, detailsP) == false)
      return false;

    attrP->name = kaStrdup(&orionldState.kalloc, expanded);

    //
    // Expand the value, if necessary (if the @context says so)
    //
    if (valueToBeExpanded == true)
      orionldValueExpand(attrP);


    //
    // Expand also sub-attr names
    //
    LM_TMP(("NFY: expanding name of sub-attributes of '%s'", attrP->name));

    for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
    {
      LM_TMP(("NFY: %s", subAttrP->name));
      if (strcmp(subAttrP->name, "type") == 0)
        continue;
      if (strcmp(subAttrP->name, "value") == 0)   // FIXME: Only if "Property"
        continue;
      if (strcmp(subAttrP->name, "object") == 0)  // FIXME: Only if "Relationship"
        continue;

      LM_TMP(("NFY: expanding name of sub-attribute '%s' of '%s'", subAttrP->name, attrP->name));
      if (orionldUriExpand(orionldState.contextP, subAttrP->name, expanded, sizeof(expanded), &valueToBeExpanded, detailsP) == false)
        return false;
      subAttrP->name = kaStrdup(&orionldState.kalloc, expanded);

      LM_TMP(("NFY: expanded name of sub-attribute '%s' of '%s'", subAttrP->name, attrP->name));

      //
      // Expand the value, if ...
      //
      if (valueToBeExpanded == true)
        orionldValueExpand(subAttrP);
    }
  }

  orionldState.notify = true;
  return true;
}



// -----------------------------------------------------------------------------
//
// subscriptionMatchCallback -
//
static bool subscriptionMatchCallback
(
  const char*  entityId,
  KjNode*      subscriptionTree,
  KjNode*      currentEntityTree,
  KjNode*      incomingRequestTree
)
{
  KjNode*  idP               = NULL;
  KjNode*  referenceP        = NULL;
  KjNode*  mimeTypeP         = NULL;
  KjNode*  attrsP            = NULL;
  KjNode*  expirationP       = NULL;
  KjNode*  throttlingP       = NULL;
  int      now               = 0;

  for (KjNode* nodeP = subscriptionTree->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if ((idP == NULL) && (strcmp(nodeP->name, "_id") == 0))
      idP = nodeP;
    else if ((referenceP == NULL) && (strcmp(nodeP->name, "reference") == 0))
      referenceP = nodeP;
    else if ((mimeTypeP == NULL) && (strcmp(nodeP->name, "mimeType") == 0))
      mimeTypeP = nodeP;
    else if ((attrsP == NULL) && (strcmp(nodeP->name, "attrs") == 0))
      attrsP = nodeP;
    else if ((expirationP == NULL) && (strcmp(nodeP->name, "expiration") == 0))
      expirationP = nodeP;
    else if ((throttlingP == NULL) && (strcmp(nodeP->name, "throttling") == 0))
      throttlingP = nodeP;
  }

  if (idP == NULL)
  {
    LM_E(("Unable to find '_id' member of the Subscription"));
    return false;
  }

  if (referenceP == NULL)
  {
    LM_E(("Unable to find 'reference' member of the Subscription '%s'", idP->value.s));
    return false;
  }

#if 0
  if (throttlingP != NULL)
  {
    KjNode*  lastNotificationP = kjLookup(subscriptionTree, "lastNotification");

    if (lastNotificationP != NULL)
    {
      now = time(NULL);

      if (lastNotificationP->value.i + throttlingP->value.i > now)
      {
        LM_TMP(("NFY: No notification to be sent due to throttling (%d). Now: %d, lastNotification: %d", throttlingP->value.i, now, lastNotification));
        return false;
      }
    }
  }
#endif

  if (expirationP != NULL)
  {
    if (now == 0)
      now = time(NULL);

    LM_TMP(("NFY: Subscription expires at: %llu. Right now is: %d", expirationP->value.i, now));

    if (now > expirationP->value.i)
    {
      LM_TMP(("NFY: No notification to be sent due to expired subscription (expires: %llu, now: %llu)", expirationP->value.i, now));
      return false;
    }
  }

  bool allAttributesInNotification = false;
  if ((attrsP == NULL) || (attrsP->value.firstChildP == NULL))
  {
    allAttributesInNotification = true;
    LM_TMP(("NFY: 'attrs' is not present or EMPTY - notification to be done with the entire UPDATE"));
  }
  else
    LM_TMP(("NFY: 'attrs' is present - its value will decide what to include in the notification"));

  //
  // Creating the attribute list that the Notification will be based on
  //
  OrionldNotificationInfo*  niP = &orionldState.notificationInfo[orionldState.notificationRecords];

  orionldState.notificationRecords += 1;  // For next callback

  niP->subscriptionId       = idP->value.s;
  niP->reference            = referenceP->value.s;
  niP->attrsForNotification = NULL;  // The notification is based on this list of attributes

  if ((mimeTypeP != NULL) && (strcmp(mimeTypeP->value.s, "application/ld+json") == 0))
    niP->mimeType = JSONLD;
  else
    niP->mimeType = JSON;

  if (allAttributesInNotification == true)
  {
    //
    // ALL attributes ... simply clone the incoming request - LEAK
    // FIXME: kjClone(&orionldState.kalloc, incomingRequestTree)
    //
    LM_TMP(("NFY2: ALL attributes - Cloning the entire incoming request for later notification (sub: %s)", niP->subscriptionId));
    niP->attrsForNotification = kjClone(incomingRequestTree);
  }
  else
  {
    LM_TMP(("NFY2: Some attributes - picking attributes for later notification (sub: %s)", niP->subscriptionId));
    niP->attrsForNotification = kjObject(orionldState.kjsonP, NULL);  // Invent other Kjson-pointer - this one dies when request ends

    //
    // Instead of looping over the modified attributes (incomingRequestTree) we loop over the attributes that the
    // subscription wants to be included in the Notification.
    //
    // Those attributes that were not updated must be taken from the entity we have already looked up (currentEntityTree)
    //
    // [ What if some attributes don't exist in local but are registered? :-D :-D  INSANE !!! ]
    //
    for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      dotForEq(attrP->value.s);  // Must compare with '=' instead of '.' in attribute name

      KjNode* reqAttrP        = kjLookup(incomingRequestTree, attrP->value.s);  // Look up the attribute in the incoming request, and ...
      KjNode* currentAttrVecP = NULL;

      //
      // ... if the attribute is not found in the incoming request, then it may be found in the original entity
      //
      if (reqAttrP == NULL)
      {
        if (currentAttrVecP == NULL)
        {
          currentAttrVecP = kjLookup(currentEntityTree, "attrs");
          if (currentAttrVecP == NULL)
            LM_X(1, ("What? the entity '%s' has no attrs vector in DB ...", entityId));
        }

        char* attrNameWithEq = kaStrdup(&orionldState.kalloc, attrP->value.s);

        LM_TMP(("DOT: replacing dots for EQs in '%s'", attrNameWithEq));
        dotForEq(attrNameWithEq);
        LM_TMP(("DOT: replaced dots for EQs in '%s'", attrNameWithEq));
        reqAttrP = kjLookup(currentAttrVecP, attrNameWithEq);

        if (reqAttrP == NULL)
        {
          LM_W(("NFY: The attribute '%s' (%s) is nowhere to be found", attrNameWithEq, attrP->value.s));

          //
          // If also not found in the original entity, then it can't be included in the Notification
          //
          return false;
        }
        LM_TMP(("NFY: Found attribute '%s' in Original Entity", attrP->value.s));
      }
      else
        LM_TMP(("NFY: Found attribute '%s' in Incoming Request", attrP->value.s));

      KjNode* aP = kjClone(reqAttrP);

      kjChildAdd(niP->attrsForNotification, aP);
      LM_TMP(("NFY: Added the attribute '%s' to the attribute list on which the notification will be based", aP->name));
    }
  }

  //
  // Lookup aliases for the attributes
  //
  LM_TMP(("NFY2: Lookup aliases for the attributes of sub %s", niP->subscriptionId));
  for (KjNode* aP = niP->attrsForNotification->value.firstChildP; aP != NULL; aP = aP->next)
  {
    //
    // Put back '.' instead of '=' for the attribute name
    //
    LM_TMP(("NFY2: Putting back '.' instead of '=': %s", aP->name));
    eqForDot(aP->name);
    LM_TMP(("NFY2: Put back '.' instead of '=': %s", aP->name));

    //
    // Lookup alias for attribute name in the context
    //
    char* alias = orionldAliasLookup(orionldState.contextP, aP->name, NULL);

    if (alias != NULL)
    {
      LM_TMP(("NFY2: Changing longname '%s' for shortname '%s'", aP->name, alias));
      aP->name = alias;
    }
    else
      LM_TMP(("NFY2: No alias found for longname'%s'", aP->name));
  }

  //
  // Lastly, we must add Entity ID to the tree.
  // This tree will end up being an item in the Notification::data array,
  // which is done in orionldNotify()
  //
  KjNode* entityIdNodeP = kjString(orionldState.kjsonP, "id",   entityId);

  kjChildAdd(niP->attrsForNotification, entityIdNodeP);

  LM_TMP(("NFY: %d notification records created", orionldState.notificationRecords));

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldNotifyForAttrList -
//
bool orionldNotifyForAttrList(const char* entityId, KjNode* currentEntityTree, KjNode* incomingRequestTree)
{
  dbSubscriptionMatchEntityIdAndAttributes(entityId, currentEntityTree, incomingRequestTree, subscriptionMatchCallback);
  return true;
}



// -----------------------------------------------------------------------------
//
// orionldPostEntityOverwrite -
//
bool orionldPostEntityOverwrite(ConnectionInfo* ciP)
{
  LM_TMP(("NFY: In orionldPostEntityOverwrite"));
  //
  // Forwarding and Subscriptions will be taken care of later.
  // For now, just local updates
  //
  // 1. Get entity, as a KjNode tree
  // 2. For each attribute in orionldState.requestTree:
  //    - If found in currentEntityTree, merge both
  //    - If not found - add attribute from orionldState.requestTree to currentEntityTree (also add to "attrNames[]")
  // 3. Write to mongo
  //
  char*   entityId           = orionldState.wildcard[0];
  KjNode* currentEntityTree = dbEntityLookup(entityId);
  char*   title;
  char*   details;

  // Expand attribute names
  if (expandAttrNames(orionldState.requestTree, &details) == false)
  {
    ciP->httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldBadRequestData, "Can't expand attribute names", details);
    return false;
  }

  //
  // Forwarding
  //
  // 1. Decide the type of tree. In this case it is a vector of attributes and the entity id and type are not in the tree
  // 2. Query the "registrations" for this entity id/type and any of the attributes of the tree
  // 3. For each match in "registrations", schedule a REST request to be sent to the IP:PORT of the registration
  //    - it will be the very same request, i.e. POST /entities/EID/attrs and with, I think, the exact same URI params
  // 4. Each response must be recorded for the response, in case some attribute has bnot been updated - 207 Multi Status
  //

  //
  // Subscriptions
  //
  // Notifications are only prepared here.
  // The actual sending of notifications is done after the response has been sent,
  // in rest.cpp, function requestCompleted().
  //

  LM_TMP(("DOT: Calling orionldNotifyForAttrList where EQs are needed"));
  orionldNotifyForAttrList(entityId, currentEntityTree, orionldState.requestTree);

  if (currentEntityTree == NULL)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", orionldState.wildcard[0]);
    return false;
  }

  // Merge orionldState.requestTree with currentEntityTree
  if (kjTreeMergeAddNewAttrsOverwriteExisting(currentEntityTree, orionldState.requestTree, &title, &details) == false)
  {
    ciP->httpStatusCode = SccReceiverInternalError;
    orionldErrorResponseCreate(OrionldInternalError, title, details);
    return false;
  }


  //
  // Set the modification date of the entity
  //
  kjModDateSet(currentEntityTree);

  // Write to database
  dbEntityUpdate(entityId, currentEntityTree);

  //
  // All OK - set HTTP STatus Code
  //
  ciP->httpStatusCode = SccNoContent;

  return true;
}



// ----------------------------------------------------------------------------
//
// orionldPostEntity -
//
// POST /ngsi-ld/v1/entities/*/attrs
//
// URI PARAMETERS
//   options=noOverwrite
//
// From ETSI spec:
//   Behaviour
//   * If the Entity Id is not present or it is not a valid URI then an error of type BadRequestData shall be raised.
//   * If the NGSI-LD endpoint does not know about this Entity, because there is no an existing Entity which id
//     (URI) is equivalent to the one passed as parameter, an error of type ResourceNotFound shall be raised.
//   * For each Attribute (Property or Relationship) included by the Entity Fragment at root level:
//     * If the target Entity does not include a matching Attribute then such Attribute shall be appended to the target Entity
//     * If the target Entity already includes a matching Attribute
//       - If no datasetId is present in the Attribute included by the Entity Fragment:
//         * If options==noOverwrite: the existing Attribute in the target Entity shall be left untouched
//         * If options!=noOverwrite: the existing Attribute in the target Entity shall be replaced by the new one supplied.
//
//
// If options=noOverwrite is set, then we can simply use updateActionType == ActionTypeAppendStrict
//
// If options=noOverwrite is NOT set, then we have no matching already existing function.
// Tries have been made to modify mongoBackend but without success - see issue https://github.com/FIWARE/context.Orion-LD/issues/153
//
// Instead we will REIMPLEMENT the whole DB-part, using the newest C driver for mongodb
//
bool orionldPostEntity(ConnectionInfo* ciP)
{
  LM_TMP(("NFY: In orionldPostEntity"));

  // Is the payload not a JSON object?
  OBJECT_CHECK(orionldState.requestTree, kjValueType(orionldState.requestTree->type));

  if (orionldState.uriParamOptions.noOverwrite == false)
    return orionldPostEntityOverwrite(ciP);

  LM_TMP(("NFY: still in orionldPostEntity"));
  // 1. Check that the entity exists
  if (mongoEntityExists(orionldState.wildcard[0], orionldState.tenant) == false)
  {
    ciP->httpStatusCode = SccNotFound;
    orionldErrorResponseCreate(OrionldBadRequestData, "Entity does not exist", orionldState.wildcard[0]);
    return false;
  }

  UpdateContextRequest   mongoRequest;
  UpdateContextResponse  mongoResponse;
  ContextElement*        ceP       = new ContextElement();  // FIXME: Any way I can avoid to allocate ?
  EntityId*              entityIdP;

  mongoRequest.updateActionType = ActionTypeAppendStrict;

  mongoRequest.contextElementVector.push_back(ceP);
  entityIdP     = &mongoRequest.contextElementVector[0]->entityId;
  entityIdP->id = orionldState.wildcard[0];



  if (kjTreeToContextElement(ciP, orionldState.requestTree, ceP) == false)
  {
    // kjTreeToContextElement fills in error using 'orionldErrorResponseCreate()'
    mongoRequest.release();
    LM_E(("kjTreeToContextElement failed"));
    return false;
  }

  //
  // This service may return a payload, to indicate which attributes have been appended/changed
  // Also, its returned HTTP Status Code depends on the appended/changed attributes.
  // So. we need mongoBackend to give us that information back, in a way that can easily be used.
  // A simple string will be used, with the following design:
  //
  //   "|attrName|attrName|attrName|attrName|"
  //
  // HTTP Status Codes:
  //  * 204: All attributes in the payload were successfully appended (or overwritten)
  //  * 207: Only the Attributes included in the response payload were successfully appended.
  //  * 400: The request or its content is incorrect
  //  * 404: Entity not found
  //
  // In the case "207" - some of the attributes weren't successfully updated, the list of all
  // successfully updated attributes in returned as payload.
  // This list is a vector of attribute names.
  //
  // For better performance, the names of the erroneous attributes are recored in a string.
  // After "mongoBackend processing" this erroneous attributes string is checked and if it is not empty,
  // then an inverted array must be created for the response payload.
  // Normally, all will work just fine so this array will be empty.
  //
  // The variable to hold this "error attribute array" is:
  //   orionldState.errorAttributeArray.
  //
  // The function to use to insert the attributes is:
  //   orionldStateErrorAttributeAdd(char* attributeName)
  //


  //
  // Call mongoBackend
  //
  ciP->httpStatusCode = mongoUpdateContext(&mongoRequest,
                                           &mongoResponse,
                                           orionldState.tenant,
                                           ciP->servicePathV,
                                           ciP->uriParam,
                                           ciP->httpHeaders.xauthToken,
                                           ciP->httpHeaders.correlator,
                                           ciP->httpHeaders.ngsiv2AttrsFormat,
                                           ciP->apiVersion,
                                           NGSIV2_NO_FLAVOUR);

  //
  // Now check orionldState.errorAttributeArray to see whether any attribute failed to be updated
  //
  bool partialUpdate = (orionldState.errorAttributeArrayP[0] == 0)? false : true;
  bool retValue      = true;

  if (ciP->httpStatusCode == SccOk)
    ciP->httpStatusCode = SccNoContent;
  else
  {
    if (partialUpdate == true)
    {
      orionldPartialUpdateResponseCreate(ciP);
      ciP->httpStatusCode = (HttpStatusCode) 207;
    }
    else
    {
      LM_E(("mongoUpdateContext: HTTP Status Code: %d", ciP->httpStatusCode));
      orionldErrorResponseCreate(OrionldBadRequestData, "Internal Error", "Error from Mongo-DB Backend");
    }

    retValue = false;
  }

  mongoRequest.release();
  mongoResponse.release();

  return retValue;
}
