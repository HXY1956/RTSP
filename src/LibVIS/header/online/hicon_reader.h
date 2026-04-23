#ifndef HICON_READER_H
#define HICON_READER_H

#include <stdio.h>
#include <string>
#include <chrono>
#include <thread>
#include <cstdint> 
#include <ctime>        
#include <iomanip>      
#include <sstream>     
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <fstream>
#include "MvCameraControl.h"
#include "vis_decoder.h"
#include "base_global.h"

namespace VIS {
    class VIS_HICON_READER : public VIS_CODER {
    public:
        VIS_HICON_READER(BASE::CamInfo info): VIS_CODER(&info), devicename(info.devicepath){};
        ~VIS_HICON_READER() { CloseCam(); };

        bool Init() override;
        bool Capture(uint64_t& ImgT, cv::Mat& frame) override;
        void toCvMat(cv::Mat & frame);
        static std::map<std::string, std::string> FindHiconCamera();
        void Stop() override;
        void CloseCam();
        void ShowErrorMsg(int nErrorNum);

    private:
        MV_FRAME_OUT stOutFrame = { 0 };
        void* handle;
        unsigned char* pData;
        MV_FRAME_OUT_INFO_EX* pInfo;
        std::string devicename;
        MV_CC_DEVICE_INFO* pDeviceInfo;
    };
}

#endif