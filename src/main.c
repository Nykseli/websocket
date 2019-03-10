
#include "server.h"

int main(int argc, char const *argv[]) {
    WebSocketServer wss;
    init_server(&wss);
    return run_server(&wss);

}
