#ifndef SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXT_H_
#define SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXT_H_

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
extern "C"
{
#include "kjson/kjson.h"
}



// ----------------------------------------------------------------------------
//
// OrionldContextType
//
typedef enum OrionldContextType
{
  OrionldNoContext = 0,
  OrionldCoreContext,
  OrionldDefaultUrlContext,
  OrionldDefaultContext,
  OrionldUserContext
} OrionldContextType;



// ----------------------------------------------------------------------------
//
// OrionldContext
//
typedef struct OrionldContext
{
  char*                   url;
  char*                   name;
  KjNode*                 tree;
  OrionldContextType      type;
  bool                    ignore;     // Core/Default URL Context inside USER contexts
  bool                    temporary;  // true by default, false when inserted in context list
  struct OrionldContext*  next;
} OrionldContext;

#endif  // SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXT_H_
