// N.B. mp4extract just extracts tracks/samples from an mp4 file
// For many track types this is insufficient to reconsruct a valid
// elementary stream (ES). Use "mp4creator -extract=<trackId>" if
// you need the ES reconstructed.

#include <mp4v2/mp4v2.h>
#include <iostream>
#include <string>
#include <inttypes.h>

static void DumpTrack(MP4FileHandle mp4file, MP4TrackId tid, const std::string& fileName)
{
    uint32_t numSamples;
    MP4SampleId sid;
    MP4Duration time;
    uint32_t timescale;
    uint64_t msectime;

    uint64_t sectime, mintime, hrtime;

    numSamples = MP4GetTrackNumberOfSamples(mp4file, tid);
    timescale = MP4GetTrackTimeScale(mp4file, tid);
    printf("mp4file %s, track %d, samples %d, timescale %d\n",
           fileName.c_str(), tid, numSamples, timescale);

    for (sid = 1; sid <= numSamples; sid++) {
        time = MP4GetSampleTime(mp4file, tid, sid);
        msectime = time;
        msectime *= UINT64_C(1000);
        msectime /= timescale;
        if (msectime == 0) {
            hrtime = mintime = sectime = UINT64_C(0);
        }
        else {
            hrtime = msectime / UINT64_C(3600000); // 3600 * 1000
            msectime -= hrtime * UINT64_C(3600000);// 3600 * 1000
            mintime = msectime / UINT64_C(60000);// 60 * 1000
            msectime -= (mintime * UINT64_C(60000));// 60 * 1000
            sectime = msectime / UINT64_C(1000);
            msectime -= sectime * UINT64_C(1000);
        }

        printf("sampleId %6d, size %5u duration %8" PRIu64 " time %8" PRIu64 " %02" PRIu64 ":%02" PRIu64 ":%02" PRIu64 ".%03" PRIu64 " %c\n",
               sid, MP4GetSampleSize(mp4file, tid, sid),
               MP4GetSampleDuration(mp4file, tid, sid),
               time, hrtime, mintime, sectime, msectime,
               MP4GetSampleSync(mp4file, tid, sid) == 1 ? 'S' : ' ');
    }
}

int main()
{
    // 直接设置参数，无需命令行解析
    std::string mp4PathName = "/Users/jason/Desktop/test10.mp4";  // 修改为你的MP4文件路径
    MP4TrackId trackId = MP4_INVALID_TRACK_ID;  // 0表示所有轨道，或指定具体轨道ID
    MP4SampleId sampleId = MP4_INVALID_SAMPLE_ID;  // 指定样本ID，或保持INVALID
    MP4LogLevel verbosity = MP4_LOG_ERROR;  // 日志级别

    // 设置日志级别
    MP4LogSetLevel(verbosity);
    if (verbosity) {
        fprintf(stderr, "mp4v2_read version %s\n", MP4V2_PROJECT_version);
    }

    // 提取文件名
    std::string mp4FileName = mp4PathName;
    size_t lastSlash = mp4PathName.find_last_of('/');
    if (lastSlash != std::string::npos) {
        mp4FileName = mp4PathName.substr(lastSlash + 1);
    }

    // 打开MP4文件
    MP4FileHandle mp4File = MP4Read(mp4PathName.c_str());
    if (!mp4File) {
        fprintf(stderr, "Failed to open MP4 file: %s\n", mp4PathName.c_str());
        return -1;
    }

    // 处理特定样本
    if (sampleId != MP4_INVALID_SAMPLE_ID) {
        if (trackId == MP4_INVALID_TRACK_ID) {
            fprintf(stderr, "Must specify track for sample\n");
            return -1;
        }
        if (sampleId > MP4GetTrackNumberOfSamples(mp4File, trackId)) {
            fprintf(stderr, "Sample number %u is past end %u\n",
                    sampleId, MP4GetTrackNumberOfSamples(mp4File, trackId));
            return -1;
        }
        
        uint32_t sample_size = MP4GetTrackMaxSampleSize(mp4File, trackId);
        uint8_t *sample = (uint8_t *)malloc(sample_size);
        MP4Timestamp sampleTime;
        MP4Duration sampleDuration, sampleRenderingOffset;
        uint32_t this_size = sample_size;
        bool isSyncSample;
        
        bool ret = MP4ReadSample(mp4File,
                                trackId,
                                sampleId,
                                &sample,
                                &this_size,
                                &sampleTime,
                                &sampleDuration,
                                &sampleRenderingOffset,
                                &isSyncSample);
        if (ret == false) {
            fprintf(stderr, "Sample read error\n");
            return -1;
        }
        
        printf("Track %u, Sample %u, Length %u\n",
               trackId, sampleId, this_size);

        for (uint32_t ix = 0; ix < this_size; ix++) {
            if ((ix % 16) == 0) printf("\n%04u ", ix);
            printf("%02x ", sample[ix]);
        }
        printf("\n");
        
        free(sample);
    }
    // 处理轨道信息
    else {
        if (trackId == MP4_INVALID_TRACK_ID) {
            // 显示所有轨道
            uint32_t numTracks = MP4GetNumberOfTracks(mp4File);
            printf("Total tracks: %u\n", numTracks);
            
            for (uint32_t i = 0; i < numTracks; i++) {
                trackId = MP4FindTrackId(mp4File, i);
                printf("\n=== Track %u ===\n", trackId);
                DumpTrack(mp4File, trackId, mp4FileName);
            }
        }
        else {
            // 显示指定轨道
            printf("=== Track %u ===\n", trackId);
            DumpTrack(mp4File, trackId, mp4FileName);
        }
    }

    MP4Close(mp4File);
    printf("\nMP4 file analysis completed.\n");
    return 0;
}
