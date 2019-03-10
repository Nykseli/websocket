#ifndef WEB_SOCKET_SERVER_H
#define WEB_SOCKET_SERVER_H

typedef struct {
    int* clients;
} WebSocketServer;


void init_server(WebSocketServer* wss);
int run_server(WebSocketServer* wss);
void free_server(WebSocketServer* wss);



#endif
