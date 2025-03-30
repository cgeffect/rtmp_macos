#include <string>

extern "C" {      // 不用管了， 由于这些C源码，内部已经做了 extern "C" 你就管了， FFmpeg没有做
#include "rtmp.h" // 查找系统的环境变量 <>
#include "x264.h" // 查找系统的环境变量 <>
}

#include "VideoChannel.h"
#include "util.h"
#include "safe_queue.h"
#include "AudioChannel.h"

// 【Android RTMP】音频数据采集编码 ( AAC 音频格式解析 | FLV 音频数据标签解析 | AAC 音频数据标签头 | 音频解码配置信息 )
// https://hanshuliang.blog.csdn.net/article/details/106802700#2__1__AF__207
// https://cloud.tencent.com/developer/article/2247044?from=15425

namespace live {

class RtmpClient {
private:
public:
    RtmpClient();
    void start(const char *path);
    void initVideoEncoder(int width, int height, int fps, int bitrate);
    void pushVideo(uint8_t *dataNV21);

    void initAudioEncoder(int sample_rate, int num_channels);
    int getInputSamples();
    void pushAudio(uint8_t *dataPCM, int dataSize);

    void stop();
    void release();
    ~RtmpClient();

    void task_start(std::string path);
public:
    VideoChannel *videoChannel = nullptr;
    AudioChannel *audioChannel = nullptr;
    bool isStart;
    pthread_t pid_start;
    bool readyPushing;
    SafeQueue<RTMPPacket *> packets; // 不区分音频和视频，（音频 & 视频）一直存储，  start直接拿出去发送给流媒体服务器（添加到队列中的是压缩包）
    uint32_t start_time;             // 记录时间搓，为了计算下，时间搓
};

} // namespace live

// https://zhuanlan.zhihu.com/p/456832095
