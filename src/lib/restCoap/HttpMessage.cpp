#include "HttpMessage.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

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
        _httpCode = atoi(line.substr(9, 3).c_str());
      }
      else
      {
        // Get other headers
        int temp = line.find("Content-Length");
        if (temp >= 0)
        {
          _contentLength = atoi(line.substr(pos + 2).c_str());
        }

        temp = line.find("Content-Type");
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
  uint8_t* data = (uint8_t*)this->_body.c_str();
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
