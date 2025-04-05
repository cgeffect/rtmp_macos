
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#pragma pack(1) // 按字节对齐

union littel_endian_size {
    unsigned short int length;
    unsigned char byte[2];
};

struct pack_start_code {
    unsigned char start_code[3];
    unsigned char stream_id[1];
};

/// 参考标准文档
struct program_stream_pack_header {
    pack_start_code PackStart; // 4
    unsigned char Buf[9];      /// 为什么是9？？
    unsigned char stuffinglen;
};

struct program_stream_map {
    pack_start_code PackStart;
    littel_endian_size PackLength; // we mast do exchange
    // program_stream_info_length
    // info
    // elementary_stream_map_length
    // elem
};

struct program_stream_e {
    pack_start_code PackStart;
    littel_endian_size PackLength; // we mast do exchange
    char PackInfo1[2];
    unsigned char stuffing_length;
};

#pragma pack()

int inline ProgramStreamPackHeader(char *Pack, int length, char **NextPack, int *leftlength) {
    // printf("[%s]%x %x %x %x\n", __FUNCTION__, Pack[0], Pack[1], Pack[2], Pack[3]);
    // 通过 00 00 01 ba头的第14个字节的最后3位来确定头部填充了多少字节
    // printf("Here we found PS.PacketHeader...\n");
    program_stream_pack_header *PsHead = (program_stream_pack_header *)Pack;
    unsigned char pack_stuffing_length = PsHead->stuffinglen & '\x07';

    *leftlength = length - sizeof(program_stream_pack_header) - pack_stuffing_length; // 减去头和填充的字节
    *NextPack = Pack + sizeof(program_stream_pack_header) + pack_stuffing_length;

    if (*leftlength < 4) return 0;

    // printf("[%s]2 %x %x %x %x\n", __FUNCTION__, (*NextPack)[0], (*NextPack)[1], (*NextPack)[2], (*NextPack)[3]);

    return *leftlength;
}

inline int ProgramStreamMap(char *Pack, int length, char **NextPack, int *leftlength, char **PayloadData, int *PayloadDataLen) {
    // printf("[%s]%x %x %x %x\n", __FUNCTION__, Pack[0], Pack[1], Pack[2], Pack[3]);
    printf("Here we found PSM...\n");
    program_stream_map *PSMPack = (program_stream_map *)Pack;

    // no payload
    *PayloadData = 0;
    *PayloadDataLen = 0;

    if (length < sizeof(program_stream_map)) return 0;

    littel_endian_size psm_length;
    psm_length.byte[0] = PSMPack->PackLength.byte[1];
    psm_length.byte[1] = PSMPack->PackLength.byte[0];

    *leftlength = length - psm_length.length - sizeof(program_stream_map);

    // printf("[%s]leftlength %d\n", __FUNCTION__, *leftlength);

    if (*leftlength <= 0) return 0;

    *NextPack = Pack + psm_length.length + sizeof(program_stream_map);

    return *leftlength;
}

inline int Pes(char *Pack, int length, char **NextPack, int *leftlength, char **PayloadData, int *PayloadDataLen) {
    // printf("[%s]%x %x %x %x\n", __FUNCTION__, Pack[0], Pack[1], Pack[2], Pack[3]);
    printf("Here we found PES...\n");
    program_stream_e *PSEPack = (program_stream_e *)Pack;

    *PayloadData = 0;
    *PayloadDataLen = 0;

    if (length < sizeof(program_stream_e)) return 0;

    littel_endian_size pse_length;
    pse_length.byte[0] = PSEPack->PackLength.byte[1];
    pse_length.byte[1] = PSEPack->PackLength.byte[0];

    *PayloadDataLen = pse_length.length - 2 - 1 - PSEPack->stuffing_length;
    if (*PayloadDataLen > 0)
        *PayloadData = Pack + sizeof(program_stream_e) + PSEPack->stuffing_length;

    *leftlength = length - pse_length.length - sizeof(pack_start_code) - sizeof(littel_endian_size);

    // printf("[%s]leftlength %d\n", __FUNCTION__, *leftlength);

    if (*leftlength <= 0) return 0;

    *NextPack = Pack + sizeof(pack_start_code) + sizeof(littel_endian_size) + pse_length.length;

    return *leftlength;
}

int inline GetH246FromPs(char *buffer, int length, char **h264Buffer, int *h264length) {
    int leftlength = 0;
    char *NextPack = 0;

    *h264Buffer = buffer;
    *h264length = 0;

    if (ProgramStreamPackHeader(buffer, length, &NextPack, &leftlength) == 0)
        return 0;

    char *PayloadData = NULL;
    int PayloadDataLen = 0;

    while (leftlength >= sizeof(pack_start_code)) {
        PayloadData = NULL;
        PayloadDataLen = 0;

        if (NextPack
            && NextPack[0] == '\x00'
            && NextPack[1] == '\x00'
            && NextPack[2] == '\x01'
            && NextPack[3] == '\xE0') {
            // 接着就是流包，说明是非i帧
            // printf("Here we found AVStream ... \n");
            if (Pes(NextPack, leftlength, &NextPack, &leftlength, &PayloadData, &PayloadDataLen)) {
                if (PayloadDataLen) {
                    memcpy(buffer, PayloadData, PayloadDataLen);
                    buffer += PayloadDataLen;
                    *h264length += PayloadDataLen;
                }
            } else {
                if (PayloadDataLen) {
                    memcpy(buffer, PayloadData, PayloadDataLen);
                    buffer += PayloadDataLen;
                    *h264length += PayloadDataLen;
                }

                break;
            }
        } else if (NextPack
                   && NextPack[0] == '\x00'
                   && NextPack[1] == '\x00'
                   && NextPack[2] == '\x01'
                   && NextPack[3] == '\xBC') {
            if (ProgramStreamMap(NextPack, leftlength, &NextPack, &leftlength, &PayloadData, &PayloadDataLen) == 0)
                break;
        } else {
            // printf("[%s]no konw %x %x %x %x\n", __FUNCTION__, NextPack[0], NextPack[1], NextPack[2], NextPack[3]);
            // break;
            buffer = NextPack;
            length = leftlength;
            if (ProgramStreamPackHeader(buffer, length, &NextPack, &leftlength) == 0)
                return 0;
        }
    }

    return *h264length;
}

#define BUFSIZEPS 1024 * 1024 /// 可以根据实际文件大小来分配
int main() {
    FILE *fpPS001 = fopen("/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/res/flv2ps.ps", "rb");
    char *pbuf = new char[BUFSIZEPS]();
    if (fpPS001) {
        int nFilesize = fread(pbuf, 1, BUFSIZEPS, fpPS001);
        printf("filesize=%d\n", nFilesize);
        int nH264Size = 0;
        char *pbuf264 = new char[nFilesize]();
        GetH246FromPs(pbuf, nFilesize, &pbuf264, &nH264Size);
        printf("nH264Size=%d\n", nH264Size);
        delete[] pbuf264;
        pbuf264 = nullptr;
    }
//    delete[] pbuf;
//    pbuf = nullptr;

    fclose(fpPS001);
    fpPS001 = nullptr;
    return 0;
}
