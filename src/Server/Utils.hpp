#ifndef __UTILS_H__
#define __UTILS_H__

#include "CmnHdr.h"

#include <set>
#include <ctime>
#include <random>
#include <tcpmib.h>
#include <IPHlpApi.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

class Utils
{
public:
    static bool WritePortToMappingFile(const std::string& port) {
        HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, 
            PAGE_READWRITE | SEC_COMMIT, 0, 256, L"win_ipc_temp_port");
        if (hMap == NULL) {
            return false;
        }

        BYTE *byte = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
        if (byte == NULL) {
            return false;
        }
        memcpy(byte, port.c_str(), port.size());
        return true;
    }

    static int GetRandNum(int start, int end)
    {
        static int times = 0;
        std::srand((unsigned)std::time(NULL) + times);
        times++;

        return (rand() % (end - start)) + start;
    }

    static int GetIdlePort()
    {
        std::set<int> portSet;
        MIB_TCPTABLE TcpTable[100];
        DWORD nSize = sizeof(TcpTable);
        if (NO_ERROR == GetTcpTable(&TcpTable[0], &nSize, TRUE))
        {
            DWORD nCount = TcpTable[0].dwNumEntries;
            if (nCount > 0)
            {
                for (DWORD i = 0; i < nCount; i++)
                {
                    MIB_TCPROW TcpRow = TcpTable[0].table[i];
                    DWORD temp1 = TcpRow.dwLocalPort;
                    int temp2 = temp1 / 256 + (temp1 % 256) * 256;
                    portSet.insert(temp2);
                }
            }
        }

        int i = 0;
        int port = 9001;
        while (i < 20) {
            i++;
            port = GetRandNum(10000, 30000);

            if (portSet.find(port) == portSet.end()) {
                break;
            }
        }

        return port;
    }
};

#endif
