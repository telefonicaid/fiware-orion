#ifndef CANTCOAP_H
#define CANTCOAP_H

#include <vector>
#include "cantcoap.h"
#include "uthash.h"
// for mbed compatibility
//#define failGracefully exit

class Coap
{
  private:

    typedef int (*ResourceCallback)(CoapPDU *pdu, int sockfd, struct sockaddr_storage *recvFrom);

    // resource URIs here
    //const char *gURIA = "/test";

//    const char *gURIList[] = {
//      "/test"
//    };

    // URIs mapped to callback functions here
//    ResourceCallback gCallbacks[] = {
//      gTestCallback
//    };

//    std::vector<ResourceCallback> gCallbacks;

    static const int gNumResources = 1;



    // using uthash for the URI hash table. Each entry contains a callback handler.
    struct URIHashEntry {
        const char *uri;
        ResourceCallback callback;
        int id;
        UT_hash_handle hh;
    };

  public:
    Coap()
    {
//      gCallbacks.push_back(gTestCallback);
    }

    // callback functions defined here
    int gTestCallback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);

    int run(int argc, char **argv);
};

#endif // CANTCOAP_H

