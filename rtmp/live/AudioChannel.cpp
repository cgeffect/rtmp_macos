#include <rtmp.h>
#include <cstring>
#include "AudioChannel.h"
#include "util.h"

namespace live {

AudioChannel::AudioChannel() {
}

AudioChannel::~AudioChannel() {
    DELETE(buffer);
    if (audioEncoder) {
        faacEncClose(audioEncoder);
        audioEncoder = nullptr;
    }
}

void AudioChannel::initAudioEncoder(int sample_rate, int channels) {
    this->mChannels = channels; // 通道数量 2

    /**
     * 44100 采样率
     * 两个声道
     * 16bit 2个字节
     *
     * 上面的规格：
     *  单通道样本数：1024 * 2 = 2048
     *
     *  inputSamples = 1024 如果没有 channels 的设计，应该是这个值
     *
     *  inputSamples = 2048
     */

    /**
     * 第一步：打开faac编码器
     */
    audioEncoder = faacEncOpen(sample_rate, channels, &inputSamples, &maxOutputBytes);
    if (!audioEncoder) {
        LOGE("打开音频编码器失败");
        return;
    }
    
    printf("Input samples per frame: %lu\n", inputSamples);
    printf("Max output bytes per frame: %lu\n", maxOutputBytes);


    /**
     * 第二步：配置编码器参数
     */
    faacEncConfigurationPtr config = faacEncGetCurrentConfiguration(audioEncoder);

    config->mpegVersion = MPEG4; // mpeg4标准 acc音频标准

    config->aacObjectType = LOW; // LC标准： https://zhidao.baidu.com/question/1948794313899470708.html

    config->inputFormat = FAAC_INPUT_16BIT; // 16bit

    // 比特流输出格式为：Raw
    config->outputFormat = 0;

    // 1发送的时候，就消除 最好的，   2结束后消除回音（复杂）
    // 工作中：最麻烦的就是，（开启降噪, 噪声控制）
    config->useTns = 1;
    config->useLfe = 0;

    /**
     * 第三步：把三面的配置参数，传入进去给faac编码器,  audioEncoder==faac编码器 真正的编码器，可以用的
     */
    int ret = faacEncSetConfiguration(audioEncoder, config);
    if (!ret) { // ret == 0 失败 和 x264 设计 一样
        LOGE("音频编码器参数配置失败");
        return;
    }

    LOGE("FAAC编码器初始化成功...");
    // 输出缓冲区定义
    buffer = new u_char(maxOutputBytes);
}

// 发送aac序列头
int AudioChannel::getInputSamples() {
    return inputSamples;
}

//参考 https://wiki.multimedia.cx/index.php?title=MPEG-4_Audio#Audio_Specific_Config
static void GetAudioSpecificConfig(uint8_t* data, const uint32_t profile,
                                            const uint32_t samplerate,
                                            const uint32_t channel_num)
{
    //uint8_t type:5字节;        //前5位, 编码结构类型，AAC main编码为1，LOW低复杂度编码为2，SSR为3
    //uint8_t sample_rate:4字节; //接着4位, 表示采样率
    //uint8_t channel_num:4字节; //接着4为，表示声道数
    //uint8_t tail:3字节;        //最后3位固定为0
    
    uint16_t _profile = (uint16_t)profile+1;
    _profile <<= 11;

    uint32_t _samplerate = 0;
    switch (samplerate)
    {
    case  96000:
        _samplerate = 0;
        break;
    case 88200:
        _samplerate = 1;
        break;
    case 64000:
        _samplerate = 2;
        break;
    case 48000:
        _samplerate = 3;
        break;
    case 44100:
        _samplerate = 4;
        break;
    case 32000:
        _samplerate = 5;
        break;
    case 24000:
        _samplerate = 6;
        break;
    case 22050:
        _samplerate = 7;
        break;
    case 16000:
        _samplerate = 8;
        break;
    case 12000:
        _samplerate = 9;
        break;
    case 11025:
        _samplerate = 10;
        break;
    case 8000:
        _samplerate = 11;
        break;
    case 7350:
        _samplerate = 12;
        break;
    default:
        _samplerate = 4;
        break;
    }

    _samplerate <<= 7;

    uint16_t _channel_num = (uint16_t)channel_num;
    _channel_num <<= 3;

    uint16_t audio_spec = _profile | _samplerate | _channel_num;

    data[0] = (uint8_t)(audio_spec >> 8);
    data[1] = 0xff & audio_spec;

}

//static void get_adts_header(AVCodecContext *ctx, uint8_t *adts_header, int aac_length) {
//    uint8_t freq_idx = 0; // 0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
//    switch (ctx->sample_rate) {
//    case 96000: freq_idx = 0; break;
//    case 88200: freq_idx = 1; break;
//    case 64000: freq_idx = 2; break;
//    case 48000: freq_idx = 3; break;
//    case 44100: freq_idx = 4; break;
//    case 32000: freq_idx = 5; break;
//    case 24000: freq_idx = 6; break;
//    case 22050: freq_idx = 7; break;
//    case 16000: freq_idx = 8; break;
//    case 12000: freq_idx = 9; break;
//    case 11025: freq_idx = 10; break;
//    case 8000: freq_idx = 11; break;
//    case 7350: freq_idx = 12; break;
//    default: freq_idx = 4; break;
//    }
//    uint8_t chanCfg = ctx->channels;
//    uint32_t frame_length = aac_length + 7;
//    adts_header[0] = 0xFF;
//    adts_header[1] = 0xF1;
//    adts_header[2] = ((ctx->profile) << 6) + (freq_idx << 2) + (chanCfg >> 2);
//    adts_header[3] = (((chanCfg & 3) << 6) + (frame_length >> 11));
//    adts_header[4] = ((frame_length & 0x7FF) >> 3);
//    adts_header[5] = (((frame_length & 7) << 5) + 0x1F);
//    adts_header[6] = 0xFC;
//}

/**
 * 序列头
 * https://blog.csdn.net/qinglongzhan/article/details/82759141
 */
RTMPPacket *AudioChannel::getAudioSeqHeader() {
    // 这个头信息应该就是GetAudioSpecificConfig函数的值, GetAudioSpecificConfig函数参考自零声的rtmp-publish-20210903项目
    u_char *ppBuffer;
    u_long len;

    // 获取编码器的解码配置信息
    faacEncGetDecoderSpecificInfo(audioEncoder, &ppBuffer, &len);

    // 看图表，拼数据
    RTMPPacket *packet = new RTMPPacket;

    // int body_size = 2 + 2; // 后面的2：有16bit描述 头数据的长度，就是16bit
    int body_size = 2 + len;

    RTMPPacket_Alloc(packet, body_size);

    // AF == AAC编码器，44100采样率，位深16bit，双声道
    // AE == AAC编码器，44100采样率，位深16bit，单声道
    packet->m_body[0] = 0xAF; // 双声道
    if (mChannels == 1) {
        packet->m_body[0] = 0xAE; // 单声道
    }

    // 序列/头参数==0
    packet->m_body[1] = 0x00;

    // 序列头数据 Copy橙图的 解码数据
    memcpy(&packet->m_body[2], ppBuffer, 2); // 16bit == 2个字节 可以写死

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO; // 包类型，音频
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 11;                       // 通道ID，随便写一个，注意：不要写的和rtmp.c(里面的m_nChannel有冲突 4301行)
    packet->m_nTimeStamp = 0;                      // 头一般都是没有时间搓的
    packet->m_hasAbsTimestamp = 0;                 // 一般都不用
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE; // 大包的类型，如果是头信息，可以给一个小包

    return packet;
}

// 使用faac去编码，你必须把上面的告诉人家,  signed char 有符号（在所有音视频里面，最好用 无符号  uint8_t）
void AudioChannel::encodeData(uint8_t *data, int dataSize) { // 先试试 int8_t *data
    /**
     * 1，上面的初始化好的faac编码器
     * 2，数据（无符号的事情）
     * 3，上面的初始化好的样本数
     * 4，接收成果的 输出 缓冲区
     * 5，接收成果的 输出 缓冲区 大小
     * ret:返回编码后数据字节长度
     */
    int inputSamples = dataSize / sizeof(short);
    int byteLen = faacEncEncode(audioEncoder, reinterpret_cast<int32_t *>(data), inputSamples, buffer, maxOutputBytes);

    if (byteLen > 0) {
        // 看图，拼接数据
        RTMPPacket *packet = new RTMPPacket();

        int body_size = 2 + byteLen; // 后面的byteLen：我们实际数据编码后的长度

        RTMPPacket_Alloc(packet, body_size); // 堆区实例化里面的成员 packet

        // AF == AAC编码器，44100采样率，位深16bit，双声道
        // AE == AAC编码器，44100采样率，位深16bit，单声道
        packet->m_body[0] = 0xAF; // 双声道
        if (mChannels == 1) {
            packet->m_body[0] = 0xAE; // 单声道
        }

        // 这里是编码出来的音频数据，所以都是 01，  非序列/非头参数
        packet->m_body[1] = 0x01;

        // 音频数据 橙色Copy进去
        memcpy(&packet->m_body[2], buffer, byteLen);

        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO; // 包类型，音频
        packet->m_nBodySize = body_size;
        packet->m_nChannel = 11;                       // 通道ID，随便写一个，注意：不要写的和rtmp.c(里面的m_nChannel有冲突 4301行)
        packet->m_nTimeStamp = -1;                     // 帧数据有时间戳
        packet->m_hasAbsTimestamp = 0;                 // 一般都不用
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE; // 大包的类型，如果是头信息，可以给一个小包

        // 把数据包放入队列
        audioCallback(packet, this->handler_);
    }
}

void AudioChannel::setAudioCallback(void *handler, AudioCallback audioCallback) {
    this->handler_ = handler;
    this->audioCallback = audioCallback;
}

} // namespace live
