#pragma once

#include <string>
#include <memory>
#include <cstring>
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        enum
        {
            kPacketTypeVideo = 1,
            kPacketTypeAudio = 2,
            kPacketTypeMeta = 4,
            kPacketTypeMeta3 = 8,
            kFrameTypeKeyFrame = 16,
            kFrameTypeIDR = 32,
            kPacketTypeUnknowed = 255,
        };
        class Packet;
        using PacketPtr = std::shared_ptr<Packet>;
#pragma pack(push)
#pragma pack(1)
        class Packet
        {
        public:
            Packet(int32_t size)
            :capacity_(size)
            {

            }
            ~Packet() {}
            static PacketPtr NewPacket(int32_t size);

            bool IsVideo() const
            {
                return (type_&kPacketTypeVideo)==kPacketTypeVideo;
            }
            bool IsKeyFrame() const
            {
                return ((type_&kPacketTypeVideo)==kPacketTypeVideo)
                        &&(type_&kFrameTypeKeyFrame)==kFrameTypeKeyFrame;
            }
            bool IsAudio() const
            {
                return type_ == kPacketTypeAudio;
            }
            bool IsMeta() const
            {
                return type_ == kPacketTypeMeta;
            }
            bool IsMeta3() const
            {
                return type_ == kPacketTypeMeta3;
            }

            inline int32_t PacketSize() const
            {
                return size_;
            }
            inline int Space() const
            {
                return capacity_ - size_;
            }
            inline void SetPacketSize(size_t len)
            {
                size_ = len;
            }
            inline void UpdatePacketSize(size_t len)
            {
                size_ += len;
            }
            void SetIndex(int32_t index)
            {
                index_ = index;
            }
            int32_t Index() const
            {
                return index_;
            }
            void SetPacketType(int32_t type)
            {
                type_ = type;
            }
            int32_t PacketType() const
            {
                return type_;
            }
            void SetTimeStamp(uint64_t timestamp)
            {
                timestamp_ = timestamp;
            }
            uint64_t TimeStamp() const
            {
                return timestamp_;
            }
            inline char *Data()
            {
                return (char*)this+sizeof(Packet);
            }

            template <typename T>            
            inline std::shared_ptr<T> Ext() const
            {
                return std::static_pointer_cast<T>(ext_);
            }
            inline void SetExt(const std::shared_ptr<void> &ext)
            {
                ext_ = ext;
            }

        private:
            int32_t type_{kPacketTypeUnknowed};
            uint32_t size_{0};
            int32_t index_{-1};
            uint64_t timestamp_{0};
            uint32_t capacity_{0};
            std::shared_ptr<void> ext_;
        };
#pragma pack()        
    }
}