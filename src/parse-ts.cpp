#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define TS_PACKET_SIZE 188

// TS 包头结构
typedef struct {
    uint8_t sync_byte;
    uint8_t transport_error_indicator : 1;
    uint8_t payload_unit_start_indicator : 1;
    uint8_t transport_priority : 1;
    uint8_t PID : 13;
    uint8_t transport_scrambling_control : 2;
    uint8_t adaptation_field_control : 2;
    uint8_t continuity_counter : 4;
} TSHeader;

// PAT 表结构
typedef struct {
    uint16_t program_number;
    uint16_t program_map_PID;
} PATEntry;

// PMT 表结构
typedef struct {
    uint16_t elementary_PID;
    uint8_t stream_type;
} PMTEntry;

// 解析 TS 包头
void parse_ts_header(const uint8_t *packet, TSHeader *header) {
    header->sync_byte = packet[0];
    header->transport_error_indicator = (packet[1] >> 7) & 0x01;
    header->payload_unit_start_indicator = (packet[1] >> 6) & 0x01;
    header->transport_priority = (packet[1] >> 5) & 0x01;
    header->PID = ((packet[1] & 0x1F) << 8) | packet[2];
    header->transport_scrambling_control = (packet[3] >> 6) & 0x03;
    header->adaptation_field_control = (packet[3] >> 4) & 0x03;
    header->continuity_counter = packet[3] & 0x0F;
}

// 解析 PAT 表
void parse_pat(const uint8_t *packet, PATEntry *pat_entries, int *num_entries) {
    *num_entries = 0;
    int table_id = packet[0];
    if (table_id != 0x00) return; // 确保是 PAT 表

    int section_length = (packet[1] << 8) | packet[2];
    int program_count = (section_length - 9) / 4; // 每个节目占用 4 字节

    for (int i = 0; i < program_count; i++) {
        uint16_t program_number = (packet[8 + i * 4] << 8) | packet[9 + i * 4];
        uint16_t program_map_PID = ((packet[10 + i * 4] & 0x1F) << 8) | packet[11 + i * 4];
        pat_entries[*num_entries].program_number = program_number;
        pat_entries[*num_entries].program_map_PID = program_map_PID;
        (*num_entries)++;
    }
}

// 解析 PMT 表
void parse_pmt(const uint8_t *packet, PMTEntry *pmt_entries, int *num_entries) {
    *num_entries = 0;
    int table_id = packet[0];
    if (table_id != 0x02) return; // 确保是 PMT 表

    int section_length = (packet[1] << 8) | packet[2];
    int info_length = (packet[10] << 8) | packet[11];
    int stream_count = (section_length - 12 - info_length) / 5; // 每个流占用 5 字节

    for (int i = 0; i < stream_count; i++) {
        uint8_t stream_type = packet[12 + info_length + i * 5];
        uint16_t elementary_PID = ((packet[13 + info_length + i * 5] & 0x1F) << 8) | packet[14 + info_length + i * 5];
        pmt_entries[*num_entries].elementary_PID = elementary_PID;
        pmt_entries[*num_entries].stream_type = stream_type;
        (*num_entries)++;
    }
}

// 打印 PSI 表信息
void print_psi_info(const uint8_t *packet, int length) {
    printf("Table ID: 0x%02X\n", packet[0]);
    printf("Section Syntax Indicator: %d\n", (packet[1] >> 7) & 0x01);
    printf("Section Length: %d\n", (packet[1] & 0x0F) << 8 | packet[2]);
    printf("Table ID Extension: 0x%04X\n", (packet[3] << 8) | packet[4]);
    printf("Version Number: %d\n", (packet[5] >> 1) & 0x1F);
    printf("Current Next Indicator: %d\n", packet[5] & 0x01);
    printf("Section Number: %d\n", packet[6]);
    printf("Last Section Number: %d\n", packet[7]);
    printf("CRC32: 0x%08X\n", (packet[length - 4] << 24) | (packet[length - 3] << 16) | (packet[length - 2] << 8) | packet[length - 1]);
}

#define ts_path "/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/res/flv2ts.ts" // TS文件的绝对路径

int main() {
    FILE *file = fopen(ts_path, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    uint8_t packet[TS_PACKET_SIZE];
    TSHeader header;
    PATEntry pat_entries[256];
    PMTEntry pmt_entries[256];
    int num_pat_entries, num_pmt_entries;

    while (fread(packet, 1, TS_PACKET_SIZE, file) == TS_PACKET_SIZE) {
        parse_ts_header(packet, &header);

        if (header.PID == 0x0000) { // PAT 表
            parse_pat(packet, pat_entries, &num_pat_entries);
            printf("PAT Entries:\n");
            for (int i = 0; i < num_pat_entries; i++) {
                printf("Program Number: %d, PMT PID: %d\n", pat_entries[i].program_number, pat_entries[i].program_map_PID);
            }
        }

        for (int i = 0; i < num_pat_entries; i++) {
            if (header.PID == pat_entries[i].program_map_PID) { // PMT 表
                parse_pmt(packet, pmt_entries, &num_pmt_entries);
                printf("PMT Entries for Program Number %d:\n", pat_entries[i].program_number);
                for (int j = 0; j < num_pmt_entries; j++) {
                    printf("Stream Type: %d, Elementary PID: %d\n", pmt_entries[j].stream_type, pmt_entries[j].elementary_PID);
                }
            }
        }

        // 打印其他 PSI 表信息
        if (header.PID != 0x0000 && header.PID != pat_entries[0].program_map_PID) {
            int section_length = (packet[1] << 8) | packet[2];
            print_psi_info(packet, section_length + 3);
        }
    }

    fclose(file);
    return 0;
}
