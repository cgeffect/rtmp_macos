#ifndef DERRY_PUSH_AUDIOCHANNEL_H
#define DERRY_PUSH_AUDIOCHANNEL_H

// #include "libfaac/include/faac.h" 这样写没有任何问题
#include <faac.h>
#include <sys/types.h>

namespace live {

class AudioChannel {
public:
    typedef void (*AudioCallback)(RTMPPacket *packet, void *userdata);
    AudioChannel();
    ~AudioChannel();

    void initAudioEncoder(int sample_rate, int num_channels);

    int getInputSamples();

    void encodeData(uint8_t *data, int dataSize);

    void setAudioCallback(void *handle, AudioCallback audioCallback);

    RTMPPacket *getAudioSeqHeader();

private:
    u_long inputSamples;            // faac输出的样本数
    u_long maxOutputBytes;          // faac 编码器 最大能输出的字节数
    int mChannels;                  // 通道数
    faacEncHandle audioEncoder = 0; // 音频编码器
    u_char *buffer = 0;             // 后面要用到的 缓冲区
    AudioCallback audioCallback;
    void *handler_ = nullptr;
};

} // namespace live

#endif // DERRY_PUSH_AUDIOCHANNEL_H
