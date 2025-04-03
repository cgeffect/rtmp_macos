// 自己实现的解析flv元数据
#pragma once
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define FLV_HEADER_SIZE 9      // DataOffset included
#define FLV_TAG_HEADER_SIZE 11 // StreamID included

// 根据 tag type的值，tag body可以分为
#define AUDIODATA 8
#define VIDEODATA 9
#define SCRIPTDATAOBJECT 18

#include "libflv/flv-header.h"
#include "libflv/flv-parser.h"
#include "demux/parse_amf.h"
#include "libflv/amf0.h"
#include "demux/VideoDemux.h"

void printLog(uint8_t *body, int body_length) {
    for (int i = 0; i < body_length; i++) {
        // 使用 %02hhx 格式说明符
        printf("%02x ", body[i]);
    }
    printf("\n");
}

// 将十六进制数据转换为字符串
char *hex_to_string(unsigned char *hex_data, int length) {
    // 分配足够的内存来存储字符串，包括结尾的 '\0'
    char *str = (char *)malloc(length + 1);
    if (str == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
    }

    // 将每个字节的十六进制数据转换为对应的字符
    for (int i = 0; i < length; i++) {
        str[i] = hex_data[i];
    }

    // 添加字符串结尾的 '\0'
    str[length] = '\0';

    return str;
}
int main() {
    FILE *ifh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/test2.flv", "rb");
    if (ifh == NULL) {
        printf("Failed to open files!");
        return -1;
    }

    FILE *h264fh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/flv_unpack.h264", "wb");
    if (h264fh == NULL) {
        printf("Failed to open output file!");
        return -1;
    }

    // VideoDemux demux;
    uint32_t sz;
    uint8_t data[FLV_HEADER_SIZE];
    struct flv_header_t flv_header;

    if (FLV_HEADER_SIZE != fread(data, 1, FLV_HEADER_SIZE, ifh))
        return -1;

    if (FLV_HEADER_SIZE != flv_header_read(&flv_header, data, FLV_HEADER_SIZE))
        return -1;

    assert(flv_header.offset >= FLV_HEADER_SIZE && flv_header.offset < FLV_HEADER_SIZE + 4096);
    int offset = (int)(flv_header.offset - FLV_HEADER_SIZE);
    for (int n = offset; n > 0 && n < 4096; n -= sizeof(data)) {
        size_t size = n >= sizeof(data) ? sizeof(data) : n;
        uint8_t data[size];
        fread(data, 1, size, ifh);
        //        flv->read(flv->param, data, n >= sizeof(data) ? sizeof(data) : n); // skip
    }

    while (true) {
        uint8_t prevTagData[4];
        if (4 != fread(prevTagData, 1, 4, ifh))
            return -1;

        flv_tag_size_read(prevTagData, 4, &sz);
        /*
         FLV tag由 tag header + tag body组成。tag header固定为11个字节
         previousTagSize 的大小包含上一个tag的tag header + tag body
         */
        printf("previousTagSize: %d\n", sz);

        uint8_t tagHeaderData[FLV_TAG_HEADER_SIZE];
        fread(tagHeaderData, 1, FLV_TAG_HEADER_SIZE, ifh);
        struct flv_tag_header_t tag_header;
        flv_tag_header_read(&tag_header, tagHeaderData, FLV_TAG_HEADER_SIZE);
        printf("tag type: %d\n", tag_header.type);
        // 根据 tag type的值，tag body可以分为AUDIODATA（tag type为8），VIDEODATA（tag type为9），SCRIPTDATAOBJECT（tag type为18）。
        if (tag_header.type == AUDIODATA) {
            struct flv_audio_tag_header_t audio = {0};
            uint8_t data[tag_header.size];
            int n = flv_audio_tag_header_read(&audio, data, tag_header.size);
        } else if (tag_header.type == VIDEODATA) {
            struct flv_video_tag_header_t video = {0};
            uint8_t data[tag_header.size];
            int n = flv_video_tag_header_read(&video, data, tag_header.size);
            //                    flv_parser_video(&video, (const uint8_t*)data + n, (int)bytes - n, timestamp, handler, param);


            printf("%d\n", n);
        } else if (tag_header.type == SCRIPTDATAOBJECT) {
            uint8_t flv_tag_body[tag_header.size];
            size_t body_length = sizeof(flv_tag_body);
            size_t n = fread(flv_tag_body, 1, body_length, ifh);
            if (n < 0) {
                printf("SCRIPTDATAOBJECT error\n");
                return 0;
            }
            for (int i = 0; i < body_length; i++) {
                // 使用 %02hhx 格式说明符
                printf("%02x ", flv_tag_body[i]);
            }
            printf("\n");
            
            int offset = 0;
            while (true) {
                if (flv_tag_body[offset] == 0x00 && flv_tag_body[offset + 1] == 0x00 && flv_tag_body[offset + 2] == 0x09) {
                    break;
                }
                printLog(flv_tag_body + offset, 3);
                printf("offset: %d\n", offset);
                int type = flv_tag_body[offset];
                if (type == 2) {
                    int length = flv_tag_body[offset + 1] << 8 | flv_tag_body[offset + 2];
                    offset += 2;
                    char buffer[64] = {0};
                    char *str = hex_to_string(&flv_tag_body[offset + 1], length);
                    printf("%s\n", str);
                    offset += (1 + length);
                } else if (type == 8) {
                    int length = flv_tag_body[offset + 1] << 24 | flv_tag_body[offset + 2] << 16 | flv_tag_body[offset + 3] << 8 | flv_tag_body[offset + 4];
                    offset += 4;
                    
//                    如果类型是08, 表示数据类型是数组, 00 00 00 19表示的数组的长度, 而不是数组数据长度, 然后每一个数组的key和value又有规则去获取
//                    解析的规则为:
//                    2字节表示key值的长度,    00 08：表示 PropertyName 的长度为 8 字节。
//                    N字节表示 key的值,      64 75 72 61 74 69 6f 6e：表示 PropertyName 的内容为 duration。
//                    1字节表示value类型,     00：表示 PropertyData 的类型为双精度浮点数（AMF0 类型）。
//                    固定8字节表示value的值,  40 41 14 39 58 10 62 4e：表示 PropertyData 的值，这是一个 IEEE-754 双精度浮点数。
//                    一个键值对的结束符,      00：表示该键值对结束。

                    for (int i= 0; i < length; i++) {
                        int key_length = flv_tag_body[offset + 1] << 8 | flv_tag_body[offset + 2];
                        offset += 2;
                        char *str = hex_to_string(&flv_tag_body[offset + 1], key_length);
                        printf("%s: ", str);
                        offset += (1 + key_length);
//                        printLog(flv_tag_body + offset, 1);
                        int valType = flv_tag_body[offset];
                        if (valType == AMF_NUMBER) {
//                            printLog(flv_tag_body + (offset + 1), 8);
                            uint8_t data[8];
                            memcpy(data, flv_tag_body + (offset + 1), 8);
//                            printLog(data, 8);
                            double value = parse_double(data);
                            offset += 8;
                            printf("%f\n", value);
                        } else if (valType == AMF_BOOLEAN) {
                            uint8_t data[1];
                            memcpy(data, flv_tag_body + (offset + 1), 1);
                            int value = data[0];
                            offset += 1;
                            printf("%d\n", value);
                        } else if (valType == AMF_STRING) {
                            int length = flv_tag_body[offset + 1] << 8 | flv_tag_body[offset + 2];
                            offset += 2;
                            char buffer[64] = {0};
                            char *str = hex_to_string(&flv_tag_body[offset + 1], length);
                            printf("%s\n", str);
                            offset += length;
                        }
                    }
                    offset += 1;
                }
            }
//            onMetaData中包含了音视频相关的元数据，封装在Script Data Tag中，它包含了两个AMF。
//            第一个AMF是固定的值：02 00 0A 6F 6E 4D 65 74 61 44 61 74 61  共13个字节
//            第1个字节：0x02，表示字符串类型
//            第2-3个字节：UI16类型，值为0x000A，表示字符串的长度为10（onMetaData的长度）；
//            第4-13个字节：字符串onMetaData对应的16进制数字（0x6F 0x6E 0x4D 0x65 0x74 0x61 0x44 0x61 0x74 0x61）
//
//            第二个AMF
            flv_metadata_t metadata = {0};
            //            parse_script_tag(flv_tag_body, body_length, &metadata);

            printf("audiocodecid: %d\n", metadata.audiocodecid);
            printf("audiodatarate: %f\n", metadata.audiodatarate);
            printf("audiosamplerate: %d\n", metadata.audiosamplerate);
            printf("audiosamplesize: %d\n", metadata.audiosamplesize);
            printf("stereo: %d\n", metadata.stereo);
            printf("videocodecid: %d\n", metadata.videocodecid);
            printf("videodatarate: %f\n", metadata.videodatarate);
            printf("framerate: %f\n", metadata.framerate);
            printf("duration: %f\n", metadata.duration);
            printf("interval: %d\n", metadata.interval);
            printf("width: %d\n", metadata.width);
            printf("height: %d\n", metadata.height);
        } else {
            printf("tag header error\n");
            return 0;
        }
    }
    return 0;
}
