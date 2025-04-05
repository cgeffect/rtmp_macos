// // https://cloud.tencent.com/developer/article/2021489

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#define ts_path "/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/res/flv2ts.ts" // TS文件的绝对路径

void Read_Ts_Packet(FILE *file_handle, unsigned char *packet_buf, int len);      // 读一个TS流的packet
int parse_TS(unsigned char *buffer, int FileSize);                               // 分析TS流，并找出PAT的PID和PAT的table
void parse_PAT(unsigned char *buffer, int len);                                  // 分析PAT，并找出所含频道的数目和PMT的PID
void pronum_pmtid_printf();                                                      // 打印PMT的PID
unsigned char *Find_PMT(unsigned short pmt_pid);                                 // 找出PMT的table
void parse_PMT(unsigned char *buffer, int len, unsigned short pmt_pid);          // 解析PMT，找出其中的Video和Audio的PID
void printf_program_list();                                                      // 打印PMT table中包含的stream的类型和PID
unsigned char *Find_video_audio(unsigned short program_pid, unsigned char type); // 找出Video或者Audio的table

typedef struct
{
    unsigned short program_num; // program's num
    unsigned short pmt_pid;     //
} PROGRAM;

typedef struct
{
    unsigned char stream_type;
    unsigned short elementary_pid;
} PRO_LIST;

PROGRAM programs[10] = {{0, 0}}; // 用来存储PMT的PID和数量
unsigned int num = 0;            // total program

PRO_LIST program_list[10] = {{0, 0}}; // 用来存储PMT中stream的类型和PID
unsigned int program_list_num = 0;
FILE *file_handle; // 指向TS流的指针
unsigned int FileSize = 0;

int main() {
    unsigned char buffer[188] = {0}; // TS.Packet
    unsigned char *pmt_buffer, *Video_or_Audio_buffer;
    unsigned int i = 0, j = 0, ret = 0;

    pmt_buffer = (unsigned char *)malloc(sizeof(char) * 188); // 给buffer分配空间
    memset(pmt_buffer, 0, sizeof(char) * 188);                // 清空buffer

    Video_or_Audio_buffer = (unsigned char *)malloc(sizeof(char) * 188);
    memset(Video_or_Audio_buffer, 0, sizeof(char) * 188);

    file_handle = fopen(ts_path, "rb+"); // 以二进制方式打开TS文件

    if (NULL == file_handle) // 判断是否打开文件
    {
        perror("fopen");
        printf("open file error!\n");
        return 0;
    } else
        printf("open file success!\n");

    fseek(file_handle, 0, SEEK_END); // 指针file_handle将以SEEK_END位置偏移0个位置，即将指针移动到文件尾
    FileSize = ftell(file_handle);   // 计算file_handle到文件头的偏移字节数，即计算文件的大小

    printf("file size = %d\n", FileSize);
    rewind(file_handle); // equivalent (void) feek(file_handle,0L,SEEK_SET)   将file_handle指针移动到文件头位置
    printf("find PAT begin-------->\n");

    //////////looking for  PAT
    for (i = 0; i < FileSize / 188; i++) {
        Read_Ts_Packet(file_handle, buffer, 188); // 读TS的packet函数，每次读188个字节到buffer
        ret = parse_TS(buffer, 188);              // 解析188个字节的TS's packet，并打印找到的PAT’s table。如果解析成功即找到PAT，则返回 1，否则返回0

        if (ret == 1) {
            break;
        } else {
            printf("There is no PAT table!\n");
        }
    }

    if (ret == 1) {
        parse_PAT(buffer, 188); // 解析PAT，并找出所含频道的数目和PMT的PID
    }

    pronum_pmtid_printf(); // 打印PMT的PID
    rewind(file_handle);
    printf("find PMT begin -------->\n");
    for (i = 0; i < num; i++) {
        pmt_buffer = Find_PMT(programs[i].pmt_pid); // 根据PMT的PID找到PMT's table

        printf("PMT table -------->\n");
        for (j = 0; j < 188; j++) {
            printf("0x%x ", pmt_buffer[j]); // 打印PMT
        }

        if (pmt_buffer) {
            parse_PMT(pmt_buffer, 188, programs[i].pmt_pid); // 解析找到的PMT，得到Video、Audio等的PID
        }
        memset(pmt_buffer, 0, sizeof(char) * 188);

        printf("\n");
    }

    printf_program_list(); // 打印elementary流的PID和type。
    rewind(file_handle);

    printf("find Audio and Video begin-------->\n");
    for (i = 0; i < program_list_num; i++) {
        Video_or_Audio_buffer = Find_video_audio(program_list[i].elementary_pid,
                                                 program_list[i].stream_type); // 根据PID找到elementary流

        printf("the program's PID is 0x%x\n", program_list[i].elementary_pid);
        printf("the program's Table --------->\n");
        for (j = 0; j < 188; j++) {
            printf("0x%x ", Video_or_Audio_buffer[j]); // 打印elementary's table
        }
        memset(Video_or_Audio_buffer, 0, sizeof(char) * 188);
        printf("\n");
    }

    free(pmt_buffer);
    free(Video_or_Audio_buffer);
    pmt_buffer = NULL;
    Video_or_Audio_buffer = NULL;

    fclose(file_handle);
    printf("\n");

    return 0;
}

/**************************************************
 * read one TS packet's data
 * *************************************************/
void Read_Ts_Packet(FILE *file_handle, unsigned char *packet_buf, int len) {
    fread(packet_buf, 188, 1, file_handle);
}
// 解析pid
int parse_TS(unsigned char *buffer, int FileSize) {
    unsigned char *temp = buffer;
    short pat_pid;
    int i = 0;
    // 第一个字节必须是0x47
    if (buffer[0] != 0x47) {
        printf("it's not a ts packet!\n");
        return 0;
    }
    // sysc_byte                      8
    // transport_error_indicator      1
    // payload_unit_start_indicator   1
    // transport_priority             1
    // PID                            13
    while (temp < buffer + FileSize) {
        // 第二个字节的后5位, 加上第三个字节8位共同构成pid
        pat_pid = (temp[1] & 0x1f) << 8 | temp[2];

        if (pat_pid != 0) {
            printf("finding PAT table ....\n");
        } else {
            printf("already  find the PAT table\n");
            printf("pat_pid = 0x%x\n", pat_pid);
            printf("pat table ------->\n");
            for (i = 0; i <= 187; i++) {
                printf("0x%x ", buffer[i]);
            }
            printf("\n");
            return 1;
        }

        temp = temp + 188;
    }

    return 0;
}

/*******************************************************
 * parse PAT table, get the PMT's PID
 * ********************************************************/
void parse_PAT(unsigned char *buffer, int len) {
    unsigned char *temp, *p;
    char adaptation_control;
    int adaptation_length, i = 0;
    unsigned short section_length, prg_No, PMT_Pid;

    temp = buffer;
    adaptation_control = temp[3] & 0x30;

    if (adaptation_control == 0x10)
        temp = buffer + 4 + 1;
    else if (adaptation_control == 0x30) {
        adaptation_length = buffer[4];
        temp = buffer + 4 + 1 + adaptation_length + 1;
    } else {
        return;
    }

    section_length = (temp[1] & 0x0f) << 8 | temp[2];

    p = temp + 1 + section_length;
    temp = temp + 8;

    while (temp < p - 4) {
        prg_No = (temp[0] << 8) | (temp[1]);

        if (prg_No == 0) {
            temp = temp + 4;
            continue;
        } else {
            PMT_Pid = (temp[2] & 0x1f) << 8 | temp[3];

            programs[num].program_num = prg_No;
            programs[num].pmt_pid = PMT_Pid;
            //  printf("pmt_pid is ox%x\n", PMT_Pid);
            num++;

            temp = temp + 4;
        }
    }
}

void pronum_pmtid_printf() {
    unsigned int i;
    printf("PAT table's program_num and PMT's PID:\n");
    for (i = 0; i < num; i++) {
        printf("program_num = 0x%x (%d),PMT_Pid = 0x%x (%d)\n",
               programs[i].program_num, programs[i].program_num,
               programs[i].pmt_pid, programs[i].pmt_pid);
    }
}

void printf_program_list() {
    unsigned int i;
    printf("All PMT Table's program list: \n");

    for (i = 0; i < program_list_num; i++) {
        printf("stream_type = 0x%x, elementary_pid = 0x%x\n", program_list[i].stream_type, program_list[i].elementary_pid);
    }
    printf("\n");
}

unsigned char *Find_PMT(unsigned short pmt_pid) {
    unsigned int i = 0, j = 0;
    int pid;
    unsigned char *buffer;
    buffer = (unsigned char *)malloc(sizeof(char) * 188);
    memset(buffer, 0, sizeof(char) * 188);

    rewind(file_handle);
    for (j = 0; j < FileSize / 188; j++) {
        Read_Ts_Packet(file_handle, buffer, 188);
        if (buffer[0] != 0x47) {
            printf("It's not TS packet !\n");
        } else {
            pid = (buffer[1] & 0x1f) << 8 | buffer[2];
            if (pid == pmt_pid) {
                printf("PMT Table already find!\n");
                return buffer;
            } else
                printf("finding PMT table.......\n");
        }
    }
}

unsigned char *Find_video_audio(unsigned short program_pid, unsigned char type) {
    unsigned int i = 0, j = 0;
    int pid;
    unsigned char *buffer;

    buffer = (unsigned char *)malloc(sizeof(char) * 188);
    memset(buffer, 0, sizeof(char) * 188);
    rewind(file_handle);
    for (j = 0; j < FileSize / 188; j++) {
        Read_Ts_Packet(file_handle, buffer, 188);
        if (buffer[0] != 0x47) {
            printf("It's not TS packet !\n");
        } else {
            pid = (buffer[1] & 0x1f) << 8 | buffer[2];
            if (program_pid == pid) {
                if (type == 0x02)
                    printf("Find a program and this program is Video type!\n");
                else if (type == 0x03)
                    printf("Find a program and this program is Audio type!\n");
                else
                    printf("Find a program but this program is other type !\n");

                return buffer;
            } else
                printf("finding Video or Audio table.....\n ");
        }
    }
}

void parse_PMT(unsigned char *buffer, int len, unsigned short pmt_pid) {
    unsigned char *temp, *p;
    char adaptation_control;
    int adaptation_length, i = 0;
    int program_info_length;
    int ES_info_length;
    unsigned short section_length, pid;
    temp = buffer;

    adaptation_control = temp[3] & 0x30;
    if (adaptation_control == 0x10) {
        temp = buffer + 4 + 1;
    } else if (adaptation_control == 0x30) {
        adaptation_length = buffer[4];
        temp = buffer + 5 + adaptation_length + 1;
    } else
        return;

    section_length = (temp[1] & 0x0f) << 8 | temp[2];
    p = temp + 1 + section_length;

    //  temp = temp + 10;

    program_info_length = (temp[10] & 0x0f) << 8 | temp[11];
    temp = temp + 12 + program_info_length;

    for (; temp < p - 4;) {
        program_list[program_list_num].stream_type = temp[0],
        program_list[program_list_num].elementary_pid = (temp[1] & 0x1f) << 8 | temp[2];
        ES_info_length = (temp[3] & 0x0f) << 8 | temp[4];

        temp = temp + 4 + ES_info_length + 1;

        program_list_num++;
    }
}
