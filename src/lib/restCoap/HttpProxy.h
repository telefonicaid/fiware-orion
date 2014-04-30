#ifndef HTTPPROXY_H
#define HTTPPROXY_H

#include "cantcoap.h"

static const int  BUFFER_SIZE     = 500;
static const int  URI_BUFFER_SIZE = 100;

struct MemoryStruct {
  char   *memory;
  size_t  size;
};

extern void sendHttpRequest(char *host, unsigned short port, CoapPDU * request, MemoryStruct* chunk);
extern void http2Coap(MemoryStruct *http, CoapPDU *coap);


#endif // HTTPPROXY_H
