/**
 * 使用ffmpeg发送flv文件到服务器
 */

/// ffmpeg.c.api
extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}
#include <iostream>
using namespace std;

// #pragma comment(lib, "avformat.lib")
// #pragma comment(lib, "avutil.lib")
// #pragma comment(lib, "avcodec.lib")
// #pragma warning(disable : 4996)
/// 将敬告禁用 ;;;#pragma warning(disable: 4996)

static int avError(int errNum) {
    char buf[1024];
    // 获取错误信息
    av_strerror(errNum, buf, sizeof(buf));
    cout << " failed: " << buf << endl;
    return -1;
}

static double r2d(AVRational r) {
    return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int main() {
    // 所有代码执行之前要调用av_register_all和avformat_network_init
    // 初始化所有的封装和解封装 flv mp4 mp3 mov。不包含编码和解码
    // av_register_all();

    avformat_network_init();
    unsigned int avfmtVersion = avformat_version();
    printf("hello, vs2015+ffmpeg=%ud\n", avfmtVersion);

    // 使用的相对路径，执行文件在bin目录下。test.mp4放到bin目录下即可
    const char *inUrl = "/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/768x320.flv";
    // 输出的地址
    const char *outUrl = "rtmp://172.16.184.26:1935/live/test";

    AVFormatContext *ictx = NULL;

    // 打开文件，解封文件头
    int ret = avformat_open_input(&ictx, inUrl, 0, NULL);
    if (ret < 0) {
        return avError(ret);
    }
    cout << "avformat_open_input success!" << endl;
    // 获取音频视频的信息 .h264 flv 没有头信息
    ret = avformat_find_stream_info(ictx, 0);
    if (ret != 0) {
        return avError(ret);
    }
    // 打印视频视频信息
    // 0打印所有  inUrl 打印时候显示，
    av_dump_format(ictx, 0, inUrl, 0);
    printf("\n\n\n");

    //////////////////////////////////////
    //////////////////////////////////////////////////////////////////
    //                   输出流处理部分
    /////////////////////////////////////////////////////////////////
    AVFormatContext *octx = NULL;
    // 如果是输入文件 flv可以不传，可以从文件中判断。如果是流则必须传
    // 创建输出上下文
    ret = avformat_alloc_output_context2(&octx, NULL, "flv", outUrl);
    if (ret < 0) {
        return avError(ret);
    }
    cout << "avformat_alloc_output_context2 success!" << endl;

    // 遍历输入的 AVStream，并且创建对应的输出流
    for (int i = 0; i < ictx->nb_streams; i++) {
        // 创建一个新的流到 octx 中
        AVStream *out = avformat_new_stream(octx, NULL); // 不再需要传入 codec
        if (!out) {
            return avError(0);
        }

        // 复制配置信息
        ret = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
        if (ret < 0) {
            return avError(ret);
        }
        out->codecpar->codec_tag = 0; // 修改 codecpar 中的 codec_tag
    }

    av_dump_format(octx, 0, outUrl, 1);

    //////////////////////////////////////////////////////////////////
    //                   准备推流
    /////////////////////////////////////////////////////////////////

    // 打开IO
    ret = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
    if (ret < 0) {
        avError(ret);
    }

    // 写入头部信息
    ret = avformat_write_header(octx, 0);
    if (ret < 0) {
        avError(ret);
    }
    cout << "avformat_write_header Success!" << endl;

    int64_t lastPts = 0;
    AVPacket avPacket;
    // 获取当前的时间戳  微妙
    long long startTime = av_gettime();
    while (true) {
        ret = av_read_frame(ictx, &avPacket);
        if (ret < 0) {
            break;
        }
        cout << avPacket.pts << " " << flush;

        // 计算转换时间戳 pts dts
        // 获取时间基数
        AVRational itime = ictx->streams[avPacket.stream_index]->time_base;
        AVRational otime = octx->streams[avPacket.stream_index]->time_base;
        avPacket.pts =
            av_rescale_q_rnd(avPacket.pts, itime, otime,
                             (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        avPacket.dts =
            av_rescale_q_rnd(avPacket.pts, itime, otime,
                             (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        // 到这一帧时候经历了多长时间
        avPacket.duration =
            av_rescale_q_rnd(avPacket.duration, itime, otime,
                             (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        avPacket.pos = -1;

        if (avPacket.pts < lastPts) {
            avPacket.pts = avPacket.dts = lastPts + 1;
        }

        // 视频帧推送速度
        if (ictx->streams[avPacket.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational tb = ictx->streams[avPacket.stream_index]->time_base;
            // 已经过去的时间
            long long now = av_gettime() - startTime;
            long long dts = 0;
            dts = avPacket.dts * (1000 * 1000 * r2d(tb));
            if (dts > now)
                av_usleep(dts - now);
            else {
                cout << "sss";
            }
        }

        lastPts = avPacket.pts;
        // 推送  会自动释放空间 不需要调用av_packet_unref
        ret = av_interleaved_write_frame(octx, &avPacket);
        if (ret < 0) {
            break;
        }
    }

    avio_closep(&octx->pb);
    avformat_free_context(octx);

    /*
    AVFormatContext, ...........
            avformat_open_input(......)
            avformat_find_streaminfo(.....)

            avformat_alloc_output_context2(.....)
            avformat_new_stream(...)
            avcodec_parameters_copy(....)


            avio_open(..)
            avformat_write_header(...)

            while(1){
                    av_read_frame(...)//AVPacket
                    av_interleaved_write_frame(...)

            }
    */

    avformat_network_deinit();
    getchar();
    return 0;
}
