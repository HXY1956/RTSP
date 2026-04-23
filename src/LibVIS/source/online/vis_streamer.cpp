#include "vis_streamer.h"
#include <type_traits>
#include "base_global.h"

bool VIS::VIS_STREAMER::Init() {
    std::string pipeline;
    std::string frametype = "video/x-raw";
    std::string decoder = " ! ";
    if(zipped == "JPEG"){
        frametype = "image/jpeg";
        decoder = " ! jpegdec ! ";
    } 
     
     pipeline =
    "v4l2src device=" + devicePath_ + " ! " +
    frametype +",width=" + std::to_string(width) + ",height=" + std::to_string(height) +
    ",framerate=" + std::to_string(_framerate) + "/1" + decoder + 
    "queue max-size-buffers=1 leaky=downstream ! "
    "videoconvert ! "
    "video/x-raw,format=" + format + " ! "
    "appsink name=sink emit-signals=true drop=true max-buffers=1 sync=false";
     
    GError* error = nullptr;
    _pipeline = gst_parse_launch(pipeline.c_str(), &error);
    if (!_pipeline) {
        std::cerr << "Failed to open video file: " << error->message << std::endl;
        return false;
    }
    sink = gst_bin_get_by_name(GST_BIN(_pipeline), "sink");
    gst_element_set_state(_pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn ret = gst_element_get_state(
        _pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline failed to go to PLAYING\n");
        return -1;
    }
    return true;
}

bool VIS::VIS_STREAMER::Capture(uint64_t& ImgT, cv::Mat& frame) {
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample) throw("No Valid Sink");
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo _map;
    if (gst_buffer_map(buffer, &_map, GST_MAP_READ)) {
        ImgT = GST_BUFFER_PTS(buffer);
        if (isFirst) {
            BaseTime = duration_cast<nanoseconds>(
                high_resolution_clock::now().time_since_epoch()
            ).count() - ProjectStartTime;
            FirstFrameTime = ImgT;
            isFirst = false;
        }
        ImgT = ImgT - FirstFrameTime + BaseTime;
        frame = cv::Mat(height, width, channels, (void*)_map.data);

        if(outwidth !=  width || outheight != height){
            cv::Mat resized;
            cv::resize(frame, resized, cv::Size(outwidth, outheight));
            frame = resized;
        }
        if(outformat != format){
            cv::Mat recolor;
            if(format == "GRAY8" && outformat == "BGR") cv::cvtColor(frame, recolor, cv::COLOR_GRAY2BGR);
            else if(format == "GRAY8" && outformat == "RGB") cv::cvtColor(frame, recolor, cv::COLOR_GRAY2RGB);
            else if(format == "BGR" && outformat == "GRAY8") cv::cvtColor(frame, recolor, cv::COLOR_BGR2GRAY);
            else if(format == "BGR" && outformat == "RGB") cv::cvtColor(frame, recolor, cv::COLOR_BGR2RGB);
            else if(format == "RGB" && outformat == "GRAY8") cv::cvtColor(frame, recolor, cv::COLOR_RGB2GRAY);
            else if(format == "RGB" && outformat == "BGR") cv::cvtColor(frame, recolor, cv::COLOR_RGB2BGR);
            frame = recolor;
        }

        gst_buffer_unmap(buffer, &_map);
    }
    gst_sample_unref(sample);
    return true;
}