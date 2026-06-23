#ifndef VIS_STREAMER_H
#define VIS_STREAMER_H
#include "vis_decoder.h"

namespace VIS {

    /**
	*@简介  在线视频流读取器类，继承自VIS_CODER，提供对视频流的初始化、捕获和关闭功能
    */
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

