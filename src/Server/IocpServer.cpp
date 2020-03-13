#include "IocpServer.h"

#include "MsTcpIp.h"
#include "Utils.hpp"

#define END_SERVER 1
#define DEFAULT_PORT "27016"
#pragma comment(lib, "Ws2_32.lib")

IocpServer::IocpServer() :
    m_IocpHandle(0),
    m_RecvCb(nullptr)
{}

IocpServer::~IocpServer()
{}

void IocpServer::Run(const RecvFunction& recvCb)
{
    m_RecvCb = recvCb;
    if (Init() < 0) {
        return;
    }

    if (CreateSomeWorkThread() < 0) {
        return;
    }

    AcceptReqAndRecv();

    std::size_t size = m_Threads.size();
    for (std::size_t i = 0; i < size; ++i) {
        m_Threads[i]->join();
    }
    CleanupAllPendingSocket();
}

void IocpServer::Stop()
{
    int size = m_Threads.size();
    for (int i = 0; i < size; ++i) {
        m_IocpHandle.PostStatus(END_SERVER, -1, NULL);
    }

    m_ListenSocket.Cleanup();
}

void IocpServer::SendMsg(const LSocket& sock, const char* data, DWORD size)
{
    DoResponse(sock, data, size);
}

void IocpServer::SendMsg(const LSocket& sock, const std::string& data)
{
    DoResponse(sock, data.c_str(), data.size());
}

// handle recv data
void IocpServer::DoResponse(const LSocket& socket, const char* data, DWORD size)
{
    if (socket.IsInvalid()) {
        std::cout << "response socket is invalid" << std::endl;
        return;
    }

    // echo send
    IOReq* res = new IOReq;

    std::lock_guard<std::mutex> guard(m_PendingSendMutex);
    m_PendingSendReqs.emplace(std::make_pair(socket, res));

    res->Send(socket, data, size);
}

int IocpServer::Init()
{
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return -1;
    }

    // Determine version
    WORD version = wsaData.wVersion;
    if (LOBYTE(version) != 2 ||
        HIBYTE(version) != 2) {
        std::cout << "WSADATA version unacceptable :"
            << (int)LOBYTE(version) << "." << (int)HIBYTE(version) << std::endl;
        WSACleanup();
        return -1;
    }

    struct addrinfo *result = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed: " << iResult << std::endl;
        WSACleanup();
        return -1;
    }

    // Create a SOCKET
    m_ListenSocket = WSASocket(result->ai_family, result->ai_socktype, 
        result->ai_protocol, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (m_ListenSocket.IsInvalid()) {
        std::cout << "create socket error: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }

    // Setup the TCP listening socket
    iResult = bind(m_ListenSocket.Get(), result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        std::cout << "bind failed: " << WSAGetLastError() << std::endl;
        m_ListenSocket.Close();
        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }
    freeaddrinfo(result);

    // Listen on a socket
    iResult = listen(m_ListenSocket.Get(), SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        std::cout << "listen failed :" << WSAGetLastError() << std::endl;
        m_ListenSocket.Close();
        WSACleanup();
        return -1;
    }

    return 0;
}

void IocpServer::DoWork()
{
    ULONG_PTR compleKey;
    DWORD numBytes;
    IOReq *req = nullptr;

    while (1) {
        m_IocpHandle.GetStatus(&compleKey, &numBytes, (OVERLAPPED **)&req, INFINITE);

        // server end
        if (compleKey == END_SERVER) {
            return;
        }

        // the socket disconnect
        if (numBytes == 0) {
            ClearPendingSocket(req->Socket());
            req->Socket().Cleanup();
            continue;
        }

        // complete recv
        if (req->Type() == IOReq::ReqType_Recv) {
            if (m_RecvCb) {
                m_RecvCb(req->Socket(), req->data(), numBytes);
            }

            if (!req->Recv(req->Socket())) {
                delete req;
                std::cout << "recv one error:" << WSAGetLastError() << std::endl;
            }
            continue;
        }

        // complete send
        if (req->Type() == IOReq::ReqType_Send) {
            ClearPendingSendSocket(req->Socket());
        }
    }
}

void IocpServer::CleanupAllPendingSocket()
{
    {
        std::lock_guard<std::mutex> guard(m_PendingRecvMutex);
        for (auto iter = m_PendingRecvReqs.begin(); iter != m_PendingRecvReqs.end();) {
            LSocket sock = iter->first;
            sock.Cleanup();

            delete iter->second;
            iter = m_PendingRecvReqs.erase(iter);
        }
    }

    {
        std::lock_guard<std::mutex> guard(m_PendingSendMutex);
        for (auto iter = m_PendingSendReqs.begin(); iter != m_PendingSendReqs.end();) {
            LSocket sock = iter->first;
            sock.Cleanup();

            delete iter->second;
            iter = m_PendingSendReqs.erase(iter);
        }
    }
}

void IocpServer::ClearPendingSocket(const LSocket& socket)
{
    ClearPendingRecvSocket(socket);
    ClearPendingSendSocket(socket);
}

void IocpServer::ClearPendingRecvSocket(const LSocket& socket)
{
    std::lock_guard<std::mutex> guard(m_PendingRecvMutex);
    auto iter = m_PendingRecvReqs.find(socket);
    if (iter != m_PendingRecvReqs.end()) {
        delete iter->second;
        m_PendingRecvReqs.erase(socket);
    }
}

void IocpServer::ClearPendingSendSocket(const LSocket& socket)
{
    std::lock_guard<std::mutex> guard(m_PendingSendMutex);
    auto iter = m_PendingSendReqs.find(socket);
    if (iter != m_PendingSendReqs.end()) {
        delete iter->second;
        m_PendingSendReqs.erase(socket);
    }
}

int IocpServer::CreateSomeWorkThread()
{
    try {
        SYSTEM_INFO sys_Info;
        GetSystemInfo(&sys_Info);

        for (std::size_t i = 0; i < sys_Info.dwNumberOfProcessors * 2; ++i) {
            m_Threads.emplace_back(
                std::make_shared<std::thread>(&IocpServer::DoWork, this));
        }
    }
    catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}

void IocpServer::AcceptReqAndRecv()
{
    while (1) {
        LSocket acceptSocket = WSAAccept(m_ListenSocket.Get(), NULL, NULL, NULL, 0);
        if (m_ListenSocket.IsInvalid()) {
            std::cout << "accept stop..." << std::endl;
            return;
        }

        if (acceptSocket.IsInvalid()) {
            std::cout << "accept one invaile scoket..." << std::endl;
            continue;
        }

        std::cout << "accept on socket" << acceptSocket.Get() << std::endl;

        unsigned long chOpt;
        *((char *)&chOpt) = 1;

        int iResult = setsockopt(acceptSocket.Get(), SOL_SOCKET, SO_KEEPALIVE, (char *)&chOpt, sizeof(chOpt));
        if (iResult == SOCKET_ERROR) {
            std::cout << "setsockopt for SO_KEEPALIVE failed with error: " << WSAGetLastError() << std::endl;
        }

        // keepalive and timeout detail
        tcp_keepalive aliveDetail;
        aliveDetail.onoff = 1;
        aliveDetail.keepalivetime = 1000 * 30;
        aliveDetail.keepaliveinterval = 1000 * 10; // Resend if No-Reply
        WSAIoctl(acceptSocket.Get(), SIO_KEEPALIVE_VALS, &aliveDetail,sizeof(tcp_keepalive),
            NULL, 0, (unsigned long *)&chOpt, 0, NULL);

        // recv
        m_IocpHandle.AssociateSocket(acceptSocket.Get(), NULL);

        IOReq *req = new IOReq;
        {
            std::lock_guard<std::mutex> guard(m_PendingRecvMutex);
            m_PendingRecvReqs.emplace(std::make_pair(acceptSocket, req));
        }

        bool rc = req->Recv(acceptSocket.Get());
        if (!rc) {
            ClearPendingRecvSocket(acceptSocket);
            std::cout << "recv one error:" << WSAGetLastError() << std::endl;
        }
    }
}
