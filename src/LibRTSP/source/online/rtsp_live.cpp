#include "rtsp_live.h"

namespace RTSP {

RTSP_BASE::RTSP_BASE(AudioInfo* Aset, CamInfo* Cset, RtspInfo* Rset) {
    framerate = Cset->framerate;
    Isize.first = Cset->width;
    Isize.second = Cset->height;
    format = Cset->format;
    sample_rate = Aset->sample_rate;
    sample_size = Aset->sample_size;
    channels = Aset->channels;
    IP = Rset->IP;
    port = Rset->port;
    suffix = Rset->suffix;
    if(Rset->vformat == "H264") zipF = zip_format::H264;
    else if(Rset->vformat == "H265") zipF = zip_format::H265;
    else if(Rset->vformat == "RAW") zipF = zip_format::RAW;
}

void RTSP_BASE::push_video_frame(const cv::Mat& mat, const uint64_t& pts_ns) {
    curr_pts_video = pts_ns;

    if (paused || should_stop || !video_src || !GST_IS_APP_SRC(video_src))
        return;

    size_t buf_size = mat.total() * mat.elemSize();
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, buf_size, nullptr);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        memcpy(map.data, mat.data, buf_size);
        gst_buffer_unmap(buffer, &map);
    } else {
        gst_buffer_unref(buffer);
        std::cerr << "gst_buffer_map failed for video\n";
        return;
    }

    uint64_t _pts_ns = (pts_ns > stream_start_pts) ? (pts_ns - stream_start_pts) : 0;
    GST_BUFFER_PTS(buffer) = (GstClockTime)_pts_ns;
    GST_BUFFER_DTS(buffer) = (GstClockTime)_pts_ns;
    GST_BUFFER_DURATION(buffer) = (gint64)(GST_SECOND / framerate);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(video_src), buffer);
    if (ret != GST_FLOW_OK) {
        std::cerr << "Origin Failed One Epoch\n";
    }
}

void RTSP_BASE::push_smoke_frame(const cv::Mat& mat, const uint64_t& pts_ns) {
    if (paused || should_stop || !smoke_src || !GST_IS_APP_SRC(smoke_src))
        return;

    cv::Mat input = mat.isContinuous() ? mat : mat.clone();
    size_t buf_size = input.total() * input.elemSize();
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, buf_size, nullptr);
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        memcpy(map.data, input.data, buf_size);
        gst_buffer_unmap(buffer, &map);
    } else {
        gst_buffer_unref(buffer);
        std::cerr << "gst_buffer_map failed for smoke\n";
        return;
    }

    uint64_t _pts_ns = (pts_ns > stream_start_pts) ? (pts_ns - stream_start_pts) : 0;
    GST_BUFFER_PTS(buffer) = (GstClockTime)_pts_ns;
    GST_BUFFER_DTS(buffer) = (GstClockTime)_pts_ns;
    GST_BUFFER_DURATION(buffer) = (gint64)(GST_SECOND / framerate);

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(smoke_src), buffer);
    if (ret != GST_FLOW_OK) {
        std::cerr << "Smoke Failed One Epoch\n";
    }
}

void RTSP_BASE::push_audio_frame(const std::vector<uint8_t>& pcm_s16, const uint64_t& pts_ns) {
    curr_pts_audio = pts_ns;

    if (paused || should_stop || !audio_src || !GST_IS_APP_SRC(audio_src))
        return;

    size_t num_bytes = pcm_s16.size();
    if (num_bytes == 0) return;

    GstBuffer* buffer = gst_buffer_new_allocate(NULL, num_bytes, NULL);
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        std::cerr << "Audio: gst_buffer_map failed\n";
        gst_buffer_unref(buffer);
        return;
    }
    memcpy(map.data, pcm_s16.data(), num_bytes);

    gst_buffer_unmap(buffer, &map);

    size_t samples_per_channel = num_bytes / (channels * sample_size);
    uint64_t duration_ns = (uint64_t)((double)samples_per_channel * 1e9 / sample_rate);
    GST_BUFFER_DURATION(buffer) = duration_ns;

    uint64_t _pts_ns = std::max(pts_ns - stream_start_pts - 0.5 * 1e9 , 0.0);
    GST_BUFFER_PTS(buffer) = (GstClockTime)_pts_ns;
    GST_BUFFER_DTS(buffer) = (GstClockTime)_pts_ns;

    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(audio_src), buffer);
    if (ret != GST_FLOW_OK) {
        std::cerr << "Audio Failed One Epoch\n";
    }
}

RTSP_LIVE::RTSP_LIVE(AudioInfo Aset, CamInfo Cset, RtspInfo Rset)
    : RTSP_BASE(&Aset, &Cset, &Rset) {
    if (sample_size == 1) audio_format = "U8";
    else if (sample_size == 2) audio_format = "S16LE";
    else if (sample_size == 4) audio_format = "S32LE";
    else audio_format = "S16LE";
}

RTSP_LIVE::~RTSP_LIVE() {
    cleanup();
}

void RTSP_LIVE::cleanup() {
    if (!is_initialized) return;

    should_stop = true;
    paused = true;

    if (server_id != 0) {
        server_id = 0;
    }

    if (loop && g_main_loop_is_running(loop)) {
        g_main_loop_quit(loop);
    }

    if (worker.joinable()) {
        worker.join();
    }

    auto cleanup_appsrc = [](GstElement*& src) {
        if (src && GST_IS_APP_SRC(src)) {
            gst_app_src_end_of_stream(GST_APP_SRC(src));
            gst_object_unref(src);
            src = nullptr;
        }
    };
    cleanup_appsrc(video_src);
    cleanup_appsrc(audio_src);
    cleanup_appsrc(smoke_src);

    if (factory) {
        g_signal_handlers_disconnect_by_func(factory, (gpointer)media_config, this);
        g_signal_handlers_disconnect_by_func(factory, (gpointer)client_removed_connection, this);
        gst_object_unref(factory);
        factory = nullptr;
    }

    if (server) {
        gst_object_unref(server);
        server = nullptr;
    }

    if (loop) {
        while (g_main_loop_is_running(loop)) {
            g_usleep(10000);
        }
        g_main_loop_unref(loop);
        loop = nullptr;
    }

    if (context) {
        g_main_context_unref(context);
        context = nullptr;
    }

    should_stop = false;
    paused = false;
    stream_start_pts = 0;
    curr_pts_video = 0;
    curr_pts_audio = 0;
    is_initialized = false;
}

bool RTSP_LIVE::Init() {
    if (is_initialized) {
        std::cerr << "Already initialized, cleaning up first..." << std::endl;
        cleanup();
    }

    should_stop = false;
    paused = false;

    context = g_main_context_new();
    loop = g_main_loop_new(context, FALSE);
    if (!loop) {
        std::cerr << "Failed to create main loop" << std::endl;
        return false;
    }

    server = gst_rtsp_server_new();
    if (!server) {
        std::cerr << "Failed to create RTSP server" << std::endl;
        cleanup();
        return false;
    }

    gst_rtsp_server_set_address(server, IP.c_str());
    gst_rtsp_server_set_service(server, std::to_string(port).c_str());
    
    server_id = gst_rtsp_server_attach(server, context);
    if (server_id == 0) {
        std::cerr << "Failed to attach RTSP server" << std::endl;
        cleanup();
        return false;
    }

    GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server);
    if (!mounts) {
        std::cerr << "Failed to get mount points" << std::endl;
        cleanup();
        return false;
    }

    factory = gst_rtsp_media_factory_new();
    if (!factory) {
        std::cerr << "Failed to create media factory" << std::endl;
        g_object_unref(mounts);
        cleanup();
        return false;
    }

    std::ostringstream launch;

    std::string vpipe;
    // switch(zipF){
    //     case RTSP::zip_format::RAW:
    //     vpipe = "queue max-size-time=0 max-size-buffers=100 max-size-bytes=0 leaky=downstream ! ";
    //     vpipe += "rtpvrawpay ";
    //     break;
    //     case RTSP::zip_format::H264:
    //     vpipe = "videoconvert ! video/x-raw,format=I420 ! ";
    //     vpipe += "queue max-size-time=0 max-size-buffers=100 max-size-bytes=0 leaky=downstream ! ";
    //     vpipe += "x264enc tune=zerolatency byte-stream=true key-int-max=15 speed-preset=ultrafast bframes=0 intra-refresh=true ! rtph264pay ";
    //     break;
    //     case RTSP::zip_format::H265:
    //     vpipe = "videoconvert ! video/x-raw,format=I420 ! ";
    //     vpipe += "queue max-size-time=0 max-size-buffers=100 max-size-bytes=0 leaky=downstream ! "
    //     vpipe += "x265enc tune=zerolatency key-int-max=15 speed-preset=ultrafast ! h265parse config-interval=1 ! rtph265pay ";
    //     break;
    // }
    vpipe = "videoconvert ! video/x-raw,format=I420 ! ";
    vpipe += "queue max-size-time=0 max-size-buffers=100 max-size-bytes=0 leaky=downstream ! ";
    vpipe += "x264enc tune=zerolatency byte-stream=true key-int-max=15 speed-preset=ultrafast bframes=0 intra-refresh=true ! rtph264pay ";

    // 视频流 (payload 96)
    launch << "( appsrc name=videosrc is-live=true format=time caps=video/x-raw,format=" << format
           << ",width=" << Isize.first << ",height=" << Isize.second
           << ",framerate=" << framerate << "/1 ! ";
    launch << vpipe;
    launch << "name=pay0 pt=96 ) ";

    // 去雾视频流 (payload 97)
    launch << "( appsrc name=smokesrc is-live=true format=time caps=video/x-raw,format=" << format
           << ",width=" << Isize.first << ",height=" << Isize.second
           << ",framerate=" << framerate << "/1 ! ";
    launch << vpipe;
    launch << "name=pay1 pt=97 ) ";

    // 音频流 (payload 98)
    launch << "( appsrc name=audiosrc is-live=true format=time caps=audio/x-raw,format=" << audio_format
           << ",channels=" << channels << ",rate=" << sample_rate
           << ",layout=interleaved ! ";
    launch << "audioconvert ! audioresample ! ";
    launch << "queue max-size-time=0 max-size-buffers=100 max-size-bytes=0 leaky=downstream ! ";
    launch << "voaacenc ! rtpmp4apay name=pay2 pt=98 )";

    std::string launch_str = launch.str();

    gst_rtsp_media_factory_set_launch(factory, launch_str.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);
    gst_rtsp_media_factory_set_suspend_mode(factory, GST_RTSP_SUSPEND_MODE_NONE);
    g_object_set(factory, "eos-shutdown", TRUE, NULL);

    // 连接信号
    g_signal_handlers_disconnect_by_func(factory, (gpointer)media_config, this);
    g_signal_handlers_disconnect_by_func(factory, (gpointer)client_removed_connection, this);
    g_signal_connect(factory, "media-configure", G_CALLBACK(media_config), this);

    std::string mount_path = "/" + suffix;
    gst_rtsp_mount_points_add_factory(mounts, mount_path.c_str(), factory);
    g_object_unref(mounts);

    std::cerr << "RTSP Server is live at: rtsp://" << IP << ":" << port << mount_path << std::endl;

    // 启动主循环线程
    worker = std::thread([this]() {
        std::cerr << "RTSP main loop started" << std::endl;
        g_main_loop_run(loop);
        std::cerr << "RTSP main loop exited" << std::endl;
    });

    is_initialized = true;
    return true;
}

void RTSP_LIVE::Stop() {
    std::cerr << "Stopping RTSP server..." << std::endl;
    cleanup();
    std::cerr << "RTSP server stopped" << std::endl;
}

GstFlowReturn RTSP_LIVE::media_config(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data) {
    auto* self = static_cast<RTSP_LIVE*>(user_data);
    std::cerr << "Receiver connected" << std::endl;

    if (self->should_stop) {
        std::cerr << "Server is stopping, rejecting connection" << std::endl;
        return GST_FLOW_ERROR;
    }

    GstElement* pipeline = gst_rtsp_media_get_element(media);
    if (!pipeline) {
        std::cerr << "Failed to get pipeline from media" << std::endl;
        return GST_FLOW_ERROR;
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    if (self->video_src) {
        gst_object_unref(self->video_src);
        self->video_src = nullptr;
    }
    self->video_src = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "videosrc");

    if (self->audio_src) {
        gst_object_unref(self->audio_src);
        self->audio_src = nullptr;
    }
    self->audio_src = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "audiosrc");

    if (self->smoke_src) {
        gst_object_unref(self->smoke_src);
        self->smoke_src = nullptr;
    }
    self->smoke_src = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "smokesrc");

    auto config_appsrc = [](GstElement* src, const char* name) {
        if (src && GST_IS_APP_SRC(src)) {
            g_object_set(src, "is-live", TRUE, "block", FALSE, "min-latency", 0, "max-latency", 0, NULL);
        } else {
            g_printerr("%s not ready\n", name);
        }
    };
    config_appsrc(self->video_src, "videosrc");
    config_appsrc(self->audio_src, "audiosrc");
    config_appsrc(self->smoke_src, "smokesrc");

    g_signal_handlers_disconnect_by_func(media, (gpointer)client_removed_connection, self);
    g_signal_connect(media, "unprepared", G_CALLBACK(client_removed_connection), self);

    self->stream_start_pts = std::min(self->curr_pts_audio, self->curr_pts_video);

    self->paused = false;

    gst_object_unref(pipeline);
    return GST_FLOW_OK;
}

GstFlowReturn RTSP_LIVE::client_removed_connection(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data) {
    auto* self = static_cast<RTSP_LIVE*>(user_data);
    std::cerr << "Client disconnected" << std::endl;
    self->paused = true;
    return GST_FLOW_OK;
}

} // namespace RTSP