#ifndef RTSP_FILE_H
#define RTSP_FILE_H
#include "rtsp_live.h"

namespace RTSP {
    enum class zip_format{
        H264,
        H265,
        RAW
    };

    class RTSP_FILE: public RTSP_BASE{
    public:
        RTSP_FILE(AudioInfo Aset, CamInfo Cset, RtspInfo Rset) : RTSP_BASE(&Aset, &Cset, &Rset){
            filepath = Rset.filepath;
            if(Rset.vformat == "H264") zipF = zip_format::H264;
            if(Rset.vformat == "H265") zipF = zip_format::H265;
            if(Rset.vformat == "RAW") zipF = zip_format::RAW;
        } 
        bool Init() override;
        void Stop() override;
    private:
        std::string filepath;
        GstElement* file_pipeline = nullptr;
        std::thread file_worker;
        GMainLoop* file_loop = nullptr;
        zip_format zipF;

    };
}


#endif