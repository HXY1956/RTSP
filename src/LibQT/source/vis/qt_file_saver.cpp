#include "qt_file_saver.h"
#include "rtsp_live.h"
#include <filesystem>
#include <sstream>
#include <iostream>
namespace fs = std::filesystem;

namespace QT{
    FILE_SAVER::FILE_SAVER(QObject* parent)
        : QThread(parent)
    {
    }

    FILE_SAVER::FILE_SAVER(
        AudioInfo Aset,
        CamInfo Cset,
        RtspInfo Rset,
        QObject* parent)
        : QThread(parent)
    {
        setaudio(Aset);
        setvideo(Cset);
        zipformat = Rset.vformat;
    }

    FILE_SAVER::~FILE_SAVER()
    {
        stopThread();
    }

    void FILE_SAVER::setvideo(CamInfo Cset)
    {
        framerate = Cset.framerate;
        Isize.first = Cset.width;
        Isize.second = Cset.height;
        format = Cset.format;
    }

    void FILE_SAVER::setaudio(AudioInfo Aset)
    {
        sample_rate = Aset.sample_rate;
        sample_size = Aset.sample_size;
        channels = Aset.channels;
    }

    void FILE_SAVER::setfilepath(
        const std::string& path)
    {
        filepath = path;
    }

    bool FILE_SAVER::ensureDirectoryExists(
        const std::string& filePath)
    {
        fs::path dirPath =
            fs::path(filePath).parent_path();

        if (!fs::exists(dirPath))
        {
            try
            {
                return
                    fs::create_directories(
                        dirPath);
            }
            catch (...)
            {
                return false;
            }
        }

        return true;
    }

    bool FILE_SAVER::Init()
    {
        ensureDirectoryExists(filepath);

        if (sample_size == 1)
            audio_format = "U8";

        else if (sample_size == 2)
            audio_format = "S16LE";

        else if (sample_size == 4)
            audio_format = "S32LE";

        std::ostringstream launch;
        RTSP::zip_format zipF;

        if(zipformat == "H264") zipF = RTSP::zip_format::H264;
        else if(zipformat == "H265") zipF = RTSP::zip_format::H265;

        switch(zipF){
        case RTSP::zip_format::H264:{
            launch << "( appsrc name=videosrc is-live=true format=time caps=video/x-raw,format=" << format
            << ",width=" << std::to_string(Isize.first) << ",height=" << std::to_string(Isize.second) <<",framerate="<<framerate<<"/1"
            << " ! videoconvert ! video/x-raw,format=NV12 ! nvvidconv ! video/x-raw(memory:NVMM),format=NV12 ! nvv4l2h264enc bitrate=5000000 key-int-max=30 ! "
               "h264parse ! video/x-h264,stream-format=avc,alignment=au ! queue ! mux. ) " ;
            break;
        }
        case RTSP::zip_format::H265:{
            launch << "( appsrc name=videosrc is-live=true format=time caps=video/x-raw,format=" << format
            << ",width=" << std::to_string(Isize.first) << ",height=" << std::to_string(Isize.second) <<",framerate="<<framerate<<"/1"
            << " ! videoconvert ! video/x-raw,format=NV12 ! nvvidconv ! video/x-raw(memory:NVMM),format=NV12 ! nvv4l2h265enc bitrate=5000000 key-int-max=30 ! "
               "h265parse ! video/x-h265,stream-format=hvc1,alignment=au ! queue ! mux. ) " ;
            break;
        }
        }
        launch  << "( appsrc name=audiosrc is-live=true format=time caps=audio/x-raw,format="<< audio_format
            << ",channels=" << std::to_string(channels) << ",rate=" << std::to_string(sample_rate) << ",layout=interleaved ! "
            << "audioconvert ! audioresample ! queue max-size-buffers=20 ! voaacenc ! aacparse ! queue max-size-buffers=20 ! mux. ) "
            << "matroskamux name=mux streamable=true ! filesink location=\"" << filepath << "\"";

        std::string launch_str =
            launch.str();

        file_pipeline =
            gst_parse_launch(
                launch_str.c_str(),
                nullptr);

        if (!file_pipeline)
        {
            std::cerr
                << "Failed to create pipeline\n";

            return false;
        }

        if (video_src) {
            gst_object_unref(video_src);
            video_src = nullptr;
        }

        if (audio_src) {
            gst_object_unref(audio_src);
            audio_src = nullptr;
        }

        video_src =
            gst_bin_get_by_name(
                GST_BIN(file_pipeline),
                "videosrc");

        audio_src =
            gst_bin_get_by_name(
                GST_BIN(file_pipeline),
                "audiosrc");

        if (!video_src)
        {
            std::cerr
                << "videosrc NOT found\n";
        }

        if (!audio_src)
        {
            std::cerr
                << "audiosrc NOT found\n";
        }

        if (video_src) {
            g_object_set(G_OBJECT(video_src),
                "is-live", FALSE,
                "format", GST_FORMAT_TIME,
                "do-timestamp", FALSE,
                NULL);
        }
        if (audio_src) {
            g_object_set(G_OBJECT(audio_src),
                "is-live", FALSE,
                "format", GST_FORMAT_TIME,
                "do-timestamp", FALSE,
                NULL);
        }

        GstStateChangeReturn ret =
            gst_element_set_state(
                file_pipeline,
                GST_STATE_PLAYING);

        if (ret ==
            GST_STATE_CHANGE_FAILURE)
        {
            std::cerr
                << "Failed to set PLAYING\n";

            gst_object_unref(
                file_pipeline);

            file_pipeline = nullptr;

            return false;
        }

        should_stop = false;
        m_recording = true;

        return true;
    }

    void FILE_SAVER::stopRecord()
    {
        QMutexLocker locker(&m_mutex);
        m_recording = false;
        m_cond.wakeAll();
    }

    void FILE_SAVER::stopThread()
    {
        {
        QMutexLocker locker(&m_mutex);
        m_recording = false;
        m_cond.wakeAll();
        while (!finalize_done && file_pipeline)
        {
            m_cond.wait(&m_mutex);
        }
        should_stop = true;
        m_cond.wakeAll();
        }
        wait();
    }

    void FILE_SAVER::push_video_frame(const cv::Mat& mat, const uint64_t& pts_ns) {
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

        QMutexLocker locker(&m_mutex);
        BufferTask task;
        task.buffer = buffer;
        task.type = MediaType::VIDEO;
        m_queue.enqueue(task);
        m_cond.wakeOne();
    }

    void FILE_SAVER::push_audio_frame(const std::vector<uint8_t>& pcm_s16, const uint64_t& pts_ns) { 
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

        QMutexLocker locker(&m_mutex);
        BufferTask task;
        task.buffer = buffer;
        task.type = MediaType::AUDIO;
        m_queue.enqueue(task);
        m_cond.wakeOne();
    }

    void FILE_SAVER::run()
    {
        while (!should_stop)
        {
            QMutexLocker locker(&m_mutex);

            while (m_queue.isEmpty() && !should_stop)
            {
                if (!m_recording &&
                    !eos_sent &&
                    file_pipeline)
                {
                    locker.unlock();

                    eos_sent = true;

                    if (video_src)
                    {
                        gst_app_src_end_of_stream(
                            GST_APP_SRC(video_src));
                    }

                    if (audio_src)
                    {
                        gst_app_src_end_of_stream(
                            GST_APP_SRC(audio_src));
                    }

                    GstBus* bus =
                        gst_element_get_bus(
                            file_pipeline);

                    bool done = false;

                    while (!done)
                    {
                        GstMessage* msg =
                            gst_bus_timed_pop_filtered(
                                bus,
                                100 * GST_MSECOND,
                                (GstMessageType)(
                                    GST_MESSAGE_EOS |
                                    GST_MESSAGE_ERROR));

                        if (!msg)
                        {
                            continue;
                        }

                        switch (GST_MESSAGE_TYPE(msg))
                        {
                        case GST_MESSAGE_EOS:

                            done = true;

                            std::cout<<"EOS Received;\n";

                            break;

                        case GST_MESSAGE_ERROR:
                        {
                            GError* err;

                            gchar* debug;

                            gst_message_parse_error(
                                msg,
                                &err,
                                &debug);

                            std::cout
                                << "gst error, file write failed\n";

                            g_error_free(err);

                            g_free(debug);

                            done = true;

                            break;
                        }

                        default:
                            break;
                        }

                        gst_message_unref(msg);
                    }

                    gst_object_unref(bus);

                    gst_element_set_state(
                        file_pipeline,
                        GST_STATE_NULL);

                    gst_object_unref(file_pipeline);

                    file_pipeline = nullptr;

                    std::cout<< "mp4 finalized at: " << filepath<<"\n";

                    locker.relock();

                    finalize_done = true;

                    m_cond.wakeAll();
                }
                m_cond.wait(&m_mutex);
            }

            if (m_queue.isEmpty())
            {
                continue;
            }

            BufferTask task =
                m_queue.dequeue();

            locker.unlock();
 
            if (isFirstFrame && framerate != -1 && filepath != "") {
                Init();
                isFirstFrame = false;
            }

            GstBuffer* buffer =
                task.buffer;

            if (task.type ==
                MediaType::VIDEO)
            {
                GstFlowReturn ret =
                    gst_app_src_push_buffer(
                        GST_APP_SRC(video_src),
                        buffer);

                if (ret != GST_FLOW_OK)
                {
                    qDebug()
                        << "video push failed";
                    gst_buffer_unref(buffer);
                }
            }
            else
            {
                GstFlowReturn ret =
                    gst_app_src_push_buffer(
                        GST_APP_SRC(audio_src),
                        buffer);

                if (ret != GST_FLOW_OK)
                {
                    qDebug()
                        << "audio push failed";
                    gst_buffer_unref(buffer);
                }
            }
        }

        if (file_pipeline)
        {
            gst_element_set_state(
                file_pipeline,
                GST_STATE_NULL);

            gst_object_unref(file_pipeline);

            file_pipeline = nullptr;
            video_src = nullptr;
            audio_src = nullptr;
        }
    }
}