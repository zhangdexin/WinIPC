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

    enum Status {
        Status_Idle,
        Status_Running,
        Ststus_Stopped
    };

    Client(ConnectType type, const std::string& host = "127.0.0.1", unsigned port = 0);
    ~Client();

    using RecvFunction = std::function<void(const char*, int)>;
    void Run(const RecvFunction& recvCb = nullptr);
    void Stop();

    bool SendSync(const char* buf, size_t size);
    Status GetStatus() const {
        return m_Status;
    }

private:
    bool Init();
    void RecvSync();


private:
    std::string        m_HostName;
    unsigned           m_ConnectPort;
    CEnsureCloseSocket m_Socket;
    RecvFunction       m_RecvCallback;
    Status             m_Status;
};

#endif