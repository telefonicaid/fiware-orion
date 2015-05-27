#include "curl_context.h"

#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"


static pthread_mutex_t contexts_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::map<std::string, struct curl_context> contexts;

struct curl_context
get_curl_context(const std::string& url)
{
  struct curl_context cc;
  int s = pthread_mutex_lock(&contexts_mutex);
  if(s!=0)
  {
       LM_X(1,("pthread_mutex_lock"));
  }
  std::map<std::string, struct curl_context>::iterator it;
  it = contexts.find(url);
  if (it==contexts.end())
  {
      //not found, create it
      cc.curl = curl_easy_init();
      pthread_mutex_t *pm = (pthread_mutex_t *) malloc(sizeof(*pm));
      if (pm==NULL)
      {
          LM_X(1,("malloc"));
      }
      int s = pthread_mutex_init(pm, NULL);
      if(s!=0)
      {
           LM_X(1,("pthread_mutex_init"));
      }
      cc.pmutex = pm;
      if (cc.curl != NULL)
      {
        contexts[url] = cc;
      }
  }
  else
  {
      cc = it->second;
  }
  s = pthread_mutex_unlock(&contexts_mutex);
  if(s!=0)
  {
    LM_X(1,("pthread_mutex_unlock"));
  }
  s = pthread_mutex_lock(cc.pmutex);
  if(s!=0)
  {
      LM_X(1,("pthread_mutex_lock"));
  }
  return cc;
}


void
release_curl_context(struct curl_context cc)
{
  int s = pthread_mutex_unlock(cc.pmutex);
  if(s!=0)
  {
      LM_X(1,("pthread_mutex_unlock"));
  }
  if (cc.curl != NULL)
  {
    curl_easy_reset(cc.curl);
  }
}
