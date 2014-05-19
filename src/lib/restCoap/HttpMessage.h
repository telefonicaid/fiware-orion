#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#include <string>
#include "cantcoap.h"

class HttpMessage
{
    std::string theMessage;

    int httpCode;
    int contentLength;
    std::string contentType;
    std::string body;

  public:
    HttpMessage(std::string theMessage);

    CoapPDU* toCoap();
};

#endif // HTTPMESSAGE_H
