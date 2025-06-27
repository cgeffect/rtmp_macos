// 有问题
/**
 * 基于ffmpeg和libflv实现的AAC RTMP推流器
 * 严格按照FLV格式和RTMP协议规范
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rtmp.h"   
#include "rtmp_sys.h"   
#include "amf.h"  

#ifdef WIN32     
#include <windows.h>  
#pragma comment(lib,"WS2_32.lib")   
#pragma comment(lib,"winmm.lib")  
#endif 

//定义包头长度，RTMP_MAX_HEADER_SIZE=18
#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
//存储AAC数据的buffer大小
#define BUFFER_SIZE 32768

static FILE *fp_send = NULL;
static RTMP* m_pRtmp = NULL;
static unsigned char *m_pFileBuf = NULL;
static unsigned int m_nFileBufSize = BUFFER_SIZE;

// 读文件的回调函数
int read_buffer(unsigned char *buf, int buf_size) {
    if (!fp_send) {
        return -1;
    }
    
    if (!feof(fp_send)) {
        int true_size = fread(buf, 1, buf_size, fp_send);
        return true_size;
    } else {
        return -1;
    }
}

/**
 * 初始化winsock
 */ 
static int InitSockets()    
{    
	#ifdef WIN32     
		WORD version;    
		WSADATA wsaData;    
		version = MAKEWORD(1, 1);    
		return (WSAStartup(version, &wsaData) == 0);    
	#else     
		return TRUE;    
	#endif     
}

/**
 * 释放winsock
 */ 
static void CleanupSockets()
{    
	#ifdef WIN32     
		WSACleanup();    
	#endif     
}    

/**
 * 发送RTMP数据包
 */
int SendPacket(unsigned int nPacketType, unsigned char *data, unsigned int size, unsigned int nTimestamp)  
{  
	RTMPPacket* packet;
	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
	memset(packet,0,RTMP_HEAD_SIZE);
	/*包体内存*/
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body,data,size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType;
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x05; // 音频通道

	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nTimeStamp = nTimestamp;
	/*发送*/
	int nRet = 0;
	if (RTMP_IsConnected(m_pRtmp))
	{
		nRet = RTMP_SendPacket(m_pRtmp,packet,TRUE);
	}
	/*释放内存*/
	free(packet);
	return nRet;  
}

/**
 * 生成AAC Audio Specific Config (严格按照ffmpeg实现)
 * 参考: https://wiki.multimedia.cx/index.php/MPEG-4_Audio#Audio_Specific_Config
 */
void GetAudioSpecificConfig(uint8_t* data, const uint32_t profile,
                           const uint32_t samplerate, const uint32_t channel_num)
{
    // profile在Audio Specific Config中需要加1
    uint16_t _profile = (uint16_t)profile + 1;
    _profile <<= 11;

    uint32_t _samplerate = 0;
    switch (samplerate)
    {
    case  96000: _samplerate = 0; break;
    case 88200: _samplerate = 1; break;
    case 64000: _samplerate = 2; break;
    case 48000: _samplerate = 3; break;
    case 44100: _samplerate = 4; break;
    case 32000: _samplerate = 5; break;
    case 24000: _samplerate = 6; break;
    case 22050: _samplerate = 7; break;
    case 16000: _samplerate = 8; break;
    case 12000: _samplerate = 9; break;
    case 11025: _samplerate = 10; break;
    case 8000: _samplerate = 11; break;
    case 7350: _samplerate = 12; break;
    default: _samplerate = 4; break;
    }

    _samplerate <<= 7;

    uint16_t _channel_num = (uint16_t)channel_num;
    _channel_num <<= 3;

    uint16_t audio_spec = _profile | _samplerate | _channel_num;

    data[0] = (uint8_t)(audio_spec >> 8);
    data[1] = 0xff & audio_spec;
    
    printf("Audio Specific Config: %02X %02X (profile=%d, samplerate=%d, channels=%d)\n", 
           data[0], data[1], profile, samplerate, channel_num);
}

/**
 * 发送AAC序列头 (Audio Specific Config)
 */
int SendAudioSequenceHeader(int sample_rate, int channels, int profile)
{
    RTMPPacket * packet = NULL;
    unsigned char * body = NULL;
    int i;
    
    packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + 4);
    memset(packet, 0, RTMP_HEAD_SIZE + 4);
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (unsigned char *)packet->m_body;
    
    i = 0;
    
    // FLV音频Tag头格式 (严格按照libflv实现)
    // SoundFormat(4bits) + SoundRate(2bits) + SoundSize(1bit) + SoundType(1bit)
    // AAC = 10, 44kHz = 3, 16bit = 1, Stereo = 1
    body[i++] = 0xAF; // 1010 1111: AAC, 44kHz, 16bit, 立体声
    if (channels == 1) {
        body[i-1] = 0xAE; // 1010 1110: AAC, 44kHz, 16bit, 单声道
    }
    
    // AACPacketType: 0 = AAC sequence header
    body[i++] = 0x00;
    
    // AudioSpecificConfig (严格按照ffmpeg实现)
    uint8_t audio_config[2];
    GetAudioSpecificConfig(audio_config, profile, sample_rate, channels);
    body[i++] = audio_config[0];
    body[i++] = audio_config[1];

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = i;
    packet->m_nChannel = 0x05;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = m_pRtmp->m_stream_id;

    printf("Sending AAC sequence header: ");
    for (int j = 0; j < i; j++) {
        printf("%02X ", body[j]);
    }
    printf("\n");

    /*调用发送接口*/
    int nRet = RTMP_SendPacket(m_pRtmp, packet, TRUE);
    free(packet);
    return nRet;
}

/**
 * 发送AAC数据帧 (严格按照FLV格式)
 */
int SendAACPacket(unsigned char *data, unsigned int size, unsigned int nTimeStamp)  
{  
    if(data == NULL || size < 2) {  
        return FALSE;  
    }  

    unsigned char *body = (unsigned char*)malloc(size + 2);  
    memset(body, 0, size + 2);

    int i = 0; 
    
    // FLV音频Tag头格式 (严格按照libflv实现)
    body[i++] = 0xAF; // AAC, 44kHz, 16bit, 立体声
    
    // AACPacketType: 1 = AAC raw data
    body[i++] = 0x01;

    // 裸AAC数据 (去掉ADTS头)
    memcpy(&body[i], data, size);  

    int bRet = SendPacket(RTMP_PACKET_TYPE_AUDIO, body, i + size, nTimeStamp);  

    free(body);  
    return bRet;  
}

/**
 * 初始化并连接到服务器
 */
int RTMPAAC_Connect(const char* url)  
{  
    m_nFileBufSize = BUFFER_SIZE;
    m_pFileBuf = (unsigned char*)malloc(BUFFER_SIZE);
    InitSockets();  

    m_pRtmp = RTMP_Alloc();
    RTMP_Init(m_pRtmp);
    
    /*设置URL*/
    if (RTMP_SetupURL(m_pRtmp,(char*)url) == FALSE)
    {
        RTMP_Free(m_pRtmp);
        return FALSE;
    }
    
    /*设置可写,即发布流,这个函数必须在连接前使用,否则无效*/
    RTMP_EnableWrite(m_pRtmp);
    
    /*连接服务器*/
    if (RTMP_Connect(m_pRtmp, NULL) == FALSE) 
    {
        RTMP_Free(m_pRtmp);
        return FALSE;
    } 

    /*连接流*/
    if (RTMP_ConnectStream(m_pRtmp,0) == FALSE)
    {
        RTMP_Close(m_pRtmp);
        RTMP_Free(m_pRtmp);
        return FALSE;
    }
    return TRUE;  
}

/**
 * 断开连接，释放相关的资源。
 */
void RTMPAAC_Close()  
{  
    if(m_pRtmp)  
    {  
        RTMP_Close(m_pRtmp);  
        RTMP_Free(m_pRtmp);  
        m_pRtmp = NULL;  
    }  
    CleanupSockets();   
    if (m_pFileBuf != NULL)
    {  
        free(m_pFileBuf);
        m_pFileBuf = NULL;
    }  
}

/**
 * 解析ADTS头获取帧长度 (严格按照ADTS规范)
 */
int ParseADTSHeader(unsigned char* data, int* frame_length, int* header_length) {
    if (data[0] != 0xFF || (data[1] & 0xF6) != 0xF0) {
        return 0; // 不是ADTS头
    }
    
    // ADTS头长度 - 根据ADTS规范
    *header_length = (data[1] & 0x01) ? 9 : 7;
    
    // 帧长度 (13位) - 严格按照ADTS规范
    *frame_length = ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] & 0xE0) >> 5);
    
    return 1; // 成功解析
}

/**
 * 从ADTS头中提取AAC参数 (严格按照ADTS规范)
 */
void ExtractAACParamsFromADTS(unsigned char* adts_header, int* profile, int* sample_rate, int* channels) {
    // 从ADTS头中提取profile (第2个字节的高2位)
    *profile = (adts_header[2] >> 6) & 0x03;
    
    // 从ADTS头中提取采样率索引 (第2个字节的中间4位)
    int sample_rate_index = (adts_header[2] >> 2) & 0x0F;
    
    // 从ADTS头中提取声道配置 (第2个字节的低1位 + 第3个字节的高3位)
    *channels = ((adts_header[2] & 0x01) << 2) | ((adts_header[3] >> 6) & 0x03);
    
    // 将采样率索引转换为实际采样率
    switch (sample_rate_index) {
        case 0: *sample_rate = 96000; break;
        case 1: *sample_rate = 88200; break;
        case 2: *sample_rate = 64000; break;
        case 3: *sample_rate = 48000; break;
        case 4: *sample_rate = 44100; break;
        case 5: *sample_rate = 32000; break;
        case 6: *sample_rate = 24000; break;
        case 7: *sample_rate = 22050; break;
        case 8: *sample_rate = 16000; break;
        case 9: *sample_rate = 12000; break;
        case 10: *sample_rate = 11025; break;
        case 11: *sample_rate = 8000; break;
        case 12: *sample_rate = 7350; break;
        default: *sample_rate = 44100; break;
    }
    
    // 修复profile映射：ADTS中的profile需要减1才是Audio Specific Config中的profile
    if (*profile > 0) {
        *profile = *profile - 1;
    }
    
    printf("Extracted from ADTS: profile=%d, sample_rate=%d, channels=%d\n", 
           *profile, *sample_rate, *channels);
}

/**
 * 主推流函数 (严格按照ffmpeg和libflv实现)
 */
int main(int argc, char *argv[]) {
    char *inUrl = (char *)"/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/res/test_mono.aac";
    // 输出的地址
    char *outUrl = (char *)"rtmp://127.0.0.1:1935/live/test";

    // 打开文件并检查
    fp_send = fopen(inUrl, "rb");
    if (!fp_send) {
        printf("Error: Cannot open input file %s\n", inUrl);
        return -1;
    }

    // 初始化并连接到服务器
    if (!RTMPAAC_Connect(outUrl)) {
        printf("Error: Failed to connect to RTMP server\n");
        fclose(fp_send);
        return -1;
    }

    printf("Starting AAC RTMP streaming...\n");

    // 读取第一个ADTS头，提取参数并发送一次Sequence Header
    unsigned char adts_buf[9];
    int ret = fread(adts_buf, 1, 7, fp_send); // 先读7字节
    if (ret < 7) {
        printf("Error: Failed to read ADTS header\n");
        RTMPAAC_Close();
        fclose(fp_send);
        return -1;
    }
    
    // 检查是否需要读额外的2字节
    if (adts_buf[1] & 0x01) {
        ret = fread(adts_buf + 7, 1, 2, fp_send);
        if (ret < 2) {
            printf("Error: Failed to read extended ADTS header\n");
            RTMPAAC_Close();
            fclose(fp_send);
            return -1;
        }
    }
    
    int profile, sample_rate, channels;
    ExtractAACParamsFromADTS(adts_buf, &profile, &sample_rate, &channels);
    
    // 发送AAC序列头 (Audio Specific Config)
    if (SendAudioSequenceHeader(sample_rate, channels, profile) != 1) {
        printf("Error: Failed to send AAC sequence header\n");
        RTMPAAC_Close();
        fclose(fp_send);
        return -1;
    }
    printf("AAC sequence header sent successfully\n");
    
    // 重置文件指针到开始位置
    rewind(fp_send);

    // 循环发送AAC帧 (严格按照ffmpeg实现)
    unsigned int tick = 0;
    unsigned int frame_count = 0;
    int samples_per_frame = 1024;
    unsigned int tick_gap = (samples_per_frame * 1000) / sample_rate;
    
    printf("Audio frame interval: %d ms\n", tick_gap);

    while (1) {
        rewind(fp_send);
        
        // 读取并解析ADTS帧
        int buffer_size;
        while ((buffer_size = fread(m_pFileBuf, 1, BUFFER_SIZE, fp_send)) > 0) {
            int buffer_pos = 0;
            while (buffer_pos + 7 <= buffer_size) {
                // 查找ADTS头
                if (m_pFileBuf[buffer_pos] == 0xFF && (m_pFileBuf[buffer_pos + 1] & 0xF6) == 0xF0) {
                    int frame_length, header_length;
                    if (ParseADTSHeader(&m_pFileBuf[buffer_pos], &frame_length, &header_length)) {
                        if (buffer_pos + frame_length > buffer_size) break;
                        
                        // 提取裸AAC数据 (去掉ADTS头)
                        unsigned char* aac_data = &m_pFileBuf[buffer_pos + header_length];
                        int aac_data_size = frame_length - header_length;
                        
                        if (aac_data_size > 0) {
                            if (!SendAACPacket(aac_data, aac_data_size, tick)) {
                                printf("Error: Failed to send AAC packet at frame %d\n", frame_count);
                                break;
                            }
                            
                            printf("Sent AAC frame %d, size: %d bytes, timestamp: %d\n", 
                                   frame_count, aac_data_size, tick);
                            
                            frame_count++;
                            tick += tick_gap;
                            
                            // 控制发送速率 (模拟实时推流)
                            msleep(tick_gap);
                        }
                        
                        buffer_pos += frame_length;
                        continue;
                    }
                }
                buffer_pos++;
            }
        }
        printf("Completed one loop, restarting...\n");
    }

    RTMPAAC_Close();
    if (fp_send) {
        fclose(fp_send);
        fp_send = NULL;
    }
    return 0;
} 