/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Felipe Ortiz
*/

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
  lws_context *ctx;
  pthread_t th;
};

// Private struct for persistent data
typedef struct
{
  unsigned cid;
  char *message;
  char *request;
  int index;
}data;

static int wsCallback(lws * ws,
                      enum lws_callback_reasons reason,
                      void *user,
                      void *in,
                      size_t len)
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
      dat->request = NULL;
      dat->index = 0;
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
      lws_write(ws, p, strlen(dat->message), LWS_WRITE_TEXT);
      free(buff);
      free(dat->message);
      break;
    }

    case LWS_CALLBACK_RECEIVE:
    {
      const size_t remaining = lws_remaining_packet_payload(ws);
      const size_t msg_size = strlen(message);

      if (!dat->request)
        dat->request = (char *) malloc (msg_size + remaining);

      memcpy(&dat->request[dat->index], message, msg_size);
      dat->index += msg_size;

      if (!remaining && lws_is_final_fragment(ws))
      {
        dat->request[dat->index] = 0;

        ConnectionInfo *ci = connection_manager_get(dat->cid, dat->request);
        dat->message = strdup((restService(ci, orionServices)).c_str());
        free(dat->request);
        dat->request = NULL;
        dat->index = 0;
        lws_callback_on_writable(ws);
      }
      break;
    }
    default:
      break;
  }
  return 0;
}

static struct lws_protocols protocols[] = {
    {
        "orion-ws",
        wsCallback,
        sizeof(data),
        128,
        1,
        NULL
    },
    {
        NULL, NULL, 0, 0, 0, 0
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
    lws_service(ws->ctx, 50);
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
    lws_get_internal_extensions(),
    0,0,0,0,0,0,0,0,0,0
  };

  orion_websocket *ws = (orion_websocket *) malloc(sizeof(orion_websocket));
  ws->end = 0;
  ws->ctx = lws_create_context(&info);

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
