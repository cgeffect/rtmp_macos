#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#define MAX_PATH_LENGTH 256
#define BUFFER_SIZE 4096
using namespace std;
// 定义MP4文件头结构
struct MP4FileHeader {
    char fileType[4];
    uint32_t fileSize;
    char fileType2[4];
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <mp4_file>" << endl;
        return 1;
    }
    const char *mp4_file = argv[1];
    int fd = open(mp4_file, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    // 映射文件到内存
    char *buffer = (char *)mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }
    // 解析MP4文件头
    MP4FileHeader *header = (MP4FileHeader *)buffer;
    if (memcmp(header->fileType, "ftyp", 4) != 0 || memcmp(header->fileType2, "moov", 4) != 0) {
        cerr << "Invalid MP4 file" << endl;
        munmap(buffer, BUFFER_SIZE);
        close(fd);
        return 1;
    }
    // 解析MP4文件
    // ...
    // 释放内存映射
    munmap(buffer, BUFFER_SIZE);
    close(fd);
    return 0;
    
}
