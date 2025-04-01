
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_adts_header(int sample_rate, int channels, int profile, uint8_t *adts_header, int aac_length) {
    uint8_t freq_idx = 0; // 0: 96000 Hz  3: 48000 Hz 4: 44100 Hz
    switch (sample_rate) {
    case 96000: freq_idx = 0; break;
    case 88200: freq_idx = 1; break;
    case 64000: freq_idx = 2; break;
    case 48000: freq_idx = 3; break;
    case 44100: freq_idx = 4; break;
    case 32000: freq_idx = 5; break;
    case 24000: freq_idx = 6; break;
    case 22050: freq_idx = 7; break;
    case 16000: freq_idx = 8; break;
    case 12000: freq_idx = 9; break;
    case 11025: freq_idx = 10; break;
    case 8000: freq_idx = 11; break;
    case 7350: freq_idx = 12; break;
    default: freq_idx = 4; break;
    }
    uint8_t chanCfg = channels;
    uint32_t frame_length = aac_length + 7;
    adts_header[0] = 0xFF;
    adts_header[1] = 0xF1;
    adts_header[2] = ((profile) << 6) + (freq_idx << 2) + (chanCfg >> 2);
    adts_header[3] = (((chanCfg & 3) << 6) + (frame_length >> 11));
    adts_header[4] = ((frame_length & 0x7FF) >> 3);
    adts_header[5] = (((frame_length & 7) << 5) + 0x1F);
    adts_header[6] = 0xFC;
}

// 定义ADTS头结构体
typedef struct {
    uint16_t syncword;          // 同步字，固定为0xFFF
    uint8_t id;                 // MPEG版本，0为MPEG-4，1为MPEG-2
    uint8_t layer;              // 层，固定为0
    uint8_t protection_absent;  // 有无CRC校验，0有，1无
    uint8_t profile;            // 音频编码配置文件
    uint8_t sampling_frequency_index; // 采样率索引
    uint8_t private_bit;        // 私有位，固定为0
    uint8_t channel_configuration; // 声道配置
    uint8_t original_copy;      // 原始/拷贝标识，固定为0
    uint8_t home;               // 家庭标识，固定为0
    uint8_t copyright_identification_bit; // 版权标识位，固定为0
    uint8_t copyright_identification_start; // 版权标识开始，固定为0
    uint16_t aac_frame_length;  // AAC帧长度
    uint16_t adts_buffer_fullness; // ADTS缓冲区满度
    uint8_t number_of_raw_data_blocks_in_frame; // 帧内原始数据块数量
} ADTSHeader;

// 解析ADTS头
void parseADTSHeader(const uint8_t *data, ADTSHeader *header) {
    if (data == NULL || header == NULL) return;

    // 解析同步字
    header->syncword = (data[0] << 4) | (data[1] >> 4);
    // 解析MPEG版本
    header->id = (data[1] >> 3) & 0x01;
    // 解析层
    header->layer = (data[1] >> 1) & 0x03;
    // 解析有无CRC校验
    header->protection_absent = data[1] & 0x01;
    // 解析音频编码配置文件
    header->profile = (data[2] >> 6) & 0x03;
    // 解析采样率索引
    header->sampling_frequency_index = (data[2] >> 2) & 0x0F;
    // 解析私有位
    header->private_bit = (data[2] >> 1) & 0x01;
    // 解析声道配置
    header->channel_configuration = ((data[2] & 0x01) << 2) | ((data[3] >> 6) & 0x03);
    // 解析原始/拷贝标识
    header->original_copy = (data[3] >> 5) & 0x01;
    // 解析家庭标识
    header->home = (data[3] >> 4) & 0x01;
    // 解析版权标识位
    header->copyright_identification_bit = (data[3] >> 3) & 0x01;
    // 解析版权标识开始
    header->copyright_identification_start = (data[3] >> 2) & 0x01;
    // 解析AAC帧长度
    header->aac_frame_length = ((data[3] & 0x03) << 11) | (data[4] << 3) | (data[5] >> 5);
    // 解析ADTS缓冲区满度
    header->adts_buffer_fullness = ((data[5] & 0x1F) << 6) | (data[6] >> 2);
    // 解析帧内原始数据块数量
    header->number_of_raw_data_blocks_in_frame = data[6] & 0x03;
}

// 打印ADTS头信息
void printADTSHeader(const ADTSHeader *header) {
    if (header == NULL) return;

    printf("Syncword: 0x%04X\n", header->syncword);
    printf("MPEG Version: %d\n", header->id);
    printf("Layer: %d\n", header->layer);
    printf("Protection Absent: %d\n", header->protection_absent);
    printf("Profile: %d\n", header->profile);
    printf("Sampling Frequency Index: %d\n", header->sampling_frequency_index);
    printf("Channel Configuration: %d\n", header->channel_configuration);
    printf("AAC Frame Length: %d\n", header->aac_frame_length);
    printf("ADTS Buffer Fullness: %d\n", header->adts_buffer_fullness);
    printf("Number of Raw Data Blocks in Frame: %d\n", header->number_of_raw_data_blocks_in_frame);
}