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
    Client(const std::string& host, unsigned port) : 
        m_HostName(host),
        m_ConnectPort(port) {}

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