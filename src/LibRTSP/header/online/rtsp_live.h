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
#include <cstdint>
#include <vector>
#include "base_global.h"
#include "base_mutex.h"

using namespace BASE;

namespace RTSP {
    enum class zip_format{
        H264,
        H265
    };


/**
*@简介  RTSP_BASE类是一个抽象基类，提供了RTSP推流的基本功能，包括视频、音频和烟雾帧的推送，以及初始化和停止的接口。
*       它包含了音频、摄像头和RTSP相关的配置信息，并管理GStreamer元素和时间戳。
*/
class RTSP_BASE {
public:
    RTSP_BASE(AudioInfo* Aset, CamInfo* Cset, RtspInfo* Rset);
    virtual ~RTSP_BASE() = default;

    /**
	*@简介  将视频帧推送到RTSP流中，使用OpenCV的cv::Mat格式表示图像，并指定时间戳（以纳秒为单位）。
    */
    void push_video_frame(const cv::Mat& mat, const uint64_t& pts_ns);

    /**
	*@简介  将去雾后的图像帧推送到RTSP流中，使用OpenCV的cv::Mat格式表示图像，并指定时间戳（以纳秒为单位）。
    */
    void push_smoke_frame(const cv::Mat& mat, const uint64_t& pts_ns);

    /**
	*@简介  将音频推送到RTSP流中，使用PCM S16格式的字节向量表示音频数据，并指定时间戳（以纳秒为单位）。
    */
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
    uint64_t curr_pts_video = UINT64_MAX;
    uint64_t curr_pts_audio = UINT64_MAX;

    std::atomic<bool> paused{ false };
    std::atomic<bool> should_stop{ false };

    bool isVideo = false;
    bool isAudio = false;
};

/**
*@简介  RTSP_LIVE类继承自RTSP_BASE，提供了实时RTSP推流的功能。它负责初始化GStreamer RTSP服务器、创建媒体工厂，并处理客户端连接和媒体配置。
*       该类还管理一个独立的工作线程，用于运行GMainLoop以处理RTSP请求。
*/
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
    GMainContext* context = nullptr; 
    std::thread worker;
    guint server_id = 0;             

    bool is_initialized = false;

    void cleanup();

    /**
	*@简介  当客户端连接到RTSP服务器时调用的回调函数，用于处理媒体配置和客户端连接事件。
    */
    static GstFlowReturn media_config(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data);

    /**
	*@简介  当客户端断开连接时调用的回调函数，用于处理客户端断开连接事件。
    */
    static void client_removed_connection(GstRTSPMedia *media, gpointer user_data);
};

}

#endif