// 打包264到flv, 有问题
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// FLV 文件头结构
typedef struct {
    uint8_t signature[3];   // "FLV"，固定值 0x46 0x4C 0x56
    uint8_t version;        // 文件版本，固定值 0x01
    uint8_t type_flag;      // 文件类型标志，0x05 表示包含音频和视频
    uint8_t data_offset[4]; // 头部大小，固定值 0x09
} FLVHeader;

// FLV Tag 头结构
typedef struct {
    uint8_t tag_type;           // 标签类型，0x09 表示视频
    uint8_t data_size[3];       // 数据大小，3 字节
    uint8_t timestamp[3];       // 时间戳，3 字节
    uint8_t timestamp_extended; // 时间戳扩展，1 字节
    uint8_t stream_id[3];       // 流 ID，通常为 0
} FLVTagHeader;

// 写入 FLV 文件头
void write_flv_header(FILE *fp) {
    FLVHeader header = {
        .signature = {'F', 'L', 'V'},
        .version = 1,
        .type_flag = 0x05, // 包含音频和视频
        .data_offset = {0x09, 0x00, 0x00, 0x00}};
    fwrite(&header, sizeof(header), 1, fp);
}

// 写入 FLV Tag
void write_flv_tag(FILE *fp, uint8_t *data, size_t data_size, uint32_t timestamp, uint8_t tag_type) {
    FLVTagHeader tag_header = {
        .tag_type = tag_type, // 0x09 表示视频
        .data_size = {
            (uint8_t)((data_size >> 16) & 0xFF),
            (uint8_t)((data_size >> 8) & 0xFF),
            (uint8_t)(data_size & 0xFF)},
        .timestamp = {(uint8_t)((timestamp >> 16) & 0xFF), (uint8_t)((timestamp >> 8) & 0xFF), (uint8_t)(timestamp & 0xFF)},
        .timestamp_extended = (uint8_t)((timestamp >> 24) & 0xFF),
        .stream_id = {0x00, 0x00, 0x00}};
    fwrite(&tag_header, sizeof(tag_header), 1, fp);
    fwrite(data, data_size, 1, fp);

    // 写入 Previous Tag Size
    uint32_t previous_tag_size = data_size + sizeof(tag_header);
    fwrite(&previous_tag_size, sizeof(previous_tag_size), 1, fp);
}

// 解析 H.264 NALU
int parse_nalu(uint8_t *buffer, size_t buffer_size, uint8_t **nalu_data, size_t *nalu_size) {
    static const uint8_t start_code_3bytes[] = {0x00, 0x00, 0x01};
    static const uint8_t start_code_4bytes[] = {0x00, 0x00, 0x00, 0x01};

    for (size_t i = 0; i < buffer_size - 3; ++i) {
        if (memcmp(buffer + i, start_code_3bytes, 3) == 0 || memcmp(buffer + i, start_code_4bytes, 4) == 0) {
            *nalu_data = buffer + i + (memcmp(buffer + i, start_code_4bytes, 4) == 0 ? 4 : 3);
            *nalu_size = buffer_size - i - (memcmp(buffer + i, start_code_4bytes, 4) == 0 ? 4 : 3);
            return 1;
        }
    }
    return 0;
}

// 封装 SPS 和 PPS 到 FLV
void encapsulate_sps_pps_to_flv(FILE *flv_fp, uint8_t *sps, size_t sps_len, uint8_t *pps, size_t pps_len) {
    uint8_t avc_decoder_configuration_record[1024];
    size_t avc_decoder_configuration_record_size = 0;

    // AVCDecoderConfigurationRecord
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = 0x01;   // Configuration Version
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = sps[1]; // AVC Profile Indication
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = sps[2]; // Profile Compatibility
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = sps[3]; // AVC Level Indication
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = 0xFF;   // Length Size Minus One (always 3)

    // Number of SPS
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = 0xE1;                  // 1 SPS
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = (sps_len >> 8) & 0xFF; // SPS Length
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = sps_len & 0xFF;
    memcpy(&avc_decoder_configuration_record[avc_decoder_configuration_record_size], sps, sps_len);
    avc_decoder_configuration_record_size += sps_len;

    // Number of PPS
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = 0x01;                  // 1 PPS
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = (pps_len >> 8) & 0xFF; // PPS Length
    avc_decoder_configuration_record[avc_decoder_configuration_record_size++] = pps_len & 0xFF;
    memcpy(&avc_decoder_configuration_record[avc_decoder_configuration_record_size], pps, pps_len);
    avc_decoder_configuration_record_size += pps_len;

    // 写入 FLV Tag
    write_flv_tag(flv_fp, avc_decoder_configuration_record, avc_decoder_configuration_record_size, 0, 0x09);
}

// 封装 H.264 数据到 FLV 文件
void encapsulate_h264_to_flv(const char *input_file, const char *output_file, uint8_t *sps, size_t sps_len, uint8_t *pps, size_t pps_len) {
    FILE *h264_fp = fopen(input_file, "rb");
    if (!h264_fp) {
        perror("Failed to open H.264 input file");
        return;
    }

    FILE *flv_fp = fopen(output_file, "wb");
    if (!flv_fp) {
        perror("Failed to open FLV output file");
        fclose(h264_fp);
        return;
    }

    // 写入 FLV 文件头
    write_flv_header(flv_fp);

    // 封装 SPS 和 PPS
    encapsulate_sps_pps_to_flv(flv_fp, sps, sps_len, pps, pps_len);

    uint8_t buffer[4096];
    size_t read_size;
    uint32_t timestamp = 0; // 简单起见，使用固定时间戳

    uint8_t *nalu_data;
    size_t nalu_size;

    while ((read_size = fread(buffer, 1, sizeof(buffer), h264_fp)) > 0) {
        if (parse_nalu(buffer, read_size, &nalu_data, &nalu_size)) {
            // 写入 FLV Tag
            write_flv_tag(flv_fp, nalu_data, nalu_size, timestamp, 0x09); // 0x09 表示视频
            timestamp += 1000;                                            // 每个帧的时间戳增加 1000ms
        }
    }

    fclose(h264_fp);
    fclose(flv_fp);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <input_h264_file> <output_flv_file> <sps_file> <pps_file>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
    const char *sps_file = argv[3];
    const char *pps_file = argv[4];

    // 读取 SPS 和 PPS 数据
    FILE *sps_fp = fopen(sps_file, "rb");
    if (!sps_fp) {
        perror("Failed to open SPS file");
        return 1;
    }
    fseek(sps_fp, 0, SEEK_END);
    size_t sps_len = ftell(sps_fp);
    fseek(sps_fp, 0, SEEK_SET);
    uint8_t *sps = (uint8_t *)malloc(sps_len);
    fread(sps, 1, sps_len, sps_fp);
    fclose(sps_fp);

    FILE *pps_fp = fopen(pps_file, "rb");
    if (!pps_fp) {
        perror("Failed to open PPS file");
        free(sps);
        return 1;
    }
    fseek(pps_fp, 0, SEEK_END);
    size_t pps_len = ftell(pps_fp);
    fseek(pps_fp, 0, SEEK_SET);
    uint8_t *pps = (uint8_t *)malloc(pps_len);
    fread(pps, 1, pps_len, pps_fp);
    fclose(pps_fp);

    // 封装 H.264 数据到 FLV 文件
    encapsulate_h264_to_flv(input_file, output_file, sps, sps_len, pps, pps_len);

    free(sps);
    free(pps);

    return 0;
}
