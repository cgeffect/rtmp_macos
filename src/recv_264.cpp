// 使用librtmp接受flv的video tag
#include "rtmp.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "util/adts.h"

typedef unsigned char byte;
typedef unsigned int uint;

#define SPS_HEADER_SIZE 13
#define PPS_HEADER_SIZE 3
#define IDR_PIC_HEADER_SIZE 9
#define SLICE_HEADER_SIZE 9
#define NALU_START_CODE "\x00\x00\x00\x01"

#define AAC_HEADER_SIZE = 2

enum nal_unit_type_e
{
    NAL_UNKNOWN     = 0,
    NAL_SLICE       = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,
    NAL_PPS         = 8,
    NAL_AUD         = 9,
    NAL_FILLER      = 12,
    /* ref_idc == 0 for 6,9,10,11,12 */
};

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
int sampleRate = -1;
int profile = -1;
int bitDepth = -1;
int channels = -1;

void printLog(uint8_t *data, int size) {
    for (int i = 0; i < size; i++) {
        // 使用 %02hhx 格式说明符
        printf("%02x ", data[i]);
    }
    printf("\n");
}

// 解析 rtmp video packet 数据
void parse_rtmp_video_packet_data(char *packet_data, int packet_data_size, FILE *h264fh) {
    // sps pps
    if (packet_data[0] == 0x17 && packet_data[1] == 0x00) {
        printf("nalu_type: sps pps\n");
        uint8_t *sps_header = (uint8_t *)malloc(SPS_HEADER_SIZE);
        memcpy(sps_header, packet_data, SPS_HEADER_SIZE);
//        printLog(sps_header, SPS_HEADER_SIZE);

        // 取出最后两位, sps的长度
        int sps_data_size = (sps_header[SPS_HEADER_SIZE - 2] << 8) | sps_header[SPS_HEADER_SIZE - 1];
        printf("计算结果: %d\n", sps_data_size);
        uint8_t *sps_data = (uint8_t *)malloc(sps_data_size);
        memcpy(sps_data, packet_data + SPS_HEADER_SIZE, sps_data_size);
        
//        printLog(sps_data, sps_data_size);
        
        // 写入起始码 0x00000001
        fwrite(NALU_START_CODE, 1, 4, h264fh);
        // 写入恢复的NALU头
        fwrite(sps_data, 1, sps_data_size, h264fh);
        
        uint8_t *pps_header = (uint8_t *)malloc(PPS_HEADER_SIZE);
        memcpy(pps_header, packet_data + SPS_HEADER_SIZE + sps_data_size, PPS_HEADER_SIZE);
        // 取出最后两位, pps的长度
        int pps_data_size = (pps_header[PPS_HEADER_SIZE - 2] << 8) | pps_header[PPS_HEADER_SIZE - 1];
        printf("计算结果: %d\n", pps_data_size);
        uint8_t *pps_data = (uint8_t *)malloc(pps_data_size);
        memcpy(pps_data, packet_data + SPS_HEADER_SIZE + sps_data_size + PPS_HEADER_SIZE, pps_data_size);
        
//        printLog(pps_data, pps_data_size);
        
        // 写入起始码 0x00000001
        fwrite(NALU_START_CODE, 1, 4, h264fh);
        // 写入恢复的NALU头
        fwrite(pps_data, 1, pps_data_size, h264fh);

        free(sps_header);
        free(sps_data);
        free(pps_header);
        free(pps_data);
        
    } else if (packet_data[0] == 0x17 && packet_data[1] == 0x01) {
        // I 帧
        printf("nalu_type: I 帧\n");
        uint8_t *idr_header = (uint8_t *)malloc(IDR_PIC_HEADER_SIZE);
        memcpy(idr_header, packet_data, IDR_PIC_HEADER_SIZE);
        
//        printLog(idr_header, IDR_PIC_HEADER_SIZE);
        // 去取出最后两位
        int nalu_data_size = (idr_header[IDR_PIC_HEADER_SIZE - 2] << 8) | idr_header[IDR_PIC_HEADER_SIZE - 1];
        printf("计算结果: %d %d\n", nalu_data_size, packet_data_size - IDR_PIC_HEADER_SIZE);
        uint8_t *nalu_data = (uint8_t *)malloc(nalu_data_size);
        memcpy(nalu_data, packet_data + IDR_PIC_HEADER_SIZE, nalu_data_size);
//        printLog(nalu_data, nalu_data_size);
        
        // 写入起始码 0x00000001
        fwrite(NALU_START_CODE, 1, 4, h264fh);
        // 写入恢复的NALU头
        fwrite(nalu_data, 1, nalu_data_size, h264fh);
        free(idr_header);
        free(nalu_data);
        
    } else if (packet_data[0] == 0x27 && packet_data[1] == 0x01) {
        printf("nalu_type: SEI P B\n");
        // SEI P B
        uint8_t *slice_header = (uint8_t *)malloc(SLICE_HEADER_SIZE);
        memcpy(slice_header, packet_data, SLICE_HEADER_SIZE);
        
//        printLog(slice_header, SLICE_HEADER_SIZE);

        // 去取出最后两位
        int slice_data_size = (slice_header[SLICE_HEADER_SIZE - 2] << 8) | slice_header[SLICE_HEADER_SIZE - 1];
        printf("计算结果: %d, %d\n", slice_data_size, packet_data_size - SLICE_HEADER_SIZE);
        uint8_t *slice_data = (uint8_t *)malloc(slice_data_size);
        memcpy(slice_data, packet_data + SLICE_HEADER_SIZE, slice_data_size);
        
//        printLog(slice_data, slice_data_size);
        
        // 写入起始码 0x00000001
        fwrite(NALU_START_CODE, 1, 4, h264fh);
        // 写入恢复的NALU头
        fwrite(slice_data, 1, slice_data_size, h264fh);
        
        free(slice_header);
        free(slice_data);
    }
    
    fflush(h264fh);
}

// https://www.cnblogs.com/8335IT/p/18208384 RTMP解析音频AAC
void parse_rtmp_audio_packet_data(char *packet_data, int packet_data_size, FILE *aacfh) {
    // 头信息
    if ((unsigned char)packet_data[0] == 0xAF && packet_data[1] == 0x0) {
//        printLog((uint8_t *)packet_data, 4);
        // 第一个字节
        // AF 1010 1111
        // 高前4位是编码器类型 & 0xF0 1111 0000
        int format = packet_data[0] & 0xF0 >> 4;
        if (format == 10) {
            printf("音频数据格式值: AAC\n");
        }
        // 高5-6位 & 0x0C 0000 1100
        int sampleRateVal = packet_data[0] & 0x0C >> 2;
        if (sampleRateVal == 0) {
            sampleRate = 5000;
        } else if (sampleRateVal == 1) {
            sampleRate = 11000;
        } else if (sampleRateVal == 2) {
            sampleRate = 22000;
        } else if (sampleRateVal == 3) {
            sampleRate = 44100;
        }

        // 采样位数
        int bitDepthVal = packet_data[0] & 0x02 >> 1;
        if (bitDepthVal == 0) {
            bitDepth = 9;
        } else if (bitDepthVal == 1) {
            bitDepth = 16;
        }
        
        // 声道数
        int channelsVal = packet_data[0] & 0x01;
        if (channelsVal == 0) {
            channels = 1;
        } else if (channelsVal == 1) {
            channels = 2;
        }
        
        // 3-4个字节, 参考get_adts_header里adts_header[2] = ((profile) << 6) + (freq_idx << 2) + (chanCfg >> 2); 第三个字节的高6位的值是(profile) << 6
        profile = packet_data[2] & 0xFC >> 6;

    } else if ((unsigned char)packet_data[0] == 0xAF && packet_data[1] == 0x01) {
        // 数据
        uint8_t aac_header[7];
        get_adts_header(sampleRate, channels, profile, aac_header, packet_data_size - 2);
        size_t len = fwrite(aac_header, 1, 7, aacfh);
        printf("len: %zu\n", len);
        
        fwrite(packet_data + 2, 1, packet_data_size - 2, aacfh);
    }
    
    fflush(aacfh);
}

// ffmpeg -re -i ./doc/source.flv -c copy -f flv -y rtmp://172.16.184.26:1935/live/livestream
int main() {
    RTMP *pRTMP = nullptr; /// RTMP_Alloc()
    printf("hello, librtmp,version=%d\n", RTMP_LibVersion());

    InitSockets();

    double duration = -1;
    int nRead;
    // is live stream ?
    bool bLiveStream = true;

    char *outUrl = (char *)"rtmp://172.16.184.26:1935/live/test";
    FILE *vfp = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/rtmp_recv.h264", "wb");
    if (!vfp) {
        RTMP_LogPrintf("Open File Error.\n");
        CleanupSockets();
        return -1;
    }

    FILE *afp = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/rtmp_recv.aac", "wb");
    if (!afp) {
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

    RTMPPacket packet = {0};
    while (true) {
        bool isReady = rtmp->m_bPlaying && RTMP_IsConnected(rtmp);
        printf("isReady: %s\n", isReady ? "true" : "false");
        int size = RTMP_ReadPacket(rtmp, &packet);
        // 一个消息可能被封装成多个块(Chunk)，只有当所有块读取完才处理这个消息包
        if (!RTMPPacket_IsReady(&packet)) { //是否读取完毕
            printf("not ready\n");
            continue;
        }
        if (isReady && size != 0) {

            if (packet.m_packetType == RTMP_PACKET_TYPE_VIDEO) {
                printf("recv RTMP_PACKET_TYPE_VIDEO,size=%d\n", packet.m_nBodySize);
                parse_rtmp_video_packet_data(packet.m_body, packet.m_nBodySize, vfp);
            } else if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO) {
                printf("recv RTMP_PACKET_TYPE_AUDIO ,size=%d\n", packet.m_nBodySize);
                parse_rtmp_audio_packet_data(packet.m_body, packet.m_nBodySize, afp);
            } else if (packet.m_packetType == RTMP_PACKET_TYPE_INFO) {
                printf("recv RTMP_PACKET_TYPE_INFO ,size=%d\n", packet.m_nBodySize);
            }
        } else {
            printf("error !!!\n");
            usleep(25 * 1000);
        }
    }

    fclose(vfp);
    fclose(afp);

    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        CleanupSockets();
        rtmp = NULL;
    }
    getchar();
    return 0;
}
