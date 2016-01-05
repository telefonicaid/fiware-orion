#ifndef ORION_WEBSOCKET_H
#define ORION_WEBSOCKET_H

struct _orion_websocket;
typedef struct _orion_websocket orion_websocket;

struct RestService;

orion_websocket *orion_websocket_new(struct RestService *serv);
int orion_websocket_start(orion_websocket *ws);
void orion_websocket_stop(orion_websocket *ws);

#endif
