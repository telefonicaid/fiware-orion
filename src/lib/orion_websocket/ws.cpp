#include "ws.h"
#include "connection_manager.h"

#include "rest/RestService.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/RestService.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <pthread.h>

#include <libwebsockets.h>

RestService *orionServices;

struct _orion_websocket
{
  volatile int end;
  libwebsocket_context *ctx;
  pthread_t th;
};

// Private struct for persistent data
typedef struct
{
  unsigned cid;
  char *message;
}data;

static int wscllbck(libwebsocket_context *ctx,
                    libwebsocket *ws,
                    libwebsocket_callback_reasons reason,
                    void *user, void *in, size_t len)
{
  data *dat = (data *) user;
  char *message = (char *)in;

  switch (reason)
  {
    case LWS_CALLBACK_ESTABLISHED:
    {
      int cid = connection_manager_get_cid();

      if (cid == -1)
      {
        LM_E(("No more cid available!"));
        break;
      }

      dat->cid = cid;
      LM_I(("Connecction id: %d", dat->cid));
      break;
    }
    case LWS_CALLBACK_CLOSED:
    {
      connection_manager_remove(dat->cid);
      dat->cid = -1;
      break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
      size_t size = strlen(dat->message) +
                    LWS_SEND_BUFFER_PRE_PADDING +
                    LWS_SEND_BUFFER_POST_PADDING;

      unsigned char *buff = (unsigned char *) malloc(size);
      unsigned char *p = &buff[LWS_SEND_BUFFER_PRE_PADDING];

      sprintf((char*)p, "%s", dat->message);
      libwebsocket_write(ws, p, strlen(dat->message), LWS_WRITE_TEXT);
      free(buff);
      free(dat->message);
      break;
    }

    case LWS_CALLBACK_RECEIVE:
    {
      ConnectionInfo *ci = connection_manager_get(dat->cid, message);
      dat->message = strdup((restService(ci, orionServices)).c_str());
      libwebsocket_callback_on_writable(ctx, ws);
      break;
    }
    default:
      break;
  }
  return 0;
}

static struct libwebsocket_protocols protocols[] = {
    {
        "orion-ws",
        wscllbck,
        sizeof(data),
        128
    },
    {
        NULL, NULL, 0
    }
};

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void *runWS(void *ptr)
{
  int end;

  orion_websocket *ws = (orion_websocket *)ptr;

  pthread_mutex_lock(&mtx);
  end = ws->end;
  pthread_mutex_unlock(&mtx);

  while (!end)
  {
    pthread_mutex_lock(&mtx);
    end = ws->end;
    pthread_mutex_unlock(&mtx);

    pthread_mutex_lock(&mtx);
    libwebsocket_service(ws->ctx, 50);
    pthread_mutex_unlock(&mtx);
  }
  return 0;
}

orion_websocket *orion_websocket_new(RestService *serv)
{
  struct lws_context_creation_info info = {
    9010,
    NULL,
    protocols,
    libwebsocket_get_internal_extensions(),
    0,0,0,0,0,0,0,0,0,0
  };

  orion_websocket *ws = (orion_websocket *) malloc(sizeof(orion_websocket));
  ws->end = 0;
  ws->ctx = libwebsocket_create_context(&info);

  if (!ws->ctx)
  {
    free (ws);
    return NULL;
  }
  orionServices = serv;
  return ws;
}

int orion_websocket_start(orion_websocket *ws)
{
  int err = pthread_create(&ws->th, NULL, runWS, (void *)ws);
  return err == 0;
}

void orion_websocket_stop(orion_websocket *ws)
{
  pthread_mutex_lock(&mtx);
  ws->end = 1;
  pthread_mutex_unlock(&mtx);
}
