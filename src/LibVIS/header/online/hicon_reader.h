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
    /**
	*@简介  海康工业相机读取器类，继承自VIS_CODER，提供对海康工业相机的初始化、捕获和关闭功能
    */
    class VIS_HICON_READER : public VIS_CODER {
    public:
        VIS_HICON_READER(BASE::CamInfo info): VIS_CODER(&info), devicename(info.devicepath){};
        ~VIS_HICON_READER() { CloseCam(); };

        /**
		*@简介  初始化函数，负责初始化海康工业相机，设置相机参数并准备捕获图像
        */
        bool Init() override;

        /**
        *@简介  从海康工业相机中捕获一帧图像，并返回时间戳和图像数据
        */
        bool Capture(uint64_t& ImgT, cv::Mat& frame) override;

        /**
		*@简介  将捕获的图像数据转换为OpenCV的cv::Mat格式，以便后续处理
        */
        void toCvMat(cv::Mat & frame);

        /**
		*@简介  读取海康工业相机的设备信息，并返回一个包含设备名称和路径的映射
        */
        static std::map<std::string, std::string> FindHiconCamera();

        /**
		*@简介  停止函数，负责停止海康工业相机的图像捕获，并释放相关资源
        */
        void Stop() override;

        /**
		*@简介  关闭海康工业相机，释放相机资源，确保在程序退出前正确关闭相机
        */
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