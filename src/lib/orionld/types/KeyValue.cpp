/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <string.h>                                     // strcmp
#include <stdlib.h>                                     // malloc
#include <vector>                                       // std::vector

#include "logMsg/logMsg.h"                              // LM_*

#include "orionld/types/KeyValue.h"                     // KeyValue



// -----------------------------------------------------------------------------
//
// keyValueLookup -
//
KeyValue* keyValueLookup(const std::vector<KeyValue*>& array, const char* keyName)
{
  for (int ix = 0; ix < (int) array.size(); ix++)
  {
    KeyValue* kvP = array[ix];

    if (strcmp(keyName, kvP->key) == 0)
      return kvP;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// keyValueAdd -
//
KeyValue* keyValueAdd(std::vector<KeyValue*>* array, const char* key, const char* value)
{
  KeyValue* kvP = (KeyValue*) malloc(sizeof(KeyValue));

  if (kvP != NULL)
  {
    strncpy(kvP->key,   key,   sizeof(kvP->key) - 1);
    strncpy(kvP->value, value, sizeof(kvP->value) - 1);

    LM_TMP(("VE: Adding Key-Value Pair %p: %s:%s", kvP->key, kvP->value));
    array->push_back(kvP);
  }

  return kvP;
}
