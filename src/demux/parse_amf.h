#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// 定义 FLV 元数据结构体
typedef struct {
    int audiocodecid;
    double audiodatarate;
    int audiosamplerate;
    int audiosamplesize;
    int stereo;
    int videocodecid;
    double videodatarate;
    double framerate;
    double duration;
    int interval;
    int width;
    int height;
} flv_metadata_t;

// 读取 AMF 字符串
int amf_get_string(const unsigned char **ptr, char *str, int max_len) {
    uint16_t length = (*ptr)[0] << 8 | (*ptr)[1];
    *ptr += 2 + length;
    if (length >= max_len) {
        return -1;
    }
    memcpy(str, *ptr - length, length);
    str[length] = '\0';
    return 0;
}

// 解析 IEEE-754 双精度浮点数
double parse_double(const unsigned char *hex_data) {
    union {
        double d;
        unsigned char bytes[8];
    } u;

    // 确保字节序正确（小端）
    for (int i = 0; i < 8; i++) {
        u.bytes[i] = hex_data[7 - i];
    }

    return u.d;
}

// 解析 Script Tag 数据
void parse_script_tag(const unsigned char *data, size_t data_size, flv_metadata_t *metadata) {
    const unsigned char *ptr = data;
    const unsigned char *end = data + data_size;

    // 跳过 ScriptDataValue 的字符串部分（onMetadata）
    ptr += 2 + 10; // 2 字节长度 + 10 字节字符串

    // 跳过 ECMA 数组的长度
    ptr += 4;

    while (ptr < end) {
        // 解析 PropertyName
        uint16_t name_length = (ptr[0] << 8) | ptr[1];
        ptr += 2 + name_length;

        // 解析 PropertyData
        unsigned char type = *ptr++;
        double value = 0.0;

        if (type == 0x00) { // 双精度浮点数
            value = parse_double(ptr);
            ptr += 8;
        } else if (type == 0x01) { // 布尔值
            value = *ptr++;
        } else if (type == 0x02) { // 字符串
            uint16_t string_length = (ptr[0] << 8) | ptr[1];
            ptr += 2 + string_length;
        } else if (type == 0x03) { // 对象
            // 跳过对象内容，直到遇到 0x09（对象结束标记）
            while (ptr < end && *ptr != 0x09) {
                uint16_t name_length = (ptr[0] << 8) | ptr[1];
                ptr += 2 + name_length;
                type = *ptr++;
                if (type == 0x00) { // 双精度浮点数
                    ptr += 8;
                } else if (type == 0x01) { // 布尔值
                    ptr++;
                } else if (type == 0x02) { // 字符串
                    uint16_t string_length = (ptr[0] << 8) | ptr[1];
                    ptr += 2 + string_length;
                }
            }
            if (ptr < end) {
                ptr++; // 跳过对象结束标记 0x09
            }
        } else if (type == 0x04) { // NULL
            ptr++;
        } else if (type == 0x05) { // 未定义
            ptr++;
        } else if (type == 0x06) { // 引用
            ptr += 2;
        } else if (type == 0x07) { // 混合数组
            // 跳过混合数组内容，直到遇到 0x09（对象结束标记）
            while (ptr < end && *ptr != 0x09) {
                uint16_t name_length = (ptr[0] << 8) | ptr[1];
                ptr += 2 + name_length;
                type = *ptr++;
                if (type == 0x00) { // 双精度浮点数
                    ptr += 8;
                } else if (type == 0x01) { // 布尔值
                    ptr++;
                } else if (type == 0x02) { // 字符串
                    uint16_t string_length = (ptr[0] << 8) | ptr[1];
                    ptr += 2 + string_length;
                }
            }
            if (ptr < end) {
                ptr++; // 跳过对象结束标记 0x09
            }
        } else if (type == 0x08) { // ECMA 数组
            // 跳过 ECMA 数组内容，直到遇到 0x09（对象结束标记）
            while (ptr < end && *ptr != 0x09) {
                uint16_t name_length = (ptr[0] << 8) | ptr[1];
                ptr += 2 + name_length;
                type = *ptr++;
                if (type == 0x00) { // 双精度浮点数
                    ptr += 8;
                } else if (type == 0x01) { // 布尔值
                    ptr++;
                } else if (type == 0x02) { // 字符串
                    uint16_t string_length = (ptr[0] << 8) | ptr[1];
                    ptr += 2 + string_length;
                }
            }
            if (ptr < end) {
                ptr++; // 跳过对象结束标记 0x09
            }
        } else if (type == 0x09) { // 日期
            ptr += 10; // 日期时间戳（8 字节）+ 时区（2 字节）
        } else if (type == 0x0A) { // 长字符串
            uint32_t string_length = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4 + string_length;
        } else if (type == 0x0B) { // XML 文档
            uint32_t string_length = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4 + string_length;
        } else if (type == 0x0C) { // 字节数组
            uint32_t array_length = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            ptr += 4 + array_length;
        }

        // 根据 PropertyName 的内容更新 metadata
        if (name_length == 10 && memcmp(ptr - 10, "onMetadata", 10) == 0) {
            // 跳过 onMetadata 的值
            continue;
        } else if (name_length == 8 && memcmp(ptr - 8, "duration", 8) == 0) {
            metadata->duration = value;
        } else if (name_length == 5 && memcmp(ptr - 5, "width", 5) == 0) {
            metadata->width = (int)value;
        } else if (name_length == 6 && memcmp(ptr - 6, "height", 6) == 0) {
            metadata->height = (int)value;
        } else if (name_length == 13 && memcmp(ptr - 13, "videodatarate", 13) == 0) {
            metadata->videodatarate = value;
        } else if (name_length == 9 && memcmp(ptr - 9, "framerate", 9) == 0) {
            metadata->framerate = value;
        } else if (name_length == 13 && memcmp(ptr - 13, "audiodatarate", 13) == 0) {
            metadata->audiodatarate = value;
        } else if (name_length == 15 && memcmp(ptr - 15, "audiosamplerate", 15) == 0) {
            metadata->audiosamplerate = (int)value;
        } else if (name_length == 15 && memcmp(ptr - 15, "audiosamplesize", 15) == 0) {
            metadata->audiosamplesize = (int)value;
        } else if (name_length == 6 && memcmp(ptr - 6, "stereo", 6) == 0) {
            metadata->stereo = (int)value;
        } else if (name_length == 13 && memcmp(ptr - 13, "audiocodecid", 13) == 0) {
            metadata->audiocodecid = (int)value;
        } else if (name_length == 13 && memcmp(ptr - 13, "videocodecid", 13) == 0) {
            metadata->videocodecid = (int)value;
        } else if (name_length == 8 && memcmp(ptr - 8, "interval", 8) == 0) {
            metadata->interval = (int)value;
        }
    }
}
