#pragma once
#include "base/AVTypes.h"
#include <cstdint>
#include <list>
#include <string>

namespace tmms {
namespace mm {
enum AVCPayloadFormat {
    kPayloadFormatUnknowed = 0,
    kPayloadFormatAvcc = 1,
    kPayloadFormatAnnexB = 2,
};
class VideoDemux {
public:
    VideoDemux() = default;
    ~VideoDemux() = default;

    int32_t OnDemux(const char *data, size_t size, std::list<SampleBuf> &outs);
    bool HasIdr() const;
    bool HasAud() const;
    bool HasSpsPps() const;
    VideoCodecID GetCodecID() const {
        return codec_id_;
    }
    int32_t GetCST() const {
        return composition_time_;
    }
    const std::string &GetSPS() const {
        return sps_;
    }
    const std::string &GetPPS() const {
        return pps_;
    }
    void Reset() {
        has_aud_ = false;
        has_idr_ = false;
        has_sps_pps_ = false;
        has_bframe_ = false;
    }
    bool HasBFrame() const {
        return has_bframe_;
    }

private:
    int32_t DemuxAVC(const char *data, size_t size, std::list<SampleBuf> &outs);
    const char *FindAnnexbNalu(const char *p, const char *end);
    int32_t DecodeAVCNaluAnnexb(const char *data, size_t size, std::list<SampleBuf> &outs);
    int32_t DecodeAVCNaluIAvcc(const char *data, size_t size, std::list<SampleBuf> &outs);
    int32_t DecodeAVCSeqHeader(const char *data, size_t size, std::list<SampleBuf> &outs);
    int32_t DecodeAvcNalu(const char *data, size_t size, std::list<SampleBuf> &outs);
    void CheckNaluType(const char *data);
    bool CheckBFrame(const char *data, size_t bytes);

    VideoCodecID codec_id_;
    int32_t composition_time_{0};
    uint8_t config_version_{0};
    uint8_t profile_{0};
    uint8_t profile_com_{0};
    uint8_t level_{0};
    uint8_t nalu_unit_length_{0};
    bool avc_ok_{false};
    std::string sps_;
    std::string pps_;
    AVCPayloadFormat payload_format_{kPayloadFormatUnknowed};
    bool has_aud_{false};
    bool has_idr_{false};
    bool has_sps_pps_{false};
    bool has_bframe_{false};
};
}
} // namespace tmms::mm