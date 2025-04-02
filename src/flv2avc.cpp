//// 解包flv到264, 有问题
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//// 设置结构体按1字节对齐
//#pragma pack(1)
//
//#define TAG_TYPE_SCRIPT 18
//#define TAG_TYPE_AUDIO  8
//#define TAG_TYPE_VIDEO  9
//
//typedef unsigned char byte;
//typedef unsigned int uint;
//
//typedef struct {
//    byte Signature[3];
//    byte Version;
//    byte Flags;
//    uint DataOffset;
//} FLV_HEADER;
//
//typedef struct {
//    byte TagType;           // 包含 Reserved、Filter 和 TagType
//    byte DataSize[3];          // 数据大小
//    byte Timestamp[3];         // 时间戳
//    byte TimestampExtended;    // 时间戳扩展
//    byte StreamID[3];          // 流 ID
//} TAG_HEADER;
//
//// 将大端字节序转换为小端整数
//uint reverse_bytes(byte *p, char c) {
//    int r = 0;
//    int i;
//    for (i = 0; i < c; i++)
//        r |= (*(p + i) << (((c - 1) * 8) - 8 * i));
//    return r;
//}
//
//// 从文件中读取4字节并转换为小端整数
//uint read_uint32(FILE* file) {
//    byte buffer[4];
//    fread(buffer, 1, 4, file);
//    return reverse_bytes(buffer, 4);
//}
//
//int main() {
//    FILE *ifh = NULL, *h264fh = NULL;
//    FILE *myout = stdout;
//
//    FLV_HEADER flv_header;
//    TAG_HEADER tagheader;
//    uint previoustagsize;
//
//    ifh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/test2.flv", "rb");
//    if (ifh == NULL) {
//        printf("Failed to open files!");
//        return -1;
//    }
//
//    h264fh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/flv_unpack.h264", "wb");
//    if (h264fh == NULL) {
//        printf("Failed to open output file!");
//        fclose(ifh);
//        return -1;
//    }
//
//    size_t header_size = sizeof(FLV_HEADER);
//    // 读取FLV文件头
//    fread((char *)&flv_header, 1, header_size, ifh);
//
//    fprintf(myout, "============== FLV Header ==============\n");
//    fprintf(myout, "Signature:  0x %c %c %c\n", flv_header.Signature[0], flv_header.Signature[1], flv_header.Signature[2]);
//    fprintf(myout, "Version:    0x %X\n", flv_header.Version);
//    fprintf(myout, "Flags  :    0x %X\n", flv_header.Flags);
//    fprintf(myout, "HeaderSize: 0x %X\n", reverse_bytes((byte *)&flv_header.DataOffset, sizeof(flv_header.DataOffset)));
//    fprintf(myout, "========================================\n");
//
//    // 移动文件指针到文件头之后
//    long offset = ftell(ifh);
//    fseek(ifh, reverse_bytes((byte *)&flv_header.DataOffset, sizeof(flv_header.DataOffset)), SEEK_SET);
//
//    offset = ftell(ifh);
//
//    // 遍历标签
//    while (1) {
//        // 读取上一个标签的大小
//        if (fread(&previoustagsize, sizeof(uint), 1, ifh) != 1) {
//            break;
//        }
//
//        // 读取标签头
//        fread((void *)&tagheader, sizeof(TAG_HEADER), 1, ifh);
//        
//        // 解析第一个字节
//        byte first_byte = tagheader.TagType;
//        byte reserved = (first_byte >> 6) & 0x03; // 保留位
//        byte filter = (first_byte >> 5) & 0x01;  // 过滤位
//        byte tag_type = first_byte & 0x1F;       // 标签类型
//
//        // 计算数据大小
//        uint tagheader_datasize = reverse_bytes(tagheader.DataSize, 3);
//
//        // 计算时间戳
//        uint tagheader_timestamp = reverse_bytes(tagheader.Timestamp, 3);
//        tagheader_timestamp |= ((uint)tagheader.TimestampExtended << 24);
//
//        // 打印标签信息
//        char tagtype_str[10];
//        switch (tagheader.TagType) {
//        case TAG_TYPE_AUDIO: sprintf(tagtype_str, "AUDIO"); break;
//        case TAG_TYPE_VIDEO: sprintf(tagtype_str, "VIDEO"); break;
//        case TAG_TYPE_SCRIPT: sprintf(tagtype_str, "SCRIPT"); break;
//        default: sprintf(tagtype_str, "UNKNOWN"); break;
//        }
//        fprintf(myout, "[%6s] %6d %6d |", tagtype_str, tagheader_datasize, tagheader_timestamp);
//
//        // 如果是视频标签
//        if (tagheader.TagType == TAG_TYPE_VIDEO) {
//            byte video_info_byte;
//            fread(&video_info_byte, 1, 1, ifh);
//
//            // 检查视频编码格式是否为 H.264
//            if ((video_info_byte & 0x0F) == 7) {
//                byte avc_packet_type;
//                fread(&avc_packet_type, 1, 1, ifh);
//
//                fprintf(myout, "video_info_byte: 0x%X\n", video_info_byte);
//                fprintf(myout, "avc_packet_type: 0x%X\n", avc_packet_type);
//
//                if (avc_packet_type == 1) { // AVC NALU 单元
//                    // 跳过组合时间（3 字节）
//                    fseek(ifh, 3, SEEK_CUR);
//
//                    // 读取 NALU 单元
//                    while (tagheader_datasize - 5 > 0) { // 减去视频信息字节、AVC 包类型字节和组合时间字节
//                        uint32_t nalu_length = read_uint32(ifh);
//                        tagheader_datasize -= 4;
//
//                        // 读取 NALU 数据
//                        byte *nalu_data = (byte *)malloc(nalu_length);
//                        fread(nalu_data, 1, nalu_length, ifh);
//                        tagheader_datasize -= nalu_length;
//
//                        // 写入 H.264 文件
//                        fwrite(nalu_data, 1, nalu_length, h264fh);
//
//                        free(nalu_data);
//                    }
//                }
//            }
//        } else if (tagheader.TagType == TAG_TYPE_AUDIO) {
//            // 跳过音频数据
//            fseek(ifh, tagheader_datasize, SEEK_CUR);
//        } else {
//            // 跳过其他类型的数据
//            fseek(ifh, tagheader_datasize, SEEK_CUR);
//        }
//
//        fprintf(myout, "\n");
//    }
//
//    fclose(ifh);
//    fclose(h264fh);
//
//    return 0;
//}

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define FLV_HEADER_SIZE        9 // DataOffset included
#define FLV_TAG_HEADER_SIZE    11 // StreamID included

// 根据 tag type的值，tag body可以分为
#define AUDIODATA 8
#define VIDEODATA 9
#define SCRIPTDATAOBJECT 18

#include "libflv/flv-header.h"
#include "libflv/flv-parser.h"
#include "parse_amf.h"

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

    uint32_t sz;
    uint8_t data[FLV_HEADER_SIZE];
    struct flv_header_t flv_header;

    if (FLV_HEADER_SIZE != fread(data, 1, FLV_HEADER_SIZE, ifh))
        return -1;

    if(FLV_HEADER_SIZE != flv_header_read(&flv_header, data, FLV_HEADER_SIZE))
        return -1;

    assert(flv_header.offset >= FLV_HEADER_SIZE && flv_header.offset < FLV_HEADER_SIZE + 4096);
    int offset = (int)(flv_header.offset - FLV_HEADER_SIZE);
    for(int n = offset; n > 0 && n < 4096; n -= sizeof(data)) {
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
//            onMetaData中包含了音视频相关的元数据，封装在Script Data Tag中，它包含了两个AMF。
//            第一个AMF是固定的值：02 00 0A 6F 6E 4D 65 74 61 44 61 74 61  共13个字节
//            第1个字节：0x02，表示字符串类型
//            第2-3个字节：UI16类型，值为0x000A，表示字符串的长度为10（onMetaData的长度）；
//            第4-13个字节：字符串onMetaData对应的16进制数字（0x6F 0x6E 0x4D 0x65 0x74 0x61 0x44 0x61 0x74 0x61）
            
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
