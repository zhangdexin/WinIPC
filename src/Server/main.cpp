#include "IocpServer.h"

// Server startup
int main()
{
    std::cout << "Hello Server!\n";
    IocpServer server;
    server.Run();
    return 0;
}
