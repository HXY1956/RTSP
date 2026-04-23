#ifndef RTSP_LIVE_H
#define RTSP_LIVE_H

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <thread>
#include <string>
#include <sstream>
#include <iostream>
#include <cstring>
#include <vector>
#include "base_global.h"
#include "base_mutex.h"

using namespace BASE;

namespace RTSP {
    enum class zip_format{
        H264,
        H265,
        RAW
    };

class RTSP_BASE {
public:
    RTSP_BASE(AudioInfo* Aset, CamInfo* Cset, RtspInfo* Rset);
    virtual ~RTSP_BASE() = default;

    void push_video_frame(const cv::Mat& mat, const uint64_t& pts_ns);
    void push_smoke_frame(const cv::Mat& mat, const uint64_t& pts_ns);
    void push_audio_frame(const std::vector<uint8_t>& pcm_s16, const uint64_t& pts_ns);
    virtual bool Init() = 0;
    virtual void Stop() = 0;

protected:
    int framerate;
    std::pair<int, int> Isize;
    std::string format;
    int sample_rate;
    int sample_size;
    int channels;
    std::string IP;
    int port;
    std::string suffix;
    std::string audio_format;
    zip_format zipF;

    GstElement* video_src = nullptr;
    GstElement* audio_src = nullptr;
    GstElement* smoke_src = nullptr;

    uint64_t stream_start_pts = 0;
    uint64_t curr_pts_video = 0;
    uint64_t curr_pts_audio = 0;

    std::atomic<bool> paused{ false };
    std::atomic<bool> should_stop{ false };
};

class RTSP_LIVE : public RTSP_BASE {
public:
    RTSP_LIVE(AudioInfo Aset, CamInfo Cset, RtspInfo Rset);
    ~RTSP_LIVE() override;

    bool Init() override;
    void Stop() override;

private:
    GMainLoop* loop = nullptr;
    GstRTSPServer* server = nullptr;
    GstRTSPMediaFactory* factory = nullptr;
    GMainContext* context = nullptr;  // 使用 GMainContext 而不是 GCancellable
    std::thread worker;
    guint server_id = 0;               // 保存 attach 返回的 ID

    bool is_initialized = false;

    void cleanup();

    static GstFlowReturn media_config(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data);
    static GstFlowReturn client_removed_connection(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data);
};

} // namespace RTSP

#endif // RTSP_LIVE_H