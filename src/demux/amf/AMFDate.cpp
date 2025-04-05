#include "AMFDate.h"
#include "base/MMediaLog.h"
#include "base/BytesReader.h"
using namespace tmms::mm;

AMFDate::AMFDate(const std::string &name) :
    AMFAny(name) {
}
AMFDate::AMFDate() {
}
AMFDate::~AMFDate() {
}

int AMFDate::Decode(const char *data, int size, bool has) {
    if (size < 10) {
        return -1;
    }
    utc_ = BytesReader::ReadUint64T(data);
    data += 8;
    utc_offset_ = BytesReader::ReadUint16T(data);
    return 10;
}
bool AMFDate::IsDate() {
    return true;
}
double AMFDate::Date() {
    return utc_;
}
void AMFDate::Dump() const {
    RTMP_TRACE << "Date: " << name_ << ": " << utc_ << " , " << utc_offset_  << LN;
}
