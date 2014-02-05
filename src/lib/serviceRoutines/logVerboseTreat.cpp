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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "ngsi/ParseData.h"
#include "rest/ConnectionInfo.h"
#include "rest/orionReply.h"
#include "serviceRoutines/logVerboseTreat.h"



/* ****************************************************************************
*
* logVerboseTreat - 
*/
std::string logVerboseTreat(ConnectionInfo* ciP, int components, std::vector<std::string> compV, ParseData* parseDataP)
{
  std::string out;
  if ((components == 2) && (ciP->method == "GET"))
  {
    char* verboseLevel = (char*) "0";

    if      (lmVerbose5 == true)  verboseLevel = (char*) "5";
    else if (lmVerbose4 == true)  verboseLevel = (char*) "4";
    else if (lmVerbose3 == true)  verboseLevel = (char*) "3";
    else if (lmVerbose2 == true)  verboseLevel = (char*) "2";
    else if (lmVerbose  == true)  verboseLevel = (char*) "1";

    out = orionReply(ciP, "verboseLevel", verboseLevel);
   }
   else if ((components == 2) && (ciP->method == "DELETE"))
   {
     lmVerbose  = false;
     lmVerbose2 = false;
     lmVerbose3 = false;
     lmVerbose4 = false;
     lmVerbose5 = false;

     out = orionReply(ciP, "verboseLevel", "all verbose levels reset");
   }
   else if ((components == 3) && (ciP->method == "PUT") && (compV[2] == "off"))
   {
     lmVerbose  = false;
     lmVerbose2 = false;
     lmVerbose3 = false;
     lmVerbose4 = false;
     lmVerbose5 = false;

     out = orionReply(ciP, "verboseLevel", "all verbose levels reset");
   }
   else if ((components == 3) && (ciP->method == "PUT") && (compV[2] == "0"))
   {
     lmVerbose  = false;
     lmVerbose2 = false;
     lmVerbose3 = false;
     lmVerbose4 = false;
     lmVerbose5 = false;

     out = orionReply(ciP, "verboseLevel", "all verbose levels reset");
   }
   else if ((components == 3) && (ciP->method == "PUT") && (compV[2] == "1"))
   {
     lmVerbose  = true;
     lmVerbose2 = false;
     lmVerbose3 = false;
     lmVerbose4 = false;
     lmVerbose5 = false;

     out = orionReply(ciP, "verboseLevel", "verbose level set to 1");
   }
   else if ((components == 3) && (ciP->method == "PUT") && (compV[2] == "2"))
   {
     lmVerbose  = true;
     lmVerbose2 = true;
     lmVerbose3 = false;
     lmVerbose4 = false;
     lmVerbose5 = false;

     out = orionReply(ciP, "verboseLevel", "verbose level set to 2");
   }
   else if ((components == 3) && (ciP->method == "PUT") && (compV[2] == "3"))
   {
     lmVerbose  = true;
     lmVerbose2 = true;
     lmVerbose3 = true;
     lmVerbose4 = false;
     lmVerbose5 = false;

     out = orionReply(ciP, "verboseLevel", "verbose level set to 3");
   }
   else if ((components == 3) && (ciP->method == "PUT") && (compV[2] == "4"))
   {
     lmVerbose  = true;
     lmVerbose2 = true;
     lmVerbose3 = true;
     lmVerbose4 = true;
     lmVerbose5 = false;

     out = orionReply(ciP, "verboseLevel", "verbose level set to 4");
   }
   else if ((components == 3) && (ciP->method == "PUT") && (compV[2] == "5"))
   {
     lmVerbose  = true;
     lmVerbose2 = true;
     lmVerbose3 = true;
     lmVerbose4 = true;
     lmVerbose5 = true;

     out = orionReply(ciP, "verboseLevel", "verbose level set to 5");
   }
   else
     out = orionReply(ciP, "verboseLevel", "bad URL or Verb");

   return out;
}
