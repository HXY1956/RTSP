#include "vis_decoder.h"
#include <type_traits>
#include "base_global.h"

std::map<std::string, std::string> VIS::VIS_CODER::list_capture_devices() {
    std::map<std::string, std::string> result;
    GstDeviceMonitor* monitor = gst_device_monitor_new();
    GstCaps* caps = gst_caps_new_empty_simple("video/x-raw");
    gst_device_monitor_add_filter(monitor, "Video/Source", caps);
    gst_caps_unref(caps);

    gst_device_monitor_start(monitor);
    GList* devices = gst_device_monitor_get_devices(monitor);
    for (GList* l = devices; l != NULL; l = l->next) {
        GstDevice* dev = (GstDevice*)l->data;
        const gchar* name = gst_device_get_display_name(dev);
        if (!name) continue;
        GstStructure* props = gst_device_get_properties(dev);
        if (!props) continue;
        const gchar* path = gst_structure_get_string(props, "device.path");
        if (!path){
            path = gst_structure_get_string(props, "api.v4l2.path");
        }
        if (!path) continue;
        result[name] = std::string(path);
    }
    g_list_free_full(devices, gst_object_unref);
    gst_device_monitor_stop(monitor);
    g_object_unref(monitor);

    return result;
}

void VIS::VIS_CODER::Stop() {
    if (_pipeline) {
        gst_element_set_state(_pipeline, GST_STATE_NULL);
        gst_object_unref(_pipeline);
        _pipeline = nullptr;
    }
}

bool VIS::VIS_DECODER::Init() {
    if(filepath_=="") return false;
    std::string pipeline;
    pipeline =
    "filesrc location=\"" + filepath_ +
    "\" ! qtdemux name=demux "
    "demux.video_0 ! queue ! h264parse ! nvv4l2decoder ! "
    "nvvidconv ! video/x-raw,format=I420,width=" + std::to_string(width) +
    ",height=" + std::to_string(height) +
    " ! appsink name=sink emit-signals=false sync=true max-buffers=1 drop=true";
    
    GError* error = nullptr;
    _pipeline = gst_parse_launch(pipeline.c_str(), &error);
    if (!_pipeline) {
        std::cerr << "Failed to open video file: " << error->message << std::endl;
        return false;
    }
    sink = gst_bin_get_by_name(GST_BIN(_pipeline), "sink");
    gst_element_set_state(_pipeline, GST_STATE_PLAYING);
    
    return true;
}

bool VIS::VIS_DECODER::Capture(uint64_t& ImgT, cv::Mat& frame) {
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample){
        return false;
    }
    if(isFirst){
        GstCaps* caps = gst_sample_get_caps(sample);
        GstStructure* s = gst_caps_get_structure(caps, 0);
        if (gst_structure_get_fraction(s, "framerate", &num, &den)) {
        }
        isFirst = false;
    }
    GstBuffer* buffer = gst_sample_get_buffer(sample);

    GstMapInfo _map;
    if (gst_buffer_map(buffer, &_map, GST_MAP_READ)) {
        ImgT = GST_BUFFER_PTS(buffer);
        cv::Mat yuv(height * 3 / 2, width, CV_8UC1, _map.data);
        if (outformat == "GRAY8") {
            frame = yuv(cv::Rect(0, 0, width, height)).clone();
        } else if (outformat == "BGR") {
            frame = cv::Mat(height, width, CV_8UC3);
            cv::cvtColor(yuv, frame, cv::COLOR_YUV2BGR_I420);
        } else if (outformat == "RGB") {
            frame = cv::Mat(height, width, CV_8UC3);
            cv::cvtColor(yuv, frame, cv::COLOR_YUV2RGB_I420);
        } else {
            gst_buffer_unmap(buffer, &_map);
            gst_sample_unref(sample);
            throw std::runtime_error("Unsupported target_format");
        }
        if(outwidth !=  width || outheight != height){
            cv::Mat resized;
            cv::resize(frame, resized, cv::Size(outwidth, outheight));
            frame = resized;
        }
        gst_buffer_unmap(buffer, &_map);
    }
    gst_sample_unref(sample);
    return true;
}


