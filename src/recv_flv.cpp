
// 最简单的基于librtmp的示例：接收（RTMP保存为FLV）
// 接收到的数据是完整的flv文件, 可以直接写入文件
// 原文链接：https://blog.csdn.net/leixiaohua1020/article/details/42104893

// https://zhuanlan.zhihu.com/p/649606730
#include "rtmp.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
    return 1;
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

    int bufsize = 1024;
    char *buf = (char *)malloc(bufsize);
    memset(buf, 0, bufsize);
    long countbufsize = 0;

    char *inUrl = (char *)"/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/recv.flv";
    char *outUrl = (char *)"rtmp://172.16.184.26:1935/live/test";

    FILE *fp = fopen(inUrl, "wb");
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
    if (!RTMP_SetupURL(rtmp, outUrl)) {
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

    while (true) {
        // librtmp 的 RTMP_Read 函数会将 RTMP 数据包转换为 FLV 格式
        int nRead = RTMP_Read(rtmp, buf, bufsize);
        if (nRead <= 0) {
            printf("error !!!\n");
            usleep(25 * 1000);
            continue;
        }
        for (int i = 0; i < bufsize; i++) {
            printf("%02x ", buf[i]);
        }
        printf("\n");
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

/*
ffmpeg -re -i ./doc/source.flv -c copy -f flv -y rtmp://172.16.184.26:1935/live/livestream
拉流 rtmp
ffplay rtmp://172.16.184.26:1935/live/livestream

拉流 flv
ffplay http://172.16.184.26:8080/live/livestream.flv

*/
