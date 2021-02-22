#ifndef SRC_LIB_MONGODRIVER_WRITECONCERN_H_
#define SRC_LIB_MONGODRIVER_WRITECONCERN_H_

/*
*
* Copyright 2021 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/

namespace orion
{
/* ****************************************************************************
*
* WriteConcern -
*
* By the moment we only take into account UnAcknowledged (w:0) y Acknowledged (w:1)
* modes. MongoDB supporte some others (w:majority, etc.) and we will expand this
* enum in the moment we need it
*/
enum WriteConcern
{
  WCUnAcknowledged,
  WCAcknowledged
};
}

#endif  // SRC_LIB_MONGODRIVER_WRITECONCERN_H_


