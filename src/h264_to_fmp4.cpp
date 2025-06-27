// 有问题
/**
 * 使用ffmpeg API将H.264文件转换为fMP4格式
 * fMP4 (Fragmented MP4) 支持快速访问和流媒体播放
 */

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
}

#include <iostream>
#include <string>
#include <vector>

class H264ToFMP4Converter {
private:
    AVFormatContext *input_ctx = nullptr;
    AVFormatContext *output_ctx = nullptr;
    AVStream *input_stream = nullptr;
    AVStream *output_stream = nullptr;
    
    int video_stream_index = -1;
    int64_t frame_count = 0;
    int64_t next_pts = 0;
    int64_t frame_duration = 1; // 默认1，后面会根据帧率修正
    
public:
    ~H264ToFMP4Converter() {
        cleanup();
    }
    
    bool convert(const std::string& input_file, const std::string& output_file) {
        std::cout << "Converting " << input_file << " to " << output_file << std::endl;
        
        if (!open_input(input_file)) {
            std::cerr << "Failed to open input file" << std::endl;
            return false;
        }
        
        if (!open_output(output_file)) {
            std::cerr << "Failed to open output file" << std::endl;
            return false;
        }
        
        if (!setup_streams()) {
            std::cerr << "Failed to setup streams" << std::endl;
            return false;
        }
        
        if (!write_header()) {
            std::cerr << "Failed to write header" << std::endl;
            return false;
        }
        
        if (!process_frames()) {
            std::cerr << "Failed to process frames" << std::endl;
            return false;
        }
        
        if (!write_trailer()) {
            std::cerr << "Failed to write trailer" << std::endl;
            return false;
        }
        
        std::cout << "Conversion completed successfully. Total frames: " << frame_count << std::endl;
        return true;
    }
    
private:
    bool open_input(const std::string& filename) {
        if (avformat_open_input(&input_ctx, filename.c_str(), nullptr, nullptr) < 0) {
            std::cerr << "Could not open input file: " << filename << std::endl;
            return false;
        }
        
        if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
            std::cerr << "Could not find stream information" << std::endl;
            return false;
        }
        
        // 查找视频流
        for (unsigned int i = 0; i < input_ctx->nb_streams; i++) {
            if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = i;
                input_stream = input_ctx->streams[i];
                break;
            }
        }
        
        if (video_stream_index == -1) {
            std::cerr << "Could not find video stream" << std::endl;
            return false;
        }
        
        std::cout << "Found video stream at index " << video_stream_index << std::endl;
        std::cout << "Video codec: " << avcodec_get_name(input_stream->codecpar->codec_id) << std::endl;
        std::cout << "Resolution: " << input_stream->codecpar->width << "x" << input_stream->codecpar->height << std::endl;
        std::cout << "Time base: " << input_stream->time_base.num << "/" << input_stream->time_base.den << std::endl;
        
        if (input_stream->avg_frame_rate.num && input_stream->avg_frame_rate.den) {
            frame_duration = av_rescale_q(1, av_inv_q(input_stream->avg_frame_rate), output_stream->time_base);
        } else {
            frame_duration = 1;
        }
        
        return true;
    }
    
    bool open_output(const std::string& filename) {
        if (avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, filename.c_str()) < 0) {
            std::cerr << "Could not create output context" << std::endl;
            return false;
        }
        // 关键：打开输出文件，防止段错误
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&output_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
                std::cerr << "Could not open output file: " << filename << std::endl;
                return false;
            }
        }
        std::cout << "Output context created successfully" << std::endl;
        return true;
    }
    
    bool setup_streams() {
        // 创建输出流
        output_stream = avformat_new_stream(output_ctx, nullptr);
        if (!output_stream) {
            std::cerr << "Could not create output stream" << std::endl;
            return false;
        }
        
        // 复制编解码器参数
        if (avcodec_parameters_copy(output_stream->codecpar, input_stream->codecpar) < 0) {
            std::cerr << "Could not copy codec parameters" << std::endl;
            return false;
        }
        
        // 设置时间基准
        output_stream->time_base = input_stream->time_base;
        
        // 设置流索引
        output_stream->index = 0;
        
        // 确保编解码器ID正确设置
        output_stream->codecpar->codec_id = AV_CODEC_ID_H264;
        
        std::cout << "Stream setup completed" << std::endl;
        std::cout << "Output codec: " << avcodec_get_name(output_stream->codecpar->codec_id) << std::endl;
        std::cout << "Output resolution: " << output_stream->codecpar->width << "x" << output_stream->codecpar->height << std::endl;
        
        return true;
    }
    
    bool write_header() {
        // 写入文件头
        AVDictionary *options = nullptr;
        av_dict_set(&options, "movflags", "frag_keyframe+default_base_moof", 0);
        
        if (avformat_write_header(output_ctx, &options) < 0) {
            std::cerr << "Could not write output file header" << std::endl;
            av_dict_free(&options);
            return false;
        }
        
        av_dict_free(&options);
        std::cout << "File header written" << std::endl;
        return true;
    }
    
    bool process_frames() {
        AVPacket *packet = av_packet_alloc();
        if (!packet) {
            std::cerr << "Could not allocate packet" << std::endl;
            return false;
        }
        
        // 计算每帧duration
        int64_t next_pts = 0;
        int64_t frame_duration = 1;
        AVRational fr = input_stream->avg_frame_rate;
        if (fr.num && fr.den) {
            frame_duration = av_rescale_q(1, av_inv_q(fr), output_stream->time_base);
        } else {
            // fallback: 25fps
            frame_duration = av_rescale_q(1, (AVRational){25,1}, output_stream->time_base);
        }
        
        // 读取并写入所有数据包
        while (av_read_frame(input_ctx, packet) >= 0) {
            if (packet->stream_index == video_stream_index) {
                packet->stream_index = 0;
                // 转换时间戳到输出流的时间基准
                if (packet->pts == AV_NOPTS_VALUE || packet->dts == AV_NOPTS_VALUE) {
                    packet->pts = next_pts;
                    packet->dts = next_pts;
                    packet->duration = frame_duration;
                    next_pts += frame_duration;
                } else {
                    packet->pts = av_rescale_q(packet->pts, input_stream->time_base, output_stream->time_base);
                    packet->dts = av_rescale_q(packet->dts, input_stream->time_base, output_stream->time_base);
                    packet->duration = av_rescale_q(packet->duration, input_stream->time_base, output_stream->time_base);
                    next_pts = packet->pts + packet->duration;
                }
                // 写入数据包
                if (av_interleaved_write_frame(output_ctx, packet) < 0) {
                    std::cerr << "Error writing packet" << std::endl;
                    av_packet_unref(packet);
                    av_packet_free(&packet);
                    return false;
                }
                frame_count++;
                if (frame_count % 100 == 0) {
                    std::cout << "Processed " << frame_count << " frames" << std::endl;
                }
            }
            av_packet_unref(packet);
        }
        av_packet_free(&packet);
        std::cout << "Frame processing completed" << std::endl;
        return true;
    }
    
    bool write_trailer() {
        // 写入文件尾
        if (av_write_trailer(output_ctx) < 0) {
            std::cerr << "Could not write output file trailer" << std::endl;
            return false;
        }
        
        std::cout << "File trailer written" << std::endl;
        return true;
    }
    
    void cleanup() {
        if (input_ctx) {
            avformat_close_input(&input_ctx);
        }
        if (output_ctx) {
            avformat_free_context(output_ctx);
        }
    }
};

int main(int argc, char *argv[]) {
    // if (argc != 3) {
    //     std::cout << "Usage: " << argv[0] << " <input_h264_file> <output_fmp4_file>" << std::endl;
    //     std::cout << "Example: " << argv[0] << " res/test2.h264 res/test2_fmp4.mp4" << std::endl;
    //     return 1;
    // }
    
    std::string input_file = "/Users/jason/Jason/webrtc/native-rtc/rtmp_macos/res/test2.h264";
    std::string output_file = "./fmp4.mp4";
    
    // 初始化ffmpeg
    av_log_set_level(AV_LOG_INFO);
    
    H264ToFMP4Converter converter;
    
    if (!converter.convert(input_file, output_file)) {
        std::cerr << "Conversion failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Conversion completed successfully!" << std::endl;
    return 0;
} 