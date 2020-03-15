#ifndef __CLIENT_H__
#define __CLIENT_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

#include "CmnHdr.h"
#include "EnsureCleanup.h"

class Client
{
public:
    enum ConnectType {
        ConnectType_IPC,
        ConnectType_NPC,
    };

    Client(ConnectType type, const std::string& host = "127.0.0.1", unsigned port = 0);

    void Run();
    void Stop();

    bool SendSync(const char* buf, size_t size);

private:
    bool Init();
    void RecvSync();


private:
    std::string        m_HostName;
    unsigned           m_ConnectPort;
    CEnsureCloseSocket m_Socket;
};

#endif