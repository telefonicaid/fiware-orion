#ifndef COAP_H
#define COAP_H

#include <sys/socket.h>
#include "cantcoap.h"

struct MemoryStruct {
  char   *memory;
  size_t  size;
};

class Coap
{
  private:

    static const int  BUFFER_SIZE     = 500;
    static const int  URI_BUFFER_SIZE = 100;
    char             *host;
    char             *portString;
    unsigned short    port;

    //size_t  writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
    void  sendHttpRequest(CoapPDU * request, MemoryStruct* chunk);
    int   callback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);
    void  serve();
    void  printAddressStructures(struct addrinfo *addr);
    void  printAddress(struct addrinfo *addr);
    void  http2Coap(MemoryStruct *http, CoapPDU *coap);
  public:
    int   run(const char *_host, unsigned short _port);
};

#endif // COAP_H

