#include "Client.h"

#define BUFFER_SIZE (1024 * 4)

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

Client::Client(ConnectType type, const std::string& host, unsigned port) :
    m_HostName(host),
    m_ConnectPort(port),
    m_Status(Status_Idle)
{
    if (type == ConnectType_IPC) {
        m_HostName = "127.0.0.1";
        if (m_ConnectPort == 0) {
            HANDLE hFile = OpenFileMapping(FILE_MAP_READ, FALSE, L"win_ipc_temp_port");
            if (hFile == NULL) {
                throw std::exception("open maping error");
            }

            BYTE* byte = (LPBYTE)MapViewOfFile(hFile, FILE_MAP_READ, 0, 0, 0);
            if (byte == NULL) {
                throw std::exception("maping byte null");
            }

            m_ConnectPort = (int)std::stoi(std::string((char*)byte));

            UnmapViewOfFile(hFile);
            CloseHandle(hFile);
        }
    }
    else {
        if (m_ConnectPort == 0) m_ConnectPort = 27016;
    }
}

Client::~Client()
{
    Stop();
}

void Client::Run(const RecvFunction& recvCb)
{
    m_RecvCallback = recvCb;
    if (!Init()) {
        return;
    }

    m_Status = Status_Running;
    RecvSync();

    WSACleanup();
}

void Client::Stop()
{
    shutdown(m_Socket, SD_BOTH);
    m_Socket.Cleanup();
    m_Status = Ststus_Stopped;
}

bool Client::SendSync(const char* buf, size_t size)
{
    if (m_Socket.IsInvalid()) {
        return false;
    }

    int iResult = send(m_Socket, buf, size, 0);
    if (iResult == SOCKET_ERROR) {
        std::cout << "send failed with error" << WSAGetLastError() << std::endl;
        m_Socket.Cleanup();
        WSACleanup();
        return false;
    }

    return true;
}

bool Client::Init()
{
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed:" << iResult << std::endl;
        return false;
    }

    // Determine version
    WORD version = wsaData.wVersion;
    if (LOBYTE(version) != 2 ||
        HIBYTE(version) != 2) {
        std::cout << "WSADATA version unacceptable :"
            << (int)LOBYTE(version) << "." << (int)HIBYTE(version) << std::endl;
        WSACleanup();
        return false;
    }

    struct addrinfo *result = NULL;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(m_HostName.c_str(), std::to_string(m_ConnectPort).c_str(), &hints, &result);
    if (iResult != 0) {
        std::cout << "getaddrinfo failed" << iResult << std::endl;
        WSACleanup();
        return false;
    }

    // Attempt to connect to an address until one succeeds
    for (addrinfo *ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        m_Socket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (m_Socket.IsInvalid()) {
            std::cout << "socket failed with error:" << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }

        // Connect to server.
        iResult = connect(m_Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            m_Socket.Cleanup();
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    // set timeout
    int timeout = 3000;
    int ret = setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
    if (ret == -1)
    {
        m_Socket.Cleanup();
        WSACleanup();
        return false;
    }

    return true;
}

void Client::RecvSync()
{
    while (1)
    {
        char buf[BUFFER_SIZE] = { 0 };

        // ignore exceed BUFFER_SIE pkt
        int ret = recv(m_Socket, buf, 1024 * 4, 0);
        if (ret > 0 && ret < BUFFER_SIZE) {
            if (m_RecvCallback) {
                m_RecvCallback(buf, ret);
            }
        }
        else if (ret == 0) { // disconnect
            std::cout << "disconnect" << std::endl;
            break;
        }
        else if (ret == SOCKET_ERROR) {
            DWORD errorCode = WSAGetLastError();
            std::cout << "recv error:" << errorCode << std::endl;
            if (errorCode == WSAENOTCONN || errorCode == WSAECONNRESET) { // force disconnect
                break;
            }
        }
    }

    m_Socket.Cleanup();
}
