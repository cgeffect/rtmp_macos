// LibrtmpVS2015RecvStream.cpp : ??????????ó????????
//

#include "rtmp.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "librtmpsrc.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

static int InitSockets() {
#ifdef WIN32
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(1, 1);
    return (WSAStartup(version, &wsaData) == 0);
#endif
}

static void CleanupSockets() {
#ifdef WIN32
    WSACleanup();
#endif
}

int main() {
    RTMP *pRTMP = nullptr; /// RTMP_Alloc()
    printf("hello, librtmp,version=%d\n", RTMP_LibVersion());

    InitSockets();

    double duration = -1;
    int nRead;
    // is live stream ?
    bool bLiveStream = true;

    int bufsize = 1024 * 1024 * 10;
    char *buf = (char *)malloc(bufsize);
    memset(buf, 0, bufsize);
    long countbufsize = 0;

    FILE *fp = fopen("d:\\_movies\\ande10222.flv", "wb");
    if (!fp) {
        RTMP_LogPrintf("Open File Error.\n");
        CleanupSockets();
        return -1;
    }

    RTMP *rtmp = RTMP_Alloc();
    RTMP_Init(rtmp);
    // set connection timeout,default 30s
    rtmp->Link.timeout = 10;
    // HKS's live URL
    if (!RTMP_SetupURL(rtmp, "rtmp://192.168.1.9/live/test")) {
        RTMP_Log(RTMP_LOGERROR, "SetupURL Err\n");
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }
    if (bLiveStream) {
        rtmp->Link.lFlags |= RTMP_LF_LIVE;
    }

    // 1hour
    RTMP_SetBufferMS(rtmp, 3600 * 1000);

    if (!RTMP_Connect(rtmp, NULL)) {
        RTMP_Log(RTMP_LOGERROR, "Connect Err\n");
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }

    if (!RTMP_ConnectStream(rtmp, 0)) {
        RTMP_Log(RTMP_LOGERROR, "ConnectStream Err\n");
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        CleanupSockets();
        return -1;
    }

    while (nRead = RTMP_Read(rtmp, buf, bufsize)) {
        fwrite(buf, 1, nRead, fp);

        countbufsize += nRead;
        RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n", nRead, countbufsize * 1.0 / 1024);
    }

    if (fp)
        fclose(fp);

    if (buf) {
        free(buf);
    }
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        CleanupSockets();
        rtmp = NULL;
    }
    getchar();
    return 0;
}
