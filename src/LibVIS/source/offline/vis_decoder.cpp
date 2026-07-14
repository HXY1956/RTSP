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

VIS::VIS_DECODER::StreamInfo
VIS::VIS_DECODER::detectStreamInfo()
{
    StreamInfo result;

    GstDiscoverer* discoverer =
        gst_discoverer_new(5 * GST_SECOND,
                           nullptr);

    if (!discoverer)
        return result;

    gchar* uri =
        gst_filename_to_uri(filepath_.c_str(),
                        nullptr);

    if (!uri)
    {
        g_object_unref(discoverer);
        return result;
    }

    GstDiscovererInfo* info =
        gst_discoverer_discover_uri(
            discoverer,
            uri,
            nullptr);

    g_free(uri);

    if (!info)
    {
        g_object_unref(discoverer);
        return result;
    }

    /*
     * Detect container
     */
    GstDiscovererStreamInfo* stream_info =
        gst_discoverer_info_get_stream_info(info);

    if (stream_info)
    {
        GstCaps* caps =
            gst_discoverer_stream_info_get_caps(
                stream_info);

        if (caps)
        {
            const GstStructure* str =
                gst_caps_get_structure(caps, 0);

            const gchar* name =
                gst_structure_get_name(str);

            std::cout << "Container caps: "
                      << name << std::endl;

            if (g_str_has_prefix(name,
                     "video/quicktime"))
            {
               result.container = "qtdemux";
            }
            else if (g_str_has_prefix(name,
                          "video/x-matroska"))
            {
                result.container = "matroskademux";
            }
            else if (g_str_has_prefix(name,
                          "video/webm"))
            {
                result.container = "matroskademux";
            }
            else if (g_str_has_prefix(name,
                          "video/mpegts"))
            {
                result.container = "tsdemux";
            }
            else if (g_str_has_prefix(name,
                          "video/mpeg"))
            {
                result.container = "mpegpsdemux";
            }
            else if (g_str_has_prefix(name,
                          "video/x-msvideo"))
            {
                result.container = "avidemux";
            }
            else if (g_str_has_prefix(name,
                          "video/x-flv"))
            {
                result.container = "flvdemux";
            }
            else if (g_str_has_prefix(name,
                          "application/ogg"))
            {
                result.container = "oggdemux";
            }

            gst_caps_unref(caps);
        }
    }

    /*
     * Detect codec
     */
    GList* video_streams =
        gst_discoverer_info_get_video_streams(
            info);

    for (GList* l = video_streams;
         l != nullptr;
         l = l->next)
    {
        GstDiscovererStreamInfo* vinfo =
            GST_DISCOVERER_STREAM_INFO(
                l->data);

        GstCaps* caps =
            gst_discoverer_stream_info_get_caps(
                vinfo);

        if (!caps)
            continue;

        const GstStructure* str =
            gst_caps_get_structure(caps, 0);

        const gchar* name =
            gst_structure_get_name(str);

        std::cout << "Video codec caps: "
                  << name << std::endl;

        if (g_str_has_prefix(name,
                     "video/x-h264"))
        {
            result.codec = "h264parse";
        }
        else if (g_str_has_prefix(name,
                          "video/x-h265"))
        {
            result.codec = "h265parse";
        }
        else if (g_str_has_prefix(name,
                          "video/x-vp8"))
        {
            result.codec = "vp8parse";
        }
        else if (g_str_has_prefix(name,
                          "video/x-vp9"))
        {
            result.codec = "vp9parse";
        }
        else if (g_str_has_prefix(name,
                          "video/x-av1"))
        {
            result.codec = "av1parse";
        }
        else if (g_str_has_prefix(name,
                          "image/jpeg"))
        {
            result.codec = "jpegparse";
        }
        else if (g_str_has_prefix(name,
                          "video/mpeg"))
        {
            gint version = 0;

            gst_structure_get_int(str,
                          "mpegversion",
                          &version);

            if (version == 2)
            {
                result.codec =
                    "mpegvideoparse";
            }
            else if (version == 4)
            {
                result.codec =
                    "mpeg4videoparse";
            }
        }

        gst_caps_unref(caps);
    }

    g_list_free(video_streams);

    gst_discoverer_info_unref(info);

    g_object_unref(discoverer);

    return result;
}


bool VIS::VIS_DECODER::Init() {

    StreamInfo info = detectStreamInfo();

    if (info.container.empty())
    {
        std::cerr << "Unsupported container"
              << std::endl;
        return false;
    }

    if (info.codec.empty())
    {    
        std::cerr << "Unsupported codec"
              << std::endl;
        return false;
    }

    if(filepath_=="") return false;

    std::string pipeline =
    "filesrc location=\"" + filepath_ +
    "\" ! " + info.container +
    " name=demux "
    "demux.video_0 ! queue ! " +
    info.codec +
    " ! nvv4l2decoder ! "
    "nvvidconv ! "
    "video/x-raw,format=I420,width=" +
    std::to_string(width) +
    ",height=" +
    std::to_string(height) +
    " ! appsink name=sink "
    "emit-signals=false "
    "sync=true "
    "max-buffers=1 "
    "drop=false";

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


