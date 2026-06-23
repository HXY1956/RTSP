#ifndef AUDIO_STREAMER_H
#define AUDIO_STREAMER_H
#include "audio_decoder.h"

namespace AUDIO {
    /**
	*@简介  在线音频流读取器类，继承自AUDIO_CODER，提供对音频流的初始化、解码和关闭功能
    */
    class AUDIO_STREAMER : public AUDIO_CODER {
    public:
        AUDIO_STREAMER(AudioInfo _set) : AUDIO_CODER(&_set) {
            device_path = _set.devicepath;
        }
        bool Init() override;
        void PushFrame(AVFrame* frame) override;
    private:
        std::string device_path;
    };
}


#endif