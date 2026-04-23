#ifndef VIS_STREAMER_H
#define VIS_STREAMER_H
#include "vis_decoder.h"

namespace VIS {
    class VIS_STREAMER : public VIS_CODER {
    public:
        explicit VIS_STREAMER(CamInfo _set) : VIS_CODER(&_set) {
            devicePath_ = _set.devicepath;
        }
        bool Init() override;
        bool Capture(uint64_t& ImgT, cv::Mat& f) override;

    private:
        std::string devicePath_;
    };
};

#endif

