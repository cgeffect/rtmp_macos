// #include <stdlib.h>
// #include <string.h>
// #include <assert.h>
// #include <stdio.h>

// #define FLV_HEADER_SIZE        9 // DataOffset included
// #define FLV_TAG_HEADER_SIZE    11 // StreamID included

// // 根据 tag type的值，tag body可以分为
// #define AUDIODATA 8
// #define VIDEODATA 9
// #define SCRIPTDATAOBJECT 18

// #include "libflv/flv-header.h"
// #include "libflv/flv-parser.h"
// #include "parse_amf.h"

// int main() {

//     FILE *ifh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/test2.flv", "rb");
//     if (ifh == NULL) {
//         printf("Failed to open files!");
//         return -1;
//     }

//     FILE *h264fh = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/flv_unpack.h264", "wb");
//     if (h264fh == NULL) {
//         printf("Failed to open output file!");
//         return -1;
//     }

//     uint32_t sz;
//     uint8_t data[FLV_HEADER_SIZE];
//     struct flv_header_t flv_header;

//     if (FLV_HEADER_SIZE != fread(data, 1, FLV_HEADER_SIZE, ifh))
//         return -1;

//     if(FLV_HEADER_SIZE != flv_header_read(&flv_header, data, FLV_HEADER_SIZE))
//         return -1;

//     assert(flv_header.offset >= FLV_HEADER_SIZE && flv_header.offset < FLV_HEADER_SIZE + 4096);
//     int offset = (int)(flv_header.offset - FLV_HEADER_SIZE);
//     for(int n = offset; n > 0 && n < 4096; n -= sizeof(data)) {
//         size_t size = n >= sizeof(data) ? sizeof(data) : n;
//         uint8_t data[size];
//         fread(data, 1, size, ifh);
// //        flv->read(flv->param, data, n >= sizeof(data) ? sizeof(data) : n); // skip
//     }

//     while (true) {
//         uint8_t prevTagData[4];
//         if (4 != fread(prevTagData, 1, 4, ifh))
//             return -1;

//         flv_tag_size_read(prevTagData, 4, &sz);
//         /*
//          FLV tag由 tag header + tag body组成。tag header固定为11个字节
//          previousTagSize 的大小包含上一个tag的tag header + tag body
//          */
//         printf("previousTagSize: %d\n", sz);

//         uint8_t tagHeaderData[FLV_TAG_HEADER_SIZE];
//         fread(tagHeaderData, 1, FLV_TAG_HEADER_SIZE, ifh);
//         struct flv_tag_header_t tag_header;
//         flv_tag_header_read(&tag_header, tagHeaderData, FLV_TAG_HEADER_SIZE);
//         printf("tag type: %d\n", tag_header.type);
//         // 根据 tag type的值，tag body可以分为AUDIODATA（tag type为8），VIDEODATA（tag type为9），SCRIPTDATAOBJECT（tag type为18）。
//         if (tag_header.type == AUDIODATA) {
//             struct flv_audio_tag_header_t audio = {0};
//             uint8_t data[tag_header.size];
//             int n = flv_audio_tag_header_read(&audio, data, tag_header.size);
//         } else if (tag_header.type == VIDEODATA) {
//             struct flv_video_tag_header_t video = {0};
//             uint8_t data[tag_header.size];
//             int n = flv_video_tag_header_read(&video, data, tag_header.size);
// //                    flv_parser_video(&video, (const uint8_t*)data + n, (int)bytes - n, timestamp, handler, param);
//             printf("%d\n", n);
//         } else if (tag_header.type == SCRIPTDATAOBJECT) {
//             uint8_t flv_tag_body[tag_header.size];
//             size_t body_length = sizeof(flv_tag_body);
//             size_t n = fread(flv_tag_body, 1, body_length, ifh);
//             if (n < 0) {
//                 printf("SCRIPTDATAOBJECT error\n");
//                 return 0;
//             }
//             for (int i = 0; i < body_length; i++) {
//                 // 使用 %02hhx 格式说明符
//                 printf("%02x ", flv_tag_body[i]);
//             }
//             printf("\n");
// //            onMetaData中包含了音视频相关的元数据，封装在Script Data Tag中，它包含了两个AMF。
// //            第一个AMF是固定的值：02 00 0A 6F 6E 4D 65 74 61 44 61 74 61  共13个字节
// //            第1个字节：0x02，表示字符串类型
// //            第2-3个字节：UI16类型，值为0x000A，表示字符串的长度为10（onMetaData的长度）；
// //            第4-13个字节：字符串onMetaData对应的16进制数字（0x6F 0x6E 0x4D 0x65 0x74 0x61 0x44 0x61 0x74 0x61）

// //            第二个AMF
//             flv_metadata_t metadata = {0};
// //            parse_script_tag(flv_tag_body, body_length, &metadata);

//             printf("audiocodecid: %d\n", metadata.audiocodecid);
//             printf("audiodatarate: %f\n", metadata.audiodatarate);
//             printf("audiosamplerate: %d\n", metadata.audiosamplerate);
//             printf("audiosamplesize: %d\n", metadata.audiosamplesize);
//             printf("stereo: %d\n", metadata.stereo);
//             printf("videocodecid: %d\n", metadata.videocodecid);
//             printf("videodatarate: %f\n", metadata.videodatarate);
//             printf("framerate: %f\n", metadata.framerate);
//             printf("duration: %f\n", metadata.duration);
//             printf("interval: %d\n", metadata.interval);
//             printf("width: %d\n", metadata.width);
//             printf("height: %d\n", metadata.height);
//         } else {
//             printf("tag header error\n");
//             return 0;
//         }
//     }
//     return 0;
// }

#include "libflv/flv-demuxer.h"
#include "libflv/flv-reader.h"
#include "libflv/flv-proto.h"
#include <assert.h>
#include <stdio.h>

static unsigned char packet[8 * 1024 * 1024];
static FILE *aac;
static FILE *h264;

inline const char *ftimestamp(uint32_t t, char *buf) {
    sprintf(buf, "%02u:%02u:%02u.%03u", t / 3600000, (t / 60000) % 60, (t / 1000) % 60, t % 1000);
    return buf;
}

inline size_t get_adts_length(const uint8_t *data, size_t bytes) {
    assert(bytes >= 6);
    return ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] >> 5) & 0x07);
}

inline char flv_type(int type) {
    switch (type) {
    case FLV_AUDIO_ASC:
    case FLV_AUDIO_OPUS_HEAD:
        return 'a';
    case FLV_AUDIO_AAC:
    case FLV_AUDIO_MP3:
    case FLV_AUDIO_OPUS:
        return 'A';
    case FLV_VIDEO_AVCC:
    case FLV_VIDEO_HVCC:
    case FLV_VIDEO_VVCC:
    case FLV_VIDEO_AV1C:
        return 'v';
    case FLV_VIDEO_H264:
    case FLV_VIDEO_H265:
    case FLV_VIDEO_H266:
    case FLV_VIDEO_AV1:
        return 'V';
    default: return '*';
    }
}

static int onFLV(void * /*param*/, int codec, const void *data, size_t bytes, uint32_t pts, uint32_t dts, int flags) {
    static char s_pts[64], s_dts[64];
    static uint32_t v_pts = 0, v_dts = 0;
    static uint32_t a_pts = 0, a_dts = 0;

    printf("[%c] pts: %s, dts: %s, %u, cts: %d, bytes: %d ", flv_type(codec), ftimestamp(pts, s_pts), ftimestamp(dts, s_dts), dts, (int)(pts - dts), (int)bytes);

    if (FLV_AUDIO_AAC == codec) {
        printf("diff: %03d/%03d", (int)(pts - a_pts), (int)(dts - a_dts));
        a_pts = pts;
        a_dts = dts;

        assert(bytes == get_adts_length((const uint8_t *)data, bytes));
        fwrite(data, bytes, 1, aac);
    } else if (FLV_VIDEO_H264 == codec || FLV_VIDEO_H265 == codec || FLV_VIDEO_H266 == codec || FLV_VIDEO_AV1 == codec) {
        printf("diff: %03d/%03d %s", (int)(pts - v_pts), (int)(dts - v_dts), flags ? "[I]" : "");
        v_pts = pts;
        v_dts = dts;

        fwrite(data, bytes, 1, h264);
    } else if (FLV_AUDIO_MP3 == codec || FLV_AUDIO_OPUS == codec) {
        fwrite(data, bytes, 1, aac);
    } else if (FLV_AUDIO_ASC == codec || FLV_AUDIO_OPUS_HEAD == codec || FLV_VIDEO_AVCC == codec || FLV_VIDEO_HVCC == codec || FLV_VIDEO_VVCC == codec || FLV_VIDEO_AV1C == codec) {
        // nothing to do
    } else if ((3 << 4) == codec) {
        fwrite(data, bytes, 1, aac);
    } else {
        // nothing to do
        assert(FLV_SCRIPT_METADATA == codec);
    }

    printf("\n");
    return 0;
}

int main() {
    const char* file = "/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/test2.flv";
    aac = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/flv2audio.aac", "wb");
    h264 = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/flv2video.h264", "wb");

    void *reader = flv_reader_create(file);
    flv_demuxer_t *flv = flv_demuxer_create(onFLV, NULL);

    int type, r;
    size_t taglen;
    uint32_t timestamp;
    while (1 == flv_reader_read(reader, &type, &timestamp, &taglen, packet, sizeof(packet))) {
        r = flv_demuxer_input(flv, type, packet, taglen, timestamp);
        if (r < 0) {
            assert(0);
        }
    }

    flv_demuxer_destroy(flv);
    flv_reader_destroy(reader);

    fclose(aac);
    fclose(h264);
}
