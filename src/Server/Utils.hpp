#ifndef __UTILS_H__
#define __UTILS_H__

#include "CmnHdr.h"

class Utils
{
public:
    static void CleanupSocket(SOCKET& s) {
        if (s != INVALID_SOCKET) {
            closesocket(s);
            s = INVALID_SOCKET;
        }
    }
};

#endif
