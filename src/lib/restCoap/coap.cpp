/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: TID Developer
*/

#include "coap.h"

#include <boost/thread.hpp>

#include "restCoap/CoapController.h"


/* ****************************************************************************
*
* run -
*
* Prepares a new thread to serve CoAP requests
*
*/
int Coap::run(const char *_host, unsigned short _httpPort, unsigned short _coapPort)
{
  boost::thread *coapServerThread = new boost::thread(boost::bind(&CoapController::serve, new CoapController(_host, _httpPort, _coapPort)));
  coapServerThread->get_id(); // to prevent 'warning: unused'
  return 0;
}

