#ifndef __LSOCKET_H__
#define __LSOCKET_H__

#include "CmnHdr.h"

class LSocket
{
public:
    LSocket() :
        m_Socket(INVALID_SOCKET){}

    LSocket(SOCKET sock) :
        m_Socket(sock) {}

    bool IsInvalid() const {
        return (m_Socket == INVALID_SOCKET);
    }

    void Close() {
        if (m_Socket != INVALID_SOCKET) {
            closesocket(m_Socket);
            m_Socket = INVALID_SOCKET;
        }
    }

    void Shundown(int flag) {
        if (m_Socket != INVALID_SOCKET) {
            shutdown(m_Socket, flag);
        }
    }

    void Cleanup() {
        Shundown(SD_BOTH);
        Close();
    }

    bool operator < (const LSocket& rh) const {
        return m_Socket > rh.m_Socket;
    }

    SOCKET Get() const {
        return m_Socket;
    }

private:
    SOCKET m_Socket;
};

#endif // __LSOCKET_H__
