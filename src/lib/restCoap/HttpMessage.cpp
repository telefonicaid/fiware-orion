#include "HttpMessage.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include <stdlib.h>  // atoi
#include <sstream>   // istringstream

HttpMessage::HttpMessage(std::string theMessage)
{
  httpCode = 0;
  contentLength = 0;
  contentType = "";
  body = "";

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
        httpCode = atoi(line.substr(9, 3).c_str());
      }
      else
      {
        // Get other headers
        int temp = line.find("Content-Length");
        if (temp >= 0)
        {
          contentLength = atoi(line.substr(pos + 2).c_str());
        }

        temp = line.find("Content-Type");
        if (temp >= 0)
        {
          contentType = line.substr(pos + 2);
        }
      }
    }
    else
    {
      // We are parsing the body now (as is)
      body += line;
    }
  }
}

CoapPDU* HttpMessage::toCoap()
{
  CoapPDU* pdu = new CoapPDU();

  // Set code
  if (this->httpCode == 200)
  {
    pdu->setCode(CoapPDU::COAP_VALID);
  }
  else
  {
    pdu->httpStatusToCode(this->httpCode);
  }

  // Set payload
  uint8_t* data = (uint8_t*)this->body.c_str();
  std::size_t length = this->body.length();
  pdu->setPayload(data, length);

  // Set content-type
  if (this->contentType == "application/json")
  {
    pdu->setContentFormat(CoapPDU::COAP_CONTENT_FORMAT_APP_JSON);
  }
  else
  {
    pdu->setContentFormat(CoapPDU::COAP_CONTENT_FORMAT_APP_XML);
  }

  return pdu;
}
