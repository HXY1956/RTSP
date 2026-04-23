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
        int Decode();
        float highpass(float x);
        virtual void PushFrame(AVFrame* frame) = 0;
        bool Iget(uint64_t& time, std::vector<uint8_t>& buffer);
        void planar_to_interleaved(std::vector<uint8_t>& planar);
        static std::map<std::string, std::string> listFFmpegAudioDevices();
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