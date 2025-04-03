#pragma once

#include "base/AVTypes.h"
#include <list>
#include <cstdint>
#include <string>

namespace tmms {
namespace mm {
class AudioDemux {
public:
    AudioDemux() = default;
    ~AudioDemux() = default;

    int32_t OnDemux(const char *data, size_t size, std::list<SampleBuf> &list);
    int32_t GetCodecId() const {
        return sound_format_;
    }
    AACObjectType GetObjectType() const {
        return aac_object_;
    }
    int32_t GetSampleRateIndex() const {
        return aac_sample_rate_;
    }
    uint8_t GetChannel() const {
        return aac_channel_;
    }
    int32_t GetSampleRate() const;
    const std::string &AACSeqHeader() const;

private:
    int32_t DemuxAAC(const char *data, size_t size, std::list<SampleBuf> &list);
    int32_t DemuxMP3(const char *data, size_t size, std::list<SampleBuf> &);
    int32_t DemuxAACSequenceHeader(const char *data, int size);

    int32_t sound_format_;
    int32_t sound_rate_;
    int32_t sound_size_;
    int32_t sound_type_;
    AACObjectType aac_object_;
    int32_t aac_sample_rate_;
    uint8_t aac_channel_;
    bool aac_ok_{false};
    std::string aac_seq_header_;
};
}
} // namespace tmms::mm
