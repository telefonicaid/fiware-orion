#ifndef CURL_CONTEXT_H
#define CURL_CONTEXT_H

#include <pthread.h>

#include <string>

#include <curl/curl.h>



struct curl_context {
  CURL *curl;
  pthread_mutex_t *pmutex;
};

struct curl_context
get_curl_context(const std::string& key);

void
release_curl_context(struct curl_context cc);



#endif // CURL_CONTEXT_H
