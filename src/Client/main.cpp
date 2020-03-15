#include <iostream>
#include <string>
#include <set>

#include "Client.h"

// Client startup
int main()
{
    std::cout << "Hello Client!\n"; 

    Client client(Client::ConnectType_IPC, "127.0.0.1", 12345);
    client.Run();
    return 0;
}