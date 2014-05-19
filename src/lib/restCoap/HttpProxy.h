#ifndef HTTPPROXY_H
#define HTTPPROXY_H

#include "cantcoap.h"
#include "HttpMessage.h"

struct MemoryStruct {
  char   *memory;
  size_t  size;
};

static const int  COAP_BUFFER_SIZE     = 1024;
static const int  COAP_URI_BUFFER_SIZE = 100;

extern std::string  sendHttpRequest(char *host, unsigned short port, CoapPDU * request);
extern void         http2Coap(MemoryStruct *http, CoapPDU *coap);


#endif // HTTPPROXY_H
