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
#include "constants.h"
#include "parser.h"
#include "wsNotify.h"

#include "rest/RestService.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "rest/RestService.h"

#include "common/limits.h"

#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UnsubscribeContextResponse.h"
#include "mongoBackend/mongoUnsubscribeContext.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>

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
  // SubId
  std::vector<std::string> notify;
  // Teanat for each subId
  std::vector<std::string> tenant;
  char *message;
  char *request;
  int index;
}data;

static bool isSubscription
(
 const std::vector<std::string>& headName,
 const std::vector<std::string>& headValue,
 std::string& subId
)
{
  for (unsigned i = 0; i < headName.size(); ++i)
  {
    if (headName[i] != "Location")
      continue;

    size_t pos = headValue[i].find("subscriptions");
    if (pos == std::string::npos)
      return false;

    pos = headValue[i].find_last_of('/');
    if (pos == std::string::npos)
      return false;

    char buff[MAX_LENGTH_SUBID + 1];
    headValue[i].copy(buff, MAX_LENGTH_SUBID, pos + 1);
    buff[MAX_LENGTH_SUBID] = 0;
    subId = std::string(buff);
    return true;
  }
  return false;
}

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
      dat->notify.clear();
      dat->tenant.clear();
      dat->request = NULL;
      dat->index = 0;
      break;
    }
    case LWS_CALLBACK_CLOSED:
    {
      removeSenders(dat->notify);

      // Remove all subscriptions from DB
      for (unsigned i = 0; i < dat->notify.size(); ++i)
      {
        UnsubscribeContextRequest req;
        UnsubscribeContextResponse rsp;
        req.subscriptionId.set(dat->notify[i]);
        mongoUnsubscribeContext(&req, &rsp, dat->tenant[i]);
      }

      break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE:
    {
      int msg_size = strlen(dat->message);
      size_t size = msg_size +
                    LWS_SEND_BUFFER_PRE_PADDING +
                    LWS_SEND_BUFFER_POST_PADDING;

      unsigned char *buff = (unsigned char *) malloc(size);
      unsigned char *p = &buff[LWS_SEND_BUFFER_PRE_PADDING];
      sprintf((char*)p, "%s", dat->message);

      int written = 0;
      int bytes = 0;

      while (written < msg_size)
      {
        bytes = lws_write(ws, p + written, msg_size - bytes, LWS_WRITE_TEXT);
        if (bytes == -1)
        {
          LM_E(("Runtime Error (cannot send response through websocket channel)"));
        }
        written += bytes;
      }
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

        ConnectionInfo *ci = new ConnectionInfo("v2", JSON, true);
        std::string url;
        std::string verb;
        std::string payload;
        ws_parser_parse(dat->request, ci, url, verb, payload, ci->httpHeaders);
        ci->modify(url, verb, payload);

        const char *restMsg = restService(ci, orionServices).c_str();
        dat->message = strdup(ws_parser_message(restMsg, ci->httpHeaders, ci->httpHeader, ci->httpHeaderValue, (int)ci->httpStatusCode));

        std::string subId;
        if (isSubscription(ci->httpHeader, ci->httpHeaderValue, subId))
        {
          addSender(subId, ws);
          dat->notify.push_back(subId);
          dat->tenant.push_back(ci->tenant);
        }

        delete ci;
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
    lws_service(ws->ctx, WSConstants::Pooling);
    pthread_mutex_unlock(&mtx);
  }
  return 0;
}

orion_websocket *orion_websocket_new(RestService *serv)
{
  static struct lws_protocols protocols[] = {
    {
      WSConstants::ProtocolName.c_str(),
      wsCallback,
      sizeof(data),
      WSConstants::DataSize,
      1,
      NULL
    },
    {
      NULL, NULL, 0, 0, 0, 0
    }
  };

  struct lws_context_creation_info info = {
    WSConstants::Port,
    NULL,
    protocols,
    lws_get_internal_extensions(),
    0,0,0,0,0,0,0,0,0,0
  };

  lws_set_log_level(0, NULL);
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
