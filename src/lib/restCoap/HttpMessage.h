#ifndef HTTPMESSAGE_H
#define HTTPMESSAGE_H

#include <string>
#include "cantcoap.h"

struct MemoryStruct {
  char   *memory;
  size_t  size;
};

class HttpMessage
{
    std::string theMessage;

    int httpCode;
    int contentLength;
    std::string contentType;
    std::string body;

  public:
    HttpMessage(MemoryStruct *data);

    CoapPDU* toCoap();
};

#endif // HTTPMESSAGE_H
