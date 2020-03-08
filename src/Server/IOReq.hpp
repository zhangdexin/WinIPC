#ifndef __IOREQ_H__
#define __IOREQ_H__

#include "CmnHdr.h"
#include "Utils.hpp"

#define BUFFER_SIZE 1024 * 4

class IOReq : public OVERLAPPED
{
public:
    IOReq() {
        ResetOverlapped();
    }

    ~IOReq() {}

    enum ReqType {
        ReqType_Send,
        ReqType_Recv,
    };

    bool Recv(SOCKET socket) {
        ResetOverlapped();
        ZeroMemory(&(m_Data), sizeof(m_Data));

        m_Type = ReqType_Recv;
        m_Socket = socket;
        m_WSABuffser.buf = m_Data;
        m_WSABuffser.len = BUFFER_SIZE;

        DWORD recvByte = 0;;
        DWORD flag = 0;

        int iRet = WSARecv(m_Socket, &m_WSABuffser, 1, &recvByte, &flag, this, NULL);
        if (iRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            return false;
        }
        return true;
    }

    bool Send(SOCKET socket, const char* pData, int len) {
        ResetOverlapped();
        ZeroMemory(&(m_Data), sizeof(m_Data));
        memcpy(m_Data, pData, len);

        m_Type = ReqType_Send;
        m_Socket = socket;
        m_WSABuffser.buf = m_Data;
        m_WSABuffser.len = len;

        DWORD sendByte = 0;
        DWORD flag = 0;

        int iRet = WSASend(m_Socket, &m_WSABuffser, 1, &sendByte, flag, this, NULL);
        if (iRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            return false;
        }
        return true;
    }

    SOCKET Socket() const {
        return m_Socket;
    }

    void CloseSocket() {
        Utils::CleanupSocket(m_Socket);
    }

    ReqType Type() const {
        return m_Type;
    }

    const char* data() const {
        return m_Data;
    }

private:
    void ResetOverlapped() {
        Internal = InternalHigh = 0;
        Offset = OffsetHigh = 0;
        hEvent = NULL;
    }

private:
    char               m_Data[BUFFER_SIZE];
    WSABUF             m_WSABuffser;
    SOCKET             m_Socket;
    ReqType            m_Type;
};

#endif