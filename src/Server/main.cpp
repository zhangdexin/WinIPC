#include "IocpServer.h"

// Server startup
int main()
{
    std::cout << "Hello Server!\n";
    IocpServer server(IocpServer::ListenType_IPC, "12345");
    server.Run([&server](const LSocket& socket, const char* data, DWORD size) {
        std::string dataStr = std::string(data, size);
        std::cout << dataStr << std::endl;

        server.SendMsg(socket, dataStr + ":server");

        static int t = 0;
        t++;
        if (t == 10) {
            server.Stop();
        }
    });
    return 0;
}
