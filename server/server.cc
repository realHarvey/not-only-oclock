/*
 * server
 * Anyone-Copyright 2023 何宏波
 * wmi的c++ api比较残废，很多信息都获取不了
 * 告废
 */
#include <iostream>
#include <string>
#include <algorithm>
#include <winsock2.h>
#include <windows.h>
#include <sysinfoapi.h>
#include <wingdi.h>
#include <winnt.h>
#include <iomanip>
#include <cmath>
#include <type_traits>
#include <vector>
using std::cout, std::endl;

/*
 * update frequency: ms
 */
#define UPDATE_FREQ 1000
#define DISK_NUMBER    3

#define __B2GB(mm) ((float)mm/1024/1024/1024)

/* convert byte to GB */
#define B2GB(bytes, precision) \
({ \
        std::stringstream ss; \
        ss << std::fixed << std::setprecision(precision) << __B2GB(bytes); \
        std::stof(ss.str()); \
})

/* message buffer */
static std::vector<char> msg_buffer;
void Load_Data(char *data)
{
        int i;
        for (i = 0; data[i] != '\0'; i++) {
                msg_buffer.push_back(data[i]);
        }
}

void Send_Msg(int cfd)
{
        std::string msg_str(msg_buffer.begin(), msg_buffer.end());
        send(cfd, msg_str.c_str(), msg_str.length(), 0);
}

/*
 * Get CPU msg
 *      Cores
 */
void Get_CPU_Info()
{
        SYSTEM_INFO sys_info;
        
        GetSystemInfo(&sys_info);
        Load_Data((char*)(sys_info.dwNumberOfProcessors));
}

/*
 * Get GPU msg
 *
 */
void Get_GPU_Info()
{
        int i;
        std::string iter;
        std::string GPUs[2];
        DISPLAY_DEVICE display_device = {
                .cb = sizeof(DISPLAY_DEVICE),
        };

        for (i = 0; EnumDisplayDevices(nullptr, i, &display_device, 0); ++i) {
                iter = display_device.DeviceString;
                if (GPUs[0].empty())
                        GPUs[0] = iter;
                else if (GPUs[1].empty() && iter != GPUs[0])
                        GPUs[1] = iter; 
        }
        cout<< "< GPU >" << '\n'
            << "    GPU 0: " << GPUs[0] << '\n'
            << "    GPU 1: " << GPUs[1] << '\n';
}

/*
 * Get memory usage
 */
void Get_Memory_Info()
{
        MEMORYSTATUSEX memory_status;
        memory_status.dwLength = sizeof(memory_status);
        DWORDLONG total_memory, used_memory;
        float um, tm;

        if (!GlobalMemoryStatusEx(&memory_status)) {
                cout<< "Failed to get memory status." <<endl;
                return;
        }

        total_memory = memory_status.ullTotalPhys;
        used_memory  = memory_status.ullTotalPhys - memory_status.ullAvailPhys;

        um = B2GB(used_memory, 2);
        tm = B2GB(total_memory, 2);

        cout<< "< Memory >" << '\n'
            << "    " << um << " GB / " << tm << " GB " << '\n';
}

/*
 * Storage Usage
 */
void Get_Disk_Info()
{
        int i;
        const LPCSTR Disks[] = {"A:\\", "B:\\", "C:\\"};
        float used_storage, total_storage;
        ULARGE_INTEGER totalBytes, freeBytes, totalFreeBytes;

        cout<< "< Storage >" << '\n';
        
        for (i = 0; i < DISK_NUMBER; ++i) {
                if (!GetDiskFreeSpaceExA(Disks[i], &freeBytes, &totalBytes, &totalFreeBytes))
                        continue;

                total_storage = B2GB(totalBytes.QuadPart, 2);
                used_storage  = total_storage - B2GB(freeBytes.QuadPart,  2);
                cout<< "    " << Disks[i] << "   "
                    << used_storage << " GB / " << total_storage << " GB " << '\n';
        }
}

int main()
{
/* TCP socket */
        WSADATA wsa;
        int lfd, cfd;
        int client_addrlen;
        sockaddr_in serv_addr, client_addr;
        std::string data;

        // initialize windows socket api dll
        // cannot be without this in windows
        WSAStartup(MAKEWORD(2, 2), &wsa);

        // ipv4 tcp
        lfd = socket(AF_INET, SOCK_STREAM, 0);
        if (lfd == INVALID_SOCKET) {
                cout<< "ERROR: create socket." <<endl;
                goto end;
        }

        // socket ->IP(0 for auto check netcard IP and bind) : Port
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(12345);
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        if ( bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR ) {
                cout<< "ERROR: bind socket." <<endl;
                goto end;
        }

        // start listenning for client connect
        if ( listen(lfd, 10) == SOCKET_ERROR ) {
                cout<< "ERROR: listen on socket." <<endl;
                goto end;
        }
        cout<< "waiting for client connections..." <<endl;

        // connect
        client_addrlen = sizeof(client_addr);
        cfd = accept(lfd, (struct sockaddr*)&client_addr, &client_addrlen);
        if (cfd == INVALID_SOCKET) {
                cout<< "ERROR: accept client connection." <<endl;
                goto end;
        }
        cout<< "client connected." <<endl;
        

/* msg transfer */
        while(1) {
                Get_CPU_Info();
                Get_GPU_Info();
                Get_Memory_Info();
                Get_Disk_Info();
                Send_Msg(cfd);
                Sleep(UPDATE_FREQ);
        }

end:
        closesocket(cfd);
        closesocket(lfd);
        WSACleanup();
        return -1;
}