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
* iot_support at tid dot es
*
* Author: TID Developer
*/

#include "proxyCoap/HttpMessage.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "rest/HttpHeaders.h"

#include <stdlib.h>  // atoi
#include <sstream>   // istringstream

HttpMessage::HttpMessage(std::string theMessage)
{
  _httpCode = 0;
  _contentLength = 0;
  _contentType = "";
  _body = "";

  std::istringstream iss(theMessage);
  std::string line;

  bool isHeader = true;
  while (std::getline(iss, line))
  {
    // Clean last char in headers
    if (isHeader)
    {
      line = line.substr(0, line.size() - 1);
    }

    if (line.empty())
    {
      isHeader = false;
    }

    if (isHeader)
    {
      // Is it a header or the first line with the HTTP code?
      int pos = line.find(":");
      if (pos < 0)
      {
        // Get HTTP code
        // HTTP code is always 3 chars long starting at position 9
        //
        //   HTTP/1.1 200 OK
        //            XXX
        //
        _httpCode = atoi(line.substr(9, 3).c_str());
      }
      else
      {
        // Get other headers
        int temp = line.find(HTTP_CONTENT_LENGTH);
        if (temp >= 0)
        {
          _contentLength = atoi(line.substr(pos + 2).c_str());
        }

        temp = line.find(HTTP_CONTENT_TYPE);
        if (temp >= 0)
        {
          _contentType = line.substr(pos + 2);
        }
      }
    }
    else
    {
      // We are parsing the body now (as is)
      _body += line;
    }
  }
}

CoapPDU* HttpMessage::toCoap()
{
  CoapPDU* pdu = new CoapPDU();

  // Set code
  switch (this->_httpCode)
  {
    case 200:
      pdu->setCode(CoapPDU::COAP_CONTENT);
      break;

    case 415:
      pdu->setCode(CoapPDU::COAP_UNSUPPORTED_CONTENT_FORMAT);
      break;

    default:
      pdu->httpStatusToCode(this->_httpCode);
      break;
  }

  // Set payload
  uint8_t* data = (uint8_t*) this->_body.c_str();
  std::size_t length = this->_body.length();
  pdu->setPayload(data, length);

  // Set content-type
  if (this->_contentType == "application/json")
  {
    pdu->setContentFormat(CoapPDU::COAP_CONTENT_FORMAT_APP_JSON);
  }
  else
  {
    pdu->setContentFormat(CoapPDU::COAP_CONTENT_FORMAT_APP_XML);
  }

  return pdu;
}
