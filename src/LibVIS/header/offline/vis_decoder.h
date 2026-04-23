#ifndef VIS_DECODER_H
#define VIS_DECODER_H

#include "base_global.h"
#include "base_mutex.h"
#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

using namespace BASE;
using namespace std::chrono;

namespace VIS {
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

    class VIS_DECODER : public VIS_CODER {
    public:
        explicit VIS_DECODER(CamInfo _set) : VIS_CODER(&_set) {
            filepath_ = _set.filepath;
        };
        bool Init() override;
        bool Capture(uint64_t& ImgT, cv::Mat& f) override;
        void on_pad_added(
            GstElement* src,
            GstPad* pad,
            gpointer user_data);
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


