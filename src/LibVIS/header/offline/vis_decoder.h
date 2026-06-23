#ifndef VIS_DECODER_H
#define VIS_DECODER_H

#include "base_global.h"
#include "base_mutex.h"
#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/pbutils/pbutils.h>

using namespace BASE;
using namespace std::chrono;

namespace VIS {
    /**
	*@简介  视频编解码器基类，提供初始化、捕获和停止功能
    */
    class VIS_CODER {
    public:
        VIS_CODER(CamInfo* _set) {
            outwidth = width = _set->width;
            outheight = height = _set->height;
            outframerate = _framerate = _set->framerate;
            outformat = format = _set->format;
            zipped = _set->zip;
            channels = (format == "GRAY8") ? CV_8UC1 : CV_8UC3;
        };
        ~VIS_CODER() { Stop(); }
        virtual void Stop();
        virtual bool Capture(uint64_t& ImgT, cv::Mat& f) = 0;
        virtual bool Init() = 0;
        void setwidth(int w){outwidth = w;}
        void setheight(int h){outheight = h;}
        void setframerate(int fr){outframerate = fr;}
        void setformat(std::string _format){outformat = _format;}
        static std::map<std::string, std::string> list_capture_devices();

    protected:
        GstElement* _pipeline;
        GstElement* sink;
        cv::Mat frame;
        int64_t BaseTime = 0;
        uint64_t FirstFrameTime = 0;
        bool isFirst = true;
        int width;
        int height;
        int _framerate;
        int channels;
        std::string format;
        std::string zipped;

        int outwidth;
        int outheight;
        int outframerate;
        std::string outformat;
    };

    /**
	*@简介  离线视频解码器类，继承自VIS_CODER，提供对视频文件的解码功能
    */
    class VIS_DECODER : public VIS_CODER {
    public:
        struct StreamInfo
        {
           std::string container;
           std::string codec;
        };

        explicit VIS_DECODER(CamInfo _set) : VIS_CODER(&_set) {
            filepath_ = _set.filepath;
        };

        /**
		*@简介  初始化函数，负责设置GStreamer管道和元素，并启动播放状态
        */
        bool Init() override;

        /**
		*@简介  从视频文件中捕获一帧图像，并返回时间戳和图像数据
        */
        bool Capture(uint64_t& ImgT, cv::Mat& f) override;

        /**
		*@简介  从视频文件中检测流信息，包括容器类型和编解码器类型
        */
        StreamInfo detectStreamInfo();

        int fpsnum(){return num;}
        int fpsden(){return den;}

    private:
        std::string filepath_;
        bool isFirst = true;
        int num = 0;
        int den = 0;
        GstElement* videoconvert = nullptr;
        GstElement* capsfilter   = nullptr;
        GstElement* audio_sink   = nullptr;

    };
};

#endif


