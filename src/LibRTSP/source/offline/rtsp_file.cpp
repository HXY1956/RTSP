#include "rtsp_file.h"
#include <filesystem>

namespace fs = std::filesystem;

bool ensureDirectoryExists(const std::string& filePath) {
    fs::path dirPath = fs::path(filePath).parent_path();
    if (!fs::exists(dirPath)) {
        std::cerr << "Directory does not exist: " << dirPath << std::endl;
        try {
            if (fs::create_directories(dirPath)) {
                std::cerr << "Directory created: " << dirPath << std::endl;
            } else {
                std::cerr << "Failed to create directory: " << dirPath << std::endl;
                return false;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }
    return true;
}

bool RTSP::RTSP_FILE::Init() {
    ensureDirectoryExists(filepath);

    if (sample_size == 1) audio_format = "U8";
    if (sample_size == 2) audio_format = "S16LE";
    if (sample_size == 4) audio_format = "S32LE";

    std::ostringstream launch;

    switch(zipF){
        // case RTSP::zip_format::H264:{
        //     launch << "( appsrc name=smokesrc is-live=true format=time caps=video/x-raw,format=" << format
        //     << ",width=" << std::to_string(Isize.first) << ",height=" << std::to_string(Isize.second) <<",framerate="<<framerate<<"/1"
        //     << " ! videoconvert ! video/x-raw,format=NV12 ! nvvidconv ! video/x-raw(memory:NVMM),format=NV12 ! nvv4l2h264enc bitrate=5000000 key-int-max=30 insert-sps-pps=true ! "
        //        "h264parse config-interval=-1 ! video/x-h264,stream-format=avc,alignment=au ! queue ! mux. ) " ;
        //     break;
        // }
        case RTSP::zip_format::H264:{
            launch << "( appsrc name=smokesrc is-live=true format=time caps=video/x-raw,format=" << format
            << ",width=" << std::to_string(Isize.first) << ",height=" << std::to_string(Isize.second) <<",framerate="<<framerate<<"/1"
            << " ! videoconvert ! video/x-raw,format=NV12 ! nvvidconv ! video/x-raw(memory:NVMM),format=NV12 ! nvv4l2h265enc bitrate=5000000 key-int-max=30 ! "
               "h265parse ! video/x-h265,stream-format=hvc1,alignment=au ! queue ! mux. ) " ;
            break;
        }
        case RTSP::zip_format::H265:{
            launch << "( appsrc name=smokesrc is-live=true format=time caps=video/x-raw,format=" << format
            << ",width=" << std::to_string(Isize.first) << ",height=" << std::to_string(Isize.second) <<",framerate="<<framerate<<"/1"
            << " ! videoconvert ! video/x-raw,format=NV12 ! nvvidconv ! video/x-raw(memory:NVMM),format=NV12 ! nvv4l2h265enc bitrate=5000000 key-int-max=30 ! "
               "h265parse ! video/x-h265,stream-format=hvc1,alignment=au ! queue ! mux. ) " ;
            break;
        }
        case RTSP::zip_format::RAW:{
            launch << "( appsrc name=smokesrc is-live=true format=time caps=video/x-raw,format=" << format
            << ",width=" << std::to_string(Isize.first) << ",height=" << std::to_string(Isize.second) <<",framerate="<<framerate<<"/1"
            << " ! videoconvert ! video/x-raw,format=I420 ! "
            << "queue ! mux. ) " ;
            break;
        }
    }
    launch  << "( appsrc name=audiosrc is-live=true format=time caps=audio/x-raw,format="<< audio_format
            << ",channels=" << std::to_string(channels) << ",rate=" << std::to_string(sample_rate) << ",layout=interleaved ! "
            << "audioconvert ! audioresample ! queue max-size-buffers=20 ! voaacenc ! aacparse ! queue max-size-buffers=20 ! mux. ) "
            << "matroskamux name=mux streamable=true ! filesink location=\"" << filepath << "\"";

    std::string launch_str = launch.str();

    file_pipeline = gst_parse_launch(launch_str.c_str(), nullptr);
    if (!file_pipeline) {
        std::cerr << "Failed to create file pipeline\n";
        return false;
    }

    smoke_src = gst_bin_get_by_name(GST_BIN(file_pipeline), "smokesrc");
    audio_src = gst_bin_get_by_name(GST_BIN(file_pipeline), "audiosrc");

    if (!smoke_src) std::cerr << "smokesrc NOT found\n";
    if (!audio_src) std::cerr << "audiosrc NOT found\n";

    if (smoke_src) {
        g_object_set(G_OBJECT(smoke_src),
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

    GstStateChangeReturn ret = gst_element_set_state(file_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set file pipeline to PLAYING\n";
        gst_object_unref(file_pipeline);
        file_pipeline = nullptr;
        return false;
    }

    file_loop = g_main_loop_new(NULL, FALSE);
    file_worker = std::thread([this]() {
        g_main_loop_run(file_loop);
        });

    return true;
}

void RTSP::RTSP_FILE::Stop() {

    if (!file_pipeline) return;

    if (smoke_src)
        gst_app_src_end_of_stream(GST_APP_SRC(smoke_src));

    if (audio_src)
        gst_app_src_end_of_stream(GST_APP_SRC(audio_src));

    GstBus* bus = gst_element_get_bus(file_pipeline);

    bool done = false;
    while (!done) {
        GstMessage* msg = gst_bus_timed_pop_filtered(
            bus,
            GST_CLOCK_TIME_NONE,
            (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR)
        );

        if (msg != nullptr) {
            switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_EOS:
                std::cout << "EOS received\n";
                done = true;
                break;

            case GST_MESSAGE_ERROR:
                GError* err;
                gchar* debug;
                gst_message_parse_error(msg, &err, &debug);
                std::cerr << "Error: " << err->message << std::endl;
                g_error_free(err);
                g_free(debug);
                done = true;
                break;

            default:
                break;
            }
            gst_message_unref(msg);
        }
    }

    gst_object_unref(bus);
    gst_element_set_state(file_pipeline, GST_STATE_NULL);

    if (file_loop) {
        g_main_loop_quit(file_loop);
        if (file_worker.joinable()) file_worker.join();
        g_main_loop_unref(file_loop);
        file_loop = nullptr;
    }

    gst_object_unref(file_pipeline);
    file_pipeline = nullptr;
}