#include <iostream>
#include <string>
#include <set>

#include "Client.h"

// Client startup
int main()
{
    std::cout << "Hello Client!\n"; 

    try {
        Client client(Client::ConnectType_IPC, "127.0.0.1", 12345);

        // send msg
        std::thread thd([&]() {
            while (1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                Client::Status s = client.GetStatus();
                if (s == Client::Status_Running) {
                    std::string str = "123456";
                    bool rc = client.SendSync(str.c_str(), str.size());
                    if (!rc) {
                        std::cout << "send msg failed" << std::endl;
                        return;
                    }
                }
                else if (s == Client::Ststus_Stopped) {
                    return;
                }
            }
        });

        // run and recv msg
        client.Run([](const char* buffer, int size) {
            std::cout << "client recv:" << std::string(buffer, size) << std::endl;
        });

        thd.join();
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}