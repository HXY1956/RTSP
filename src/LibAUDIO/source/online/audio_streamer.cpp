#include "audio_streamer.h"
#include "base_global.h"

bool AUDIO::AUDIO_STREAMER::Init()
{
    avdevice_register_all();
    const AVInputFormat* input_fmt = av_find_input_format("alsa");
    if (!input_fmt)
        throw std::runtime_error("alsa input format not found.");

    std::ostringstream tmp;
    tmp << device_path;

    std::string dev = tmp.str();

    AVDictionary* options = nullptr;
    av_dict_set(&options, "sample_rate", std::to_string(sample_rate).c_str(), 0);
    av_dict_set(&options, "channels", std::to_string(channels).c_str(), 0);
    av_dict_set(&options, "sample_fmt", "s16", 0);
    av_dict_set(&options, "buffer_size", "1024", 0);    
    av_dict_set(&options, "period_size", "256", 0);    

    if (avformat_open_input(&fmt_ctx, dev.c_str(), const_cast<AVInputFormat*>(input_fmt), &options) < 0){
        std::cerr<<"Failed to open microphone device."<<std::endl;
        return false;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0){
        std::cerr<<"Failed to find stream info."<<std::endl;
        return false;
    }

    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0){
        std::cerr<<"No audio stream found in microphone."<<std::endl;
        return false;
    }

    AVStream* stream = fmt_ctx->streams[audio_stream_index];
    decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder){
        std::cerr<<"Audio decoder not found."<<std::endl;
        return false;
    }

    codec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(codec_ctx, stream->codecpar);

    if (avcodec_open2(codec_ctx, decoder, nullptr) < 0){
        std::cerr<<"Failed to open codec."<<std::endl;
        return false;
    }
    
    AVSampleFormat fmt = AV_SAMPLE_FMT_NONE;
    switch (sample_size) {
        case 1: fmt = AV_SAMPLE_FMT_U8;  break;
        case 2: fmt = AV_SAMPLE_FMT_S16; break;
        case 4: fmt = AV_SAMPLE_FMT_S32; break;
        default:
            throw std::runtime_error("Unsupported sample_size");
    }

    AVChannelLayout mono_layout;
    av_channel_layout_default(&mono_layout, 1);

    int ret = swr_alloc_set_opts2(
        &swr_in,
        &mono_layout, AV_SAMPLE_FMT_FLT, 48000,
        &codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
        0, nullptr
    );

    if (ret < 0){
        std::cerr<<"Failed to allocate SWR context"<<std::endl;
        return false;
    }

    if (swr_init(swr_in) < 0){
        std::cerr<<"Failed to init SWR context"<<std::endl;
        return false;       
    }

    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, channels);

    ret = swr_alloc_set_opts2(
        &swr_out,
        &out_layout, fmt, sample_rate,
        &mono_layout, AV_SAMPLE_FMT_FLT, 48000,
        0, nullptr
    );

    if (ret < 0){
        std::cerr<<"Failed to allocate SWR context"<<std::endl;
        return false;
    }

    if (swr_init(swr_out) < 0){
        std::cerr<<"Failed to init SWR context"<<std::endl;
        return false;       
    }

    rnnoise_st = rnnoise_create(nullptr);

    return true;
}

void AUDIO::AUDIO_STREAMER::PushFrame(AVFrame* frame)
{
    int max_samples = swr_get_out_samples(swr_in, frame->nb_samples);

    std::vector<float> tmp(max_samples);
    uint8_t* out_ptr[1] = { (uint8_t*)tmp.data() };

    int converted = swr_convert(
        swr_in,
        out_ptr,
        max_samples,
        (const uint8_t**)frame->data,
        frame->nb_samples
    );

    if (converted <= 0) return;

    std::vector<float> rn_out_all;

    if(noisemode){
        for (auto& s : tmp) s = highpass(s);
        rn_buf.insert(rn_buf.end(), tmp.begin(), tmp.begin() + converted);
        while (rn_buf.size() >= 480) {
            float out[480];
            rnnoise_process_frame(rnnoise_st, out, rn_buf.data());
            rn_out_all.insert(rn_out_all.end(), out, out + 480);
            rn_buf.erase(rn_buf.begin(), rn_buf.begin() + 480);
        }
    }
    else{
        if(!rn_buf.empty()){
            rn_out_all.insert(rn_out_all.end(), rn_buf.begin(),  rn_buf.end());
            rn_buf.clear();
        }
        rn_out_all.insert(rn_out_all.end(), tmp.begin(), tmp.begin() + converted);
    }

    if (rn_out_all.empty()) return;
    int out_samples = swr_get_out_samples(swr_out, rn_out_all.size());
    int out_bytes = out_samples * channels * sample_size;
    std::vector<uint8_t> buffer(out_bytes);
    const uint8_t* in_ptr[1];
    in_ptr[0] = (const uint8_t*)rn_out_all.data();

    uint8_t* out_ptr2[1] = { buffer.data() };
    int final_samples = swr_convert(
        swr_out,
        out_ptr2,
        out_samples,
        in_ptr,
        rn_out_all.size()
    );

    if (final_samples <= 0) return;

    buffer.resize(final_samples * channels * sample_size);

    int64_t pts_us = av_rescale_q(frame->pts, fmt_ctx->streams[audio_stream_index]->time_base, AVRational{ 1, 1000000 }); 
    int64_t pts_ns = pts_us * 1000; 
    if (isFirst) { 
        uint64_t now_ns = duration_cast<nanoseconds>( high_resolution_clock::now().time_since_epoch() ).count();
        audio_base_ns = now_ns - ProjectStartTime - pts_ns; 
        isFirst = false; 
    } 
    uint64_t upts_ns = pts_ns + audio_base_ns;

    std::lock_guard<std::mutex> lock(audiomutex);
    frames[upts_ns] = std::move(buffer);
}
