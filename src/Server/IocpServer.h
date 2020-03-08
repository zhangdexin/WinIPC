#ifndef __IOCPSERVER_H__
#define __IOCPSERVER_H__

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <map>
#include <mutex>
#include <vector>
#include <thread>
#include <iostream>
#include <string>

#include "CmnHdr.h"
#include "IOReq.hpp"
#include "IoCompletionPort.h"

class IocpServer
{
public:
    IocpServer();
    ~IocpServer();

    void Run();
    void Stop();

private:
    int Init();
    void AcceptReqAndRecv();
    int CreateSomeWorkThread();
    void DoWork();
    void DoResponse(const SOCKET socket, const char* data, DWORD size);

    void CleanupAllPendingSocket();
    void ClearPendingSocket(SOCKET socket);
    void ClearPendingRecvSocket(SOCKET socket);
    void ClearPendingSendSocket(SOCKET socket);

private:
    SOCKET             m_ListenSocket;
    CIOCP              m_IocpHandle;

    using MAPReq = std::map<SOCKET, IOReq*>;
    MAPReq             m_PendingRecvReqs;
    std::mutex         m_PendingRecvMutex;

    MAPReq             m_PendingSendReqs;
    std::mutex         m_PendingSendMutex;

    using VctThread = std::vector<std::shared_ptr<std::thread>>;
    VctThread          m_Threads;
};

#endif