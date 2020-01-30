/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "mongoDriver/DBConnection.h"

namespace orion
{
/* ****************************************************************************
*
* DBConnection::DBConnection -
*/
DBConnection::DBConnection()
{
  dbcP = NULL;
}



/* ****************************************************************************
*
* DBConnection::operator== -
*/
bool DBConnection::operator== (const DBConnection &rhs)
{
  return dbcP == rhs.dbcP;
}



/* ****************************************************************************
*
* DBConnection::isNull
*/
bool DBConnection::isNull(void)
{
  return (dbcP == NULL);
}


///////// from now on, only methods with low-level driver types in return or parameters /////////



/* ****************************************************************************
*
* DBConnection::DBConnection -
*/
DBConnection::DBConnection(mongo::DBClientBase* _dbcP)
{
  dbcP = _dbcP;
}



/* ****************************************************************************
*
* DBConnection::get -
*/

mongo::DBClientBase* DBConnection::get(void)
{
  return dbcP;
}
}
