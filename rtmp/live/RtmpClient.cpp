
#include "RtmpClient.h"
#include <functional>
#include <future>
#include <thread>

namespace live {

// 释放RTMPPacket * 包的函数指针实现
// T无法释放， 让外界释放
void releasePackets(RTMPPacket **packet) {
    if (packet) {
        RTMPPacket_Free(*packet);
        //        delete packet;
        packet = nullptr;
    }
}

/**
 * DerryPusher构造函数调用的 ---> 初始化工作
 */
void RtmpClient::task_start(std::string path) {
    char *url = (char *)(path.c_str());
    // TODO RTMPDump API 九部曲
    RTMP *rtmp = nullptr;
    int ret; // 返回值判断成功失败

    do { // 为了方便流程控制，方便重试，习惯问题而已，如果不用（方便错误时重试）
        // 1.1，rtmp 初始化
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            LOGE("rtmp 初始化失败");
            break;
        }

        // 1.2，rtmp 初始化
        RTMP_Init(rtmp);
        rtmp->Link.timeout = 5; // 设置连接的超时时间（以秒为单位的连接超时）

        // 2，rtmp 设置流媒体地址
        ret = RTMP_SetupURL(rtmp, url);
        if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
            LOGE("rtmp 设置流媒体地址失败");
            break;
        }

        // 3，开启输出模式
        RTMP_EnableWrite(rtmp);

        // 4，建立连接
        ret = RTMP_Connect(rtmp, nullptr);
        if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
            LOGE("rtmp 建立连接失败:%d, url: %s", ret, url);
            break;
        }

        // 5，连接流
        ret = RTMP_ConnectStream(rtmp, 0);
        if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
            LOGE("rtmp 连接流失败");
            break;
        }

        start_time = RTMP_GetTime();

        // 准备好了，可以开始向服务器推流了
        readyPushing = true;

        // TODO 测试是不用发送序列头信息，是没有任何问题的，但是还是规矩
        // callback(audioChannel->getAudioSeqHeader());

        // 我从队列里面获取包，直接发给服务器

        // 队列开始工作
        packets.setWork(1);

        RTMPPacket *packet = nullptr;

        while (readyPushing) {
            if (packets.size() <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            packets.pop(packet); // 阻塞式

            if (!readyPushing) {
                break;
            }

            if (!packet) {
                continue;
            }

            // TODO 到这里就是成功的去除队列的ptk了，就可以发送给流媒体服务器

            // 给rtmp的流id
            packet->m_nInfoField2 = rtmp->m_stream_id;

            // 成功取出数据包，发送
            ret = RTMP_SendPacket(rtmp, packet, 1); // 1==true 开启内部缓冲

            // packet 你都发给服务器了，可以大胆释放
            releasePackets(&packet);

            if (!ret) { // ret == 0 和 ffmpeg不同，0代表失败
                LOGE("rtmp 失败 自动断开服务器");
                break;
            }
        }
        releasePackets(&packet); // 只要跳出循环，就释放
    } while (false);

    // 本次一系列释放工作
    isStart = false;
    readyPushing = false;
    packets.setWork(0);
    packets.clear();

    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
}

} // namespace live

namespace live {

// videoCallback 函数指针的实现（存放packet到队列）
void videocallback(RTMPPacket *packet, void *userdata) {
    RtmpClient *rtmpClient = static_cast<RtmpClient *>(userdata);
    if (packet) {
        if (packet->m_nTimeStamp == -1) {
            packet->m_nTimeStamp = RTMP_GetTime() - rtmpClient->start_time; // 如果是sps+pps 没有时间搓，如果是I帧就需要有时间搓
        }
        rtmpClient->packets.push(packet); // 存入队列里面  把刚刚的音频Packet丢进去
    }
}

RtmpClient::RtmpClient() {
    char version[50];
    //  2.3
    printf(version, "librtmp version: %d\n", RTMP_LibVersion());

    // ___________________________________> 下面是x264 验证  视频编码
    x264_picture_t *picture = new x264_picture_t;

    // ___________________________________> 下面是faac 验证 音频编码

    // C++层的初始化工作而已
    videoChannel = new VideoChannel();
    audioChannel = new AudioChannel();

    // 存入队列的 关联
    videoChannel->setVideoCallback(this, videocallback);

    // 存入队列的 关联
    audioChannel->setAudioCallback(this, videocallback);

    // 队列的释放工作 关联
    packets.setReleaseCallback(releasePackets);
}

void RtmpClient::start(const char *path) {
    /**
     * 开始直播 ---> 启动工作
     */
    // 子线程  1.连接流媒体服务器， 2.发包
    if (isStart) {
        return;
    }
    isStart = true;

    // 深拷贝
    // 以前我们在做player播放器的时候，是用Flag来控制（isPlaying）
    //    char *url = new char(strlen(path)); // C++的堆区开辟 new -- delete
    //    strcpy(url, path);

    // 创建线程来进行直播
    // pthread_create(&pid_start, nullptr, task_start, (char *)path);

    // 开始线程：读取文件
    std::thread([this, path]() {
        task_start(path);
    }).detach();

//    std::future<int> localFuture_ = std::async(std::launch::async, [this, path]() {
//        task_start(path);
//        return 0;
//    });

    printf("1111");
}

void RtmpClient::initVideoEncoder(int width, int height, int fps, int bitrate) {
    /**
     * 初始化x264编码器 给下面的pushVideo函数，去编码
     */
    if (videoChannel) {
        videoChannel->initVideoEncoder(width, height, fps, bitrate);
    }
}

void RtmpClient::pushVideo(uint8_t *dataNV21) {
    /**
     * 往队列里面 加入 RTMPPkt（x264编码后的RTMPPkt）
     */
    // data == nv21数据  编码  加入队列
    if (!videoChannel || !readyPushing) { return; }

    // 把jni ---> C语言的
    videoChannel->encodeData(dataNV21);
}

void RtmpClient::initAudioEncoder(int sample_rate, int num_channels) {
    if (audioChannel) {
        audioChannel->initAudioEncoder(sample_rate, num_channels);
    }
}

int RtmpClient::getInputSamples() {
    // 获取 faac的样本数 给 Java层
    if (audioChannel) {
        return audioChannel->getInputSamples();
    }
    return 0;
}

void RtmpClient::pushAudio(uint8_t *data, int dataSize) {
    // 使用上面的faac编码器，编码，封包，入队， start线程发送给流媒体服务器
    if (!audioChannel || !readyPushing) {
        return;
    }
    audioChannel->encodeData(data, dataSize); // 核心函数：对音频数据 【进行faac的编码工作】
}

void RtmpClient::stop() {
    isStart = false;                  // start的jni函数拦截的标记，为false
    readyPushing = false;             // 九部曲推流的标记，为false
    packets.setWork(0);               // 队列不准工作了
    pthread_join(pid_start, nullptr); // 稳稳等待start线程执行完成后，我在做后面的处理工作
    // ... 为以后做预留
}

void RtmpClient::release() {
    DELETE(videoChannel);
    DELETE(audioChannel);
}

RtmpClient::~RtmpClient() {
}
} // namespace live
