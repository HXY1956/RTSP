#include "audio_decoder.h"
#include "base_global.h"
#include <unistd.h>
#include <alsa/asoundlib.h>
using namespace BASE;

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

bool isVirtualDevice(const std::string& deviceName, const std::string& desc)
{
    std::string lowerName = toLower(deviceName);
    std::string lowerDesc = toLower(desc);

    if (lowerName.find("dmix") == 0 || lowerName.find("dsnoop") == 0) {
        return true;
    }

    if (lowerName.find("u1080p") != std::string::npos || 
        lowerDesc.find("u1080p") != std::string::npos) {
        return true;
    }

    if (lowerName.find("card=") == std::string::npos) {
        return true;
    }

    static const std::vector<std::string> blackListIds = {
        "ape",       // NVIDIA Jetson 内部总线
        "nvda",      // NVIDIA
        "loopback",  // 回环
        "dummy",     // 虚拟
        "virtual",
        "generic",
        "usbstream"  
    };

    size_t pos = lowerName.find("card=");
    if (pos != std::string::npos) {
        size_t start = pos + 5;
        size_t end = lowerName.find_first_of(",:", start);
        std::string cardId = lowerName.substr(start, end - start);
        
        for (const auto& v : blackListIds) {
            if (cardId == v) return true;
        }
    }

    static const std::vector<std::string> playbackOnlyKeywords = {
        "playback", "output", "speaker", "front", "surround", "iec958"
    };
    
    for (const auto& keyword : playbackOnlyKeywords) {
        if (lowerDesc.find(keyword) != std::string::npos) {
            if (lowerDesc.find("input") == std::string::npos && 
                lowerDesc.find("mic") == std::string::npos) {
                return true;
            }
        }
    }

    return false;
}

bool canCapture(const std::string& devName)
{
    int old_level = av_log_get_level();
    av_log_set_level(AV_LOG_QUIET); 
    AVFormatContext* ctx = nullptr;
    const AVInputFormat* fmt = av_find_input_format("alsa");

    int ret = avformat_open_input(
        &ctx,
        devName.c_str(),
        const_cast<AVInputFormat*>(fmt),
        nullptr
    );

    if (ret < 0) {
        av_log_set_level(old_level);
        return false;
    }

    ret = avformat_find_stream_info(ctx, nullptr);
    avformat_close_input(&ctx);

    av_log_set_level(old_level);
    return ret >= 0;
}

float AUDIO::AUDIO_CODER::highpass(float x) {
    static float prev_y = 0;
    static float prev_x = 0;

    float alpha = 0.95;

    float y = alpha * (prev_y + x - prev_x);

    prev_x = x;
    prev_y = y;

    return y;
}

std::map<std::string, std::string> AUDIO::AUDIO_CODER::listFFmpegAudioDevices()
{
    av_log_set_level(AV_LOG_FATAL);
    std::map<std::string, std::string> result;

    if (access("/dev/snd/", F_OK) != 0)
        return result;

    avdevice_register_all();
    const AVInputFormat* iformat = av_find_input_format("alsa");
    if (!iformat) {
        std::cerr << "alsa format not found!" << std::endl;
        return result;
    }
    AVDeviceInfoList* devList = nullptr;
    int ret = 0;
    try{
        ret = avdevice_list_input_sources(const_cast<AVInputFormat*>(iformat), NULL, NULL, &devList);
    }
    catch(...){
        std::cerr << "Failed to list audio devices." << std::endl;
        return result;
    }

    if (ret < 0 || !devList) return result;
    for (int i = 0; i < devList->nb_devices; i++) {
        AVDeviceInfo* dev = devList->devices[i];
        std::string deviceName = dev->device_name;          
        std::string desc       = dev->device_description;
        if (isVirtualDevice(deviceName, desc)) continue;
        if (canCapture(deviceName)) {
            result[desc] = deviceName;
        }
        
    }

    avdevice_free_list_devices(&devList);
    return result;
}

void AUDIO::AUDIO_CODER::planar_to_interleaved(std::vector<uint8_t>& planar)
{
    std::vector<uint8_t> interleaved;
    if (channels <= 1) {
        interleaved = planar;
        return;
    }

    size_t frames_per_channel = planar.size() / (channels * sample_size);
    interleaved.resize(frames_per_channel * channels * sample_size);

    for (size_t f = 0; f < frames_per_channel; f++) {
        for (int ch = 0; ch < channels; ch++) {
            memcpy(
                &interleaved[(f * channels + ch) * sample_size],
                &planar[(ch * frames_per_channel + f) * sample_size],
                sample_size
            );
        }
    }
    planar = interleaved;
}

void AUDIO::AUDIO_CODER::Stop() {
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    avcodec_send_packet(codec_ctx, nullptr);
    while (avcodec_receive_frame(codec_ctx, frame) == 0) {
        PushFrame(frame);
    }
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

bool AUDIO::AUDIO_CODER::Iget(uint64_t& time, std::vector<uint8_t>& buffer) {
    audiomutex.lock();
    if (frames.empty()) {
        audiomutex.unlock();
        return false;
    }
    auto node = frames.extract(frames.begin());
    time = node.key();
    buffer = std::move(node.mapped());
    //planar_to_interleaved(buffer);
    audiomutex.unlock();
    return true;
}

int AUDIO::AUDIO_CODER::Decode() {
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    if (av_read_frame(fmt_ctx, pkt) < 0){
        av_frame_free(&frame);
        av_packet_free(&pkt);
        return -1;
    }

    if (pkt->stream_index != audio_stream_index) {
        av_packet_unref(pkt);
        av_frame_free(&frame);
        av_packet_free(&pkt);
        return 1;
    }
    if (avcodec_send_packet(codec_ctx, pkt) < 0) {
        av_packet_unref(pkt);
        av_frame_free(&frame);
        av_packet_free(&pkt);
        return 1;
    }
    av_packet_unref(pkt);

    while (avcodec_receive_frame(codec_ctx, frame) == 0) {
        PushFrame(frame);
        av_frame_unref(frame);
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}

void AUDIO::AUDIO_DECODER::PushFrame(AVFrame* frame) {
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

    uint64_t pts_us = av_rescale_q(frame->pts, fmt_ctx->streams[audio_stream_index]->time_base, AVRational{ 1, 1000000 });
    uint64_t pts_ns = pts_us * 1000;

    audiomutex.lock();
    frames[pts_ns] = buffer;
    audiomutex.unlock();
}

bool AUDIO::AUDIO_DECODER::Init() {
    if (avformat_open_input(&fmt_ctx, filepath.c_str(), nullptr, nullptr) < 0)
        throw std::runtime_error("Failed to open audio file.");
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0)
        throw std::runtime_error("Failed to find stream info.");

    audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_stream_index < 0)
        return false;

    AVStream* stream = fmt_ctx->streams[audio_stream_index];
    decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder)
        throw std::runtime_error("Audio decoder not found.");

    codec_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(codec_ctx, stream->codecpar);

    if (avcodec_open2(codec_ctx, decoder, nullptr) < 0)
        throw std::runtime_error("Failed to open codec.");

    AVSampleFormat fmt;
    if (sample_size == 1) fmt = AV_SAMPLE_FMT_U8;
    if (sample_size == 2) fmt = AV_SAMPLE_FMT_S16;
    if (sample_size == 4) fmt = AV_SAMPLE_FMT_S32;

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