#include "AMFNull.h"
#include "base/MMediaLog.h"
#include "base/BytesReader.h"
using namespace tmms::mm;

AMFNull::AMFNull(const std::string &name) :
    AMFAny(name) {
}
AMFNull::AMFNull() {
}
AMFNull::~AMFNull() {
}

int AMFNull::Decode(const char *data, int size, bool has) {
    return 0;
}
bool AMFNull::IsNull() {
    return true;
}
void AMFNull::Dump() const {
    RTMP_TRACE << "Null ";
}