#ifndef AUDIO_STREAMER_H
#define AUDIO_STREAMER_H
#include "audio_decoder.h"

namespace AUDIO {
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