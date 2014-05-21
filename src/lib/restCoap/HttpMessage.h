#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#include <string>
#include "cantcoap.h"

class HttpMessage
{
    int _httpCode;
    int _contentLength;
    std::string _contentType;
    std::string _body;

  public:
    HttpMessage(std::string theMessage);

    CoapPDU* toCoap();
    int contentLength() { return _contentLength; }
};

#endif // HTTPMESSAGE_H
