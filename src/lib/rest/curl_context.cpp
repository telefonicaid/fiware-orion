#include "curl_context.h"

#include <map>

#include <assert.h>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"


static pthread_mutex_t contexts_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::map<std::string, struct curl_context> contexts;

struct curl_context
get_curl_context(const std::string& key)
{
  struct curl_context cc = {NULL, NULL};
  int s = pthread_mutex_lock(&contexts_mutex);
  if(s!=0)
  {
       LM_X(1,("pthread_mutex_lock"));
  }
  std::map<std::string, struct curl_context>::iterator it;
  it = contexts.find(key);
  if (it==contexts.end())
  {
      //not found, create it
      cc.curl = curl_easy_init();
      if (cc.curl != NULL)
      {
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
        contexts[key] = cc;
      }
      else  // curl_easy returned null
      {
       cc.pmutex = NULL;    //unnecessary but clearer
      }
  }
  else //previuos context found
  {
      cc = it->second;
  }
  s = pthread_mutex_unlock(&contexts_mutex);
  if(s!=0)
  {
    LM_X(1,("pthread_mutex_unlock"));
  }

  // lock the mutex, if everything was right
  // and cc is not {NULL, NULl}
  if (cc.pmutex != NULL) {
    s = pthread_mutex_lock(cc.pmutex);
    if(s!=0)
    {
      LM_X(1,("pthread_mutex_lock"));
    }
  }

  assert((cc.curl != NULL && cc.pmutex != NULL)
          || (cc.curl == NULL && cc.pmutex == NULL));
  return cc;
}


void
release_curl_context(struct curl_context cc)
{
  assert((cc.curl != NULL && cc.pmutex != NULL)
          || (cc.curl == NULL && cc.pmutex == NULL));

  // Unlock the mutex if not an empty context
  if (cc.pmutex != NULL) {
    int s = pthread_mutex_unlock(cc.pmutex);
    if(s!=0)
    {
      LM_X(1,("pthread_mutex_unlock"));
    }
  }
  // Reset context if not an empty context
  if (cc.curl != NULL)
  {
    curl_easy_reset(cc.curl);
  }
}
