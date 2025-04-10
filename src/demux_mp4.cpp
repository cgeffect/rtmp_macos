#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// 定义 MP4 box 的结构
typedef struct {
    uint32_t size;
    char type[5];
} MP4Box;

typedef struct {
    uint32_t size;
    uint32_t type;
    uint32_t major_brand;
    uint32_t minor_version;
    uint32_t compatible_brands;
} MP4BoxHeader;

// 定义 H.264 和 AAC 的文件指针
FILE *h264_file = NULL;
FILE *aac_file = NULL;

// 打开输出文件
void open_output_files() {
    h264_file = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/output.h264", "wb");
    aac_file = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/output.aac", "wb");
    if (!h264_file || !aac_file) {
        fprintf(stderr, "Failed to open output files\n");
        exit(1);
    }
}

// 关闭输出文件
void close_output_files() {
    if (h264_file) fclose(h264_file);
    if (aac_file) fclose(aac_file);
}

// 读取一个 box 的头部信息
int read_box_header(FILE *file, MP4Box *box) {
    uint8_t header[20];
    size_t ss = fread(header, 1, sizeof(MP4BoxHeader), file);
    MP4BoxHeader *header1 = (MP4BoxHeader *)header;
    if (fread(&box->size, 4, 1, file) != 1) return 0;
    if (fread(box->type, 4, 1, file) != 1) return 0;
    box->type[4] = '\0'; // 确保字符串结尾
    return 1;
}

// 跳过一个 box
void skip_box(FILE *file, MP4Box *box) {
    if (box->size == 0) {
        fseek(file, 0, SEEK_END); // 跳到文件末尾
    } else if (box->size == 1) {
        uint64_t large_size;
        fread(&large_size, 8, 1, file);
        fseek(file, large_size - 8, SEEK_CUR); // 跳过数据部分
    } else {
        fseek(file, box->size - 8, SEEK_CUR); // 跳过数据部分
    }
}

// 解析 stsd box，找到 H.264 和 AAC 的配置
void parse_stsd(FILE *file, uint32_t size) {
    uint32_t entry_count;
    fread(&entry_count, 4, 1, file);

    for (uint32_t i = 0; i < entry_count; i++) {
        MP4Box entry_box;
        read_box_header(file, &entry_box);

        if (strncmp(entry_box.type, "avc1", 4) == 0) {
            // H.264 视频
            uint32_t avc_config_size;
            fread(&avc_config_size, 4, 1, file);
            uint8_t *avc_config = (uint8_t *)malloc(avc_config_size);
            fread(avc_config, avc_config_size, 1, file);

            // 写入 Annex B 格式的 H.264 数据
            fwrite("\x00\x00\x00\x01", 4, 1, h264_file); // 添加起始码
            fwrite(avc_config, avc_config_size, 1, h264_file);

            free(avc_config);
        } else if (strncmp(entry_box.type, "mp4a", 4) == 0) {
            // AAC 音频
            uint32_t esds_size;
            fread(&esds_size, 4, 1, file);
            uint8_t *esds_data = (uint8_t *)malloc(esds_size);
            fread(esds_data, esds_size, 1, file);

            // 写入 AAC 数据
            fwrite(esds_data, esds_size, 1, aac_file);

            free(esds_data);
        } else {
            skip_box(file, &entry_box);
        }
    }
}

// 解析 mdat box，提取 H.264 和 AAC 数据
void parse_mdat(FILE *file, uint32_t size) {
    uint8_t *mdat_data = (uint8_t *)malloc(size);
    fread(mdat_data, size, 1, file);

    // 这里需要根据 stbl 中的索引信息来提取具体的 H.264 和 AAC 数据
    // 为了简化，这里假设 mdat 数据中直接包含 H.264 和 AAC 数据
    // 实际情况下，需要根据 stsc、stsz、stco 等 box 的信息来解析

    fwrite(mdat_data, size, 1, h264_file);
    fwrite(mdat_data, size, 1, aac_file);

    free(mdat_data);
}

// 解析 MP4 文件
void parse_mp4(FILE *file) {
    while (!feof(file)) {
        MP4Box box;
        if (!read_box_header(file, &box)) break;

        if (strncmp(box.type, "ftyp", 4) == 0) {
            // 文件类型 box，跳过
            skip_box(file, &box);
        } else if (strncmp(box.type, "moov", 4) == 0) {
            // 元数据 box，跳过
            skip_box(file, &box);
        } else if (strncmp(box.type, "mdat", 4) == 0) {
            // 媒体数据 box
            parse_mdat(file, box.size - 8);
        } else if (strncmp(box.type, "stbl", 4) == 0) {
            // 样本表 box
            parse_stsd(file, box.size - 8);
        } else {
            // 其他 box，跳过
            skip_box(file, &box);
        }
    }
}

int main(int argc, char *argv[]) {

    FILE *file = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/res/sintel_trailer.mp4", "rb");
    if (!file) {
        fprintf(stderr, "Failed to open input file\n");
        return 1;
    }

    open_output_files();
    parse_mp4(file);
    close_output_files();

    fclose(file);
    return 0;
}

// https://blog.csdn.net/jidushanbojueA/article/details/138411437
// https://cloud.tencent.com/developer/article/1747047
// https://gpac.github.io/mp4box.js/test/filereader.html
