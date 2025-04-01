// 解包flv到264, 有问题
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 设置结构体按1字节对齐
#pragma pack(1)

#define TAG_TYPE_SCRIPT 18
#define TAG_TYPE_AUDIO  8
#define TAG_TYPE_VIDEO  9

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct {
    byte Signature[3];
    byte Version;
    byte Flags;
    uint DataOffset;
} FLV_HEADER;

typedef struct {
    byte TagType;           // 包含 Reserved、Filter 和 TagType
    byte DataSize[3];          // 数据大小
    byte Timestamp[3];         // 时间戳
    byte TimestampExtended;    // 时间戳扩展
    byte StreamID[3];          // 流 ID
} TAG_HEADER;

// 将大端字节序转换为小端整数
uint reverse_bytes(byte *p, char c) {
    int r = 0;
    int i;
    for (i = 0; i < c; i++)
        r |= (*(p + i) << (((c - 1) * 8) - 8 * i));
    return r;
}

// 从文件中读取4字节并转换为小端整数
uint read_uint32(FILE* file) {
    byte buffer[4];
    fread(buffer, 1, 4, file);
    return reverse_bytes(buffer, 4);
}

int main() {
    FILE *ifh = NULL, *h264fh = NULL;
    FILE *myout = stdout;

    FLV_HEADER flv_header;
    TAG_HEADER tagheader;
    uint previoustagsize;

    ifh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/test2.flv", "rb");
    if (ifh == NULL) {
        printf("Failed to open files!");
        return -1;
    }

    h264fh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/flv_unpack.h264", "wb");
    if (h264fh == NULL) {
        printf("Failed to open output file!");
        fclose(ifh);
        return -1;
    }

    size_t header_size = sizeof(FLV_HEADER);
    // 读取FLV文件头
    fread((char *)&flv_header, 1, header_size, ifh);

    fprintf(myout, "============== FLV Header ==============\n");
    fprintf(myout, "Signature:  0x %c %c %c\n", flv_header.Signature[0], flv_header.Signature[1], flv_header.Signature[2]);
    fprintf(myout, "Version:    0x %X\n", flv_header.Version);
    fprintf(myout, "Flags  :    0x %X\n", flv_header.Flags);
    fprintf(myout, "HeaderSize: 0x %X\n", reverse_bytes((byte *)&flv_header.DataOffset, sizeof(flv_header.DataOffset)));
    fprintf(myout, "========================================\n");

    // 移动文件指针到文件头之后
    long offset = ftell(ifh);
    fseek(ifh, reverse_bytes((byte *)&flv_header.DataOffset, sizeof(flv_header.DataOffset)), SEEK_SET);

    offset = ftell(ifh);

    // 遍历标签
    while (1) {
        // 读取上一个标签的大小
        if (fread(&previoustagsize, sizeof(uint), 1, ifh) != 1) {
            break;
        }

        // 读取标签头
        fread((void *)&tagheader, sizeof(TAG_HEADER), 1, ifh);
        
        // 解析第一个字节
        byte first_byte = tagheader.TagType;
        byte reserved = (first_byte >> 6) & 0x03; // 保留位
        byte filter = (first_byte >> 5) & 0x01;  // 过滤位
        byte tag_type = first_byte & 0x1F;       // 标签类型

        // 计算数据大小
        uint tagheader_datasize = reverse_bytes(tagheader.DataSize, 3);

        // 计算时间戳
        uint tagheader_timestamp = reverse_bytes(tagheader.Timestamp, 3);
        tagheader_timestamp |= ((uint)tagheader.TimestampExtended << 24);

        // 打印标签信息
        char tagtype_str[10];
        switch (tagheader.TagType) {
        case TAG_TYPE_AUDIO: sprintf(tagtype_str, "AUDIO"); break;
        case TAG_TYPE_VIDEO: sprintf(tagtype_str, "VIDEO"); break;
        case TAG_TYPE_SCRIPT: sprintf(tagtype_str, "SCRIPT"); break;
        default: sprintf(tagtype_str, "UNKNOWN"); break;
        }
        fprintf(myout, "[%6s] %6d %6d |", tagtype_str, tagheader_datasize, tagheader_timestamp);

        // 如果是视频标签
        if (tagheader.TagType == TAG_TYPE_VIDEO) {
            byte video_info_byte;
            fread(&video_info_byte, 1, 1, ifh);

            // 检查视频编码格式是否为 H.264
            if ((video_info_byte & 0x0F) == 7) {
                byte avc_packet_type;
                fread(&avc_packet_type, 1, 1, ifh);

                fprintf(myout, "video_info_byte: 0x%X\n", video_info_byte);
                fprintf(myout, "avc_packet_type: 0x%X\n", avc_packet_type);

                if (avc_packet_type == 1) { // AVC NALU 单元
                    // 跳过组合时间（3 字节）
                    fseek(ifh, 3, SEEK_CUR);

                    // 读取 NALU 单元
                    while (tagheader_datasize - 5 > 0) { // 减去视频信息字节、AVC 包类型字节和组合时间字节
                        uint32_t nalu_length = read_uint32(ifh);
                        tagheader_datasize -= 4;

                        // 读取 NALU 数据
                        byte *nalu_data = (byte *)malloc(nalu_length);
                        fread(nalu_data, 1, nalu_length, ifh);
                        tagheader_datasize -= nalu_length;

                        // 写入 H.264 文件
                        fwrite(nalu_data, 1, nalu_length, h264fh);

                        free(nalu_data);
                    }
                }
            }
        } else if (tagheader.TagType == TAG_TYPE_AUDIO) {
            // 跳过音频数据
            fseek(ifh, tagheader_datasize, SEEK_CUR);
        } else {
            // 跳过其他类型的数据
            fseek(ifh, tagheader_datasize, SEEK_CUR);
        }

        fprintf(myout, "\n");
    }

    fclose(ifh);
    fclose(h264fh);

    return 0;
}
