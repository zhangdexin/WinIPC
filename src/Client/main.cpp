#include <iostream>
#include <string>
#include <set>

#include "CmnHdr.h"
#include "Client.h"

// Client startup
int main()
{
    std::cout << "Hello Client!\n"; 

    Client client("127.0.0.1", 27016);
    client.Run();
}
