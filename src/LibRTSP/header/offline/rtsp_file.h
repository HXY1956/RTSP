#ifndef RTSP_FILE_H
#define RTSP_FILE_H
#include "rtsp_live.h"

namespace RTSP {
    /**
	*@简介  RTSP_FILE类继承自RTSP_BASE，提供对离线视频文件的RTSP推流功能。它负责初始化GStreamer管道，读取视频文件并将其推送到RTSP服务器。
    */
    class RTSP_FILE: public RTSP_BASE{
    public:
        RTSP_FILE(AudioInfo Aset, CamInfo Cset, RtspInfo Rset) : RTSP_BASE(&Aset, &Cset, &Rset){
            filepath = Rset.filepath;
        } 
        bool Init() override;
        void Stop() override;
    private:
        std::string filepath;
        GstElement* file_pipeline = nullptr;
        std::thread file_worker;
        GMainLoop* file_loop = nullptr;
    };
}


#endif