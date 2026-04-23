#ifndef RTSP_FILE_H
#define RTSP_FILE_H
#include "rtsp_live.h"

namespace RTSP {
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