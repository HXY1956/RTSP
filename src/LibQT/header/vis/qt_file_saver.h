#ifndef QT_FILE_SAVER_H
#define QT_FILE_SAVER_H

#include "base_global.h"
#include <QObject>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QLabel>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QQueue>
#include <QDebug>
#include <QWaitCondition>
#include <QDateTime>
#include <QDir>
#include <opencv2/opencv.hpp>
#include <atomic>
#include <thread>
#include <string>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <vector>
#undef signals
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#define signals protected

using namespace BASE;

namespace QT{

    class FILE_SAVER : public QThread {
        Q_OBJECT

    public:
        explicit FILE_SAVER(QObject* parent = nullptr);
        FILE_SAVER(
            AudioInfo Aset,
            CamInfo Cset,
            RtspInfo Rset,
            QObject* parent = nullptr);
        ~FILE_SAVER();

    public:
        void setvideo(CamInfo Cset);
        void setaudio(AudioInfo Aset);
        void setfilepath(const std::string& path);
        void setStreamfirstpts(uint64_t _stream_start_pts){
            stream_start_pts = _stream_start_pts;
        }

    public:
        void startRecord()
        {
            eos_sent = false;
            m_recording = true;
            isFirstFrame = true;
            finalize_done = false;
            if (!isRunning())
            {
                should_stop = false;
                start();
            }
        }
        bool Init();
        void stopRecord();
        void stopThread();

    public:
        void push_video_frame(const cv::Mat& mat, const uint64_t& pts_ns);
        void push_audio_frame(const std::vector<uint8_t>& pcm_s16, const uint64_t& pts_ns);
        static bool ensureDirectoryExists(
            const std::string& filePath);

    protected:
        void run() override;

    private:

        enum class MediaType
        {
            VIDEO,
            AUDIO
        };

        struct BufferTask
        {
            GstBuffer* buffer = nullptr;
            MediaType type;
        };

    private:
        QQueue<BufferTask> m_queue;
        QMutex m_mutex;
        QWaitCondition m_cond;
        std::atomic<bool> should_stop{ false };
        std::atomic<bool> m_recording{ true };
        bool eos_sent;
		bool isFirstFrame = true;
        std::string filepath = "";
        int framerate = -1;
        std::pair<int, int> Isize;
        std::string format = "I420";
        int sample_rate = 48000;
        int sample_size = 2;
        int channels = 2;
        std::string audio_format;
        std::string zipformat;
        GstElement* file_pipeline = nullptr;
        GstElement* video_src = nullptr;
        GstElement* audio_src = nullptr;
        uint64_t stream_start_pts = 0;
        bool finalize_done = false;
    };
}

#endif