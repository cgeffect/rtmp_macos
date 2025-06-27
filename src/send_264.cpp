
/**
 * 使用librtmp发送264文件到服务器
 */

#include <stdio.h>
#include "util/librtmp_send264.h"

static FILE *fp_send1;

// 读文件的回调函数
// we use this callback function to read data from buffer
int read_buffer1(unsigned char *buf, int buf_size) {
    if (!feof(fp_send1)) {
        int true_size = fread(buf, 1, buf_size, fp_send1);
        return true_size;
    } else {
        return -1;
    }
}

int main(int argc, char *argv[]) {
    char *inUrl = (char *)"/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/res/test2.h264";
    // 输出的地址
    char *outUrl = (char *)"rtmp://172.16.184.26:1935/live/test";

    fp_send1 = fopen(inUrl, "rb");

    // 初始化并连接到服务器
    RTMP264_Connect(outUrl);

    // 发送
    RTMP264_Send(read_buffer1);

    // 断开连接并释放相关资源
    RTMP264_Close();

    return 0;
}
