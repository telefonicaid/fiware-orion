#ifndef WS_CONNECTION_MANAGER_H
#define WS_CONNECTION_MANAGER_H

class ConnectionInfo;

int connection_manager_get_cid();
ConnectionInfo *connection_manager_get(int cid, const char *msg);
void connection_manager_remove(int cid);

#endif
