#include "VideoDemux.h"
#include "base/BytesReader.h"
#include "base/NalBitStream.h"
#include <iostream>

#define DEMUX_ERROR std::cout
#define DEMUX_DEBUG std::cout

using namespace tmms::mm;

int32_t VideoDemux::OnDemux(const char *data, size_t size, std::list<SampleBuf> &outs) {
    VideoCodecID id = (VideoCodecID)(*data & 0x0f);
    codec_id_ = id;
    if (id != kVideoCodecIDAVC) {
        DEMUX_ERROR << "not support video type:" << id;
        return -1;
    }
    return DemuxAVC(data, size, outs);
}
bool VideoDemux::HasIdr() const {
    return has_idr_;
}
bool VideoDemux::HasAud() const {
    return has_aud_;
}
bool VideoDemux::HasSpsPps() const {
    return has_sps_pps_;
}
int32_t VideoDemux::DemuxAVC(const char *data, size_t size, std::list<SampleBuf> &outs) {
    uint8_t ftype = (*data & 0xf0) >> 4;
    if (ftype == 5) {
        DEMUX_DEBUG << "igore info frame.";
        return 0;
    }

    uint8_t acv_packet_type = data[1];
    int32_t cst = BytesReader::ReadUint24T(data + 2);
    // DEMUX_DEBUG << "cst:" << cst;
    composition_time_ = cst;
    if (acv_packet_type == 0) {
        return DecodeAVCSeqHeader(data + 5, size - 5, outs);
    } else if (acv_packet_type == 1) {
        return DecodeAvcNalu(data + 5, size - 5, outs);
    } else {
        return 0;
    }
}
const char *VideoDemux::FindAnnexbNalu(const char *p, const char *end) {
    for (p += 2; p + 1 < end; p++) {
        if (*p == 0x01 && *(p - 1) == 0x00 && *(p - 2) == 0x00) {
            return p + 1;
        }
    }
    return end;
}
int32_t VideoDemux::DecodeAVCNaluAnnexb(const char *data, size_t size, std::list<SampleBuf> &outs) {
    if (size < 3) {
        DEMUX_ERROR << "error annexb bytes:" << size;
        return -1;
    }
    int32_t ret = -1;
    const char *data_end = data + size;
    const char *nalu_start = FindAnnexbNalu(data, data_end);
    while (nalu_start < data_end) {
        const char *nalu_next = FindAnnexbNalu(nalu_start + 1, data_end);
        int32_t nalu_size = nalu_next - nalu_start;

        if (nalu_size > size || size <= 0) {
            DEMUX_ERROR << "error annexb nalu bytes:" << size << " nalu size:" << nalu_size;
            return -1;
        }

        ret = 0;
        outs.emplace_back(SampleBuf(data, nalu_size));
        CheckNaluType(nalu_start);
        if (!has_bframe_) {
            has_bframe_ = CheckBFrame(data, nalu_size);
        }
        data += nalu_size;
        size -= nalu_size;

        nalu_start = nalu_next;
    }
    return ret;
}

//https://blog.csdn.net/u014552102/article/details/142860940?spm=1001.2014.3001.5501
//如果是avcc格式sps, pps 存储在 AVCDecoderConfigurationRecord里面
int32_t VideoDemux::DecodeAVCNaluIAvcc(const char *data, size_t size, std::list<SampleBuf> &outs) {
    while (size > 1) {
        uint32_t nalu_size = 0;
        if (nalu_unit_length_ == 3) {
            nalu_size = BytesReader::ReadUint32T(data);
        } else if (nalu_unit_length_ == 1) {
            nalu_size = BytesReader::ReadUint16T(data);
        } else {
            nalu_size = data[0];
        }

        data += nalu_unit_length_ + 1;
        size -= nalu_unit_length_ + 1;

        if (nalu_size > size || size <= 0) {
            DEMUX_ERROR << "error avcc nalu bytes:" << size << " nalu size:" << nalu_size;
            return -1;
        }

        outs.emplace_back(SampleBuf(data, nalu_size));
        CheckNaluType(data);
        if (!has_bframe_) {
            has_bframe_ = CheckBFrame(data, nalu_size);
        }
        data += nalu_size;
        size -= nalu_size;
    }
    return 0;
}
int32_t VideoDemux::DecodeAVCSeqHeader(const char *data, size_t size, std::list<SampleBuf> &outs) {
    if (size < 5) {
        DEMUX_ERROR << "seq header size error.size:" << size;
        return -1;
    }

    config_version_ = data[0];
    profile_ = data[1];
    profile_com_ = data[2];
    level_ = data[3];

    nalu_unit_length_ = data[4] & 0x03;
    DEMUX_DEBUG << "nalu_unit_length:" << nalu_unit_length_;

    data += 5;
    size -= 5;

    if (size < 3) {
        DEMUX_ERROR << "seq header size error.no found sps.";
        return -1;
    }
    int8_t sps_num = data[0] & 0x1F;
    if (sps_num != 1) {
        DEMUX_ERROR << "more than 1 sps.";
        return -1;
    }
    int16_t sps_length = BytesReader::ReadUint16T(data + 1);
    if (sps_length > 0 && sps_length < size - 3) {
        DEMUX_DEBUG << "found sps,bytes:" << sps_length;
    } else {
        DEMUX_ERROR << "sps length error.sps_length:" << sps_length << " size:" << size;
        return -1;
    }
    sps_.assign(data + 3, sps_length);

    data += 3;
    size -= 3;
    data += sps_length;
    size -= sps_length;

    if (size < 3) {
        DEMUX_ERROR << "seq header size error.no found pps.";
        return -1;
    }
    int8_t pps_num = data[0] & 0x1F;
    if (pps_num != 1) {
        DEMUX_ERROR << "more than 1 pps.";
        return -1;
    }
    int16_t pps_length = BytesReader::ReadUint16T(data + 1);
    if (pps_length > 0 && pps_length <= size - 3) {
        DEMUX_DEBUG << "found pps,bytes:" << pps_length;
    } else {
        DEMUX_ERROR << "pps length error.pps_length:" << pps_length << " size:" << size;
        return -1;
    }
    pps_.assign(data + 3, pps_length);
    has_sps_pps_ = true;
    return 0;
}
int32_t VideoDemux::DecodeAvcNalu(const char *data, size_t size, std::list<SampleBuf> &outs) {
    if (payload_format_ == kPayloadFormatUnknowed) {
        if (!DecodeAVCNaluIAvcc(data, size, outs)) {
            payload_format_ = kPayloadFormatAvcc;
        } else {
            if (!DecodeAVCNaluAnnexb(data, size, outs)) {
                payload_format_ = kPayloadFormatAnnexB;
            } else {
                DEMUX_ERROR << "payload format error.no found format.";
                return -1;
            }
        }
    } else if (payload_format_ == kPayloadFormatAvcc) {
        return DecodeAVCNaluIAvcc(data, size, outs);
    } else if (payload_format_ == kPayloadFormatAnnexB) {
        return DecodeAVCNaluAnnexb(data, size, outs);
    }
    return 0;
}
void VideoDemux::CheckNaluType(const char *data) {
    NaluType type = (NaluType)(data[0] & 0x1f);
    if (type == kNaluTypeIDR) {
        has_idr_ = true;
    } else if (type == kNaluTypeAccessUnitDelimiter) {
        has_aud_ = true;
    } else if (type == kNaluTypeSPS || type == kNaluTypePPS) {
        has_sps_pps_ = true;
    }
}
bool VideoDemux::CheckBFrame(const char *data, size_t bytes) {
    int nal_type = *data & 0x1f;
    if (nal_type == 5 || nal_type == 1 || nal_type == 2) {
        data += 1;
        bytes -= 1;

        int offset = 0;
        NalBitStream stream(data, bytes);
        int32_t first_mb_in_slice = stream.GetUE();
        int32_t slice_type = stream.GetUE();

        if (slice_type == 1 || slice_type == 6) {
            return true;
        }
    }
    return false;
}
