#ifndef __IOCPSERVER_H__
#define __IOCPSERVER_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <map>
#include <mutex>
#include <vector>
#include <thread>
#include <string>
#include <iostream>
#include <functional>

#include "IOReq.hpp"
#include "IoCompletionPort.h"
#include "LSocket.hpp"

class IocpServer
{
public:
    enum ListenType {
        ListenType_IPC,
        ListenType_NPC,
    };

    IocpServer(ListenType type = ListenType_IPC, const std::string& port = "");
    ~IocpServer();

    using RecvFunction = std::function<void(const LSocket& sock, const char* data, DWORD size)>;
    void Run(const RecvFunction& recvCb = nullptr);
    void Stop();

    void SendMsg(const LSocket& sock, const char* data, DWORD size);
    void SendMsg(const LSocket& sock, const std::string& data);

private:
    int Init();
    int CreateSomeWorkThread();

    void AcceptReqAndRecv();
    void DoWork();
    void DoResponse(const LSocket& socket, const char* data, DWORD size);

    void CleanupAllPendingSocket();
    void ClearPendingSocket(const LSocket& socket);
    void ClearPendingRecvSocket(const LSocket& socket);
    void ClearPendingSendSocket(const LSocket& socket);

private:
    LSocket            m_ListenSocket;
    CIOCP              m_IocpHandle;
    RecvFunction       m_RecvCb;
    std::string        m_Port;

    using MAPReq = std::map<LSocket, IOReq*>;
    MAPReq             m_PendingRecvReqs;
    std::mutex         m_PendingRecvMutex;

    MAPReq             m_PendingSendReqs;
    std::mutex         m_PendingSendMutex;

    using VctThread = std::vector<std::shared_ptr<std::thread>>;
    VctThread          m_Threads;
};

#endif // __IOCPSERVER_H__