#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H
#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
}
#include "base_global.h"
#include "base_mutex.h"
#include <rnnoise.h>

using namespace BASE;
using namespace std::chrono;

namespace AUDIO {
    /**
	*@简介  音频解码器基类，提供初始化、解码和停止功能，并支持音频帧的推送和噪声抑制
    */
    class AUDIO_CODER {
    public:
        AUDIO_CODER(AudioInfo* _set) {
            avformat_network_init(); 
            channels = _set->channels;
            sample_rate = _set->sample_rate;
            sample_size = _set->sample_size;
            noisemode = (_set->noisemode == 0);
        }
        ~AUDIO_CODER() {
            if (swr_in) swr_free(&swr_in);
            if (swr_out) swr_free(&swr_out);
            if (codec_ctx) avcodec_free_context(&codec_ctx);
            if (fmt_ctx) avformat_close_input(&fmt_ctx);
            avformat_network_deinit();
        }
        void setnoise(int noise){
            noisemode = (noise == 0);
        }
        void Stop();

        /**
		*@简介  单帧解码函数，负责从音频流中解码一帧音频数据，并将其存储在内部缓冲区中
        */
        int Decode();

        /**
		*@简介  高通滤波函数，用于对音频信号进行高通滤波处理，以去除低频噪声
        */
        float highpass(float x);

        /**
		*@简介  推送音频帧函数，将解码后的音频帧推送到内部缓冲区中，以便后续处理
        */
        virtual void PushFrame(AVFrame* frame) = 0;

        /**
		*@简介  从内部缓冲区中获取解码后的音频数据，并返回时间戳和音频数据的字节向量
        */
        bool Iget(uint64_t& time, std::vector<uint8_t>& buffer);

        /**
		*@简介  将平面格式的音频数据转换为交错格式，以便在音频播放或处理时使用
        */
        void planar_to_interleaved(std::vector<uint8_t>& planar);

        /**
		*@简介  读取FFmpeg支持的音频设备列表，并返回一个包含设备名称和路径的映射
        */
        static std::map<std::string, std::string> listFFmpegAudioDevices();

        /**
        *@简介  初始化音频解码器，负责设置解码器上下文和相关参数 
        */
        virtual bool Init() = 0;

    protected:
        AVFormatContext* fmt_ctx = nullptr;
        AVCodecContext* codec_ctx = nullptr;
        const AVCodec* decoder = nullptr;
        int audio_stream_index = -1;
        std::mutex audiomutex;
        std::map<uint64_t, std::vector<uint8_t>> frames;
        bool isFirst = true;
        int channels;
        int sample_rate;
        int sample_size;
        bool noisemode;
        uint64_t audio_clock_ns = 0;
        uint64_t audio_base_ns = 0;

        // RNNoise    
        DenoiseState* rnnoise_st = nullptr;
        SwrContext* swr_in = nullptr;
        SwrContext* swr_out = nullptr;
        std::vector<float> rn_buf;
    };

    /**
	*@简介  音频解码器类，继承自AUDIO_CODER，提供对音频文件的解码功能，并支持噪声抑制
    */
    class AUDIO_DECODER: public AUDIO_CODER {
    public:
        AUDIO_DECODER(AudioInfo _set) : AUDIO_CODER(&_set) { 
            filepath = _set.filepath;
        }
        bool Init() override;
        void PushFrame(AVFrame* frame) override;

    private:
        std::string filepath;
    };
}


#endif