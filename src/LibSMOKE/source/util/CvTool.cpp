#include "CvTool.h"
#include <iostream>
#include <opencv2/core/cuda.hpp>

namespace CVTool {

bool startWithRTSP(const std::string& url) {
    const std::string scheme("rtsp://");
    return url.compare(0, scheme.size(), scheme)==0;
}

CvVideoIO::CvVideoIO() {}
CvVideoIO::~CvVideoIO()
{
    deinitVideoIn();
    deinitVideoOut();
}

bool CvVideoIO::initVideoIn(const std::string& url) {
    deinitVideoIn();
    _video_in = std::make_shared<cv::VideoCapture>();
    if( _video_in ) {
        _video_in->open(url);
        if(!_video_in->isOpened()) {
            deinitVideoIn();
            //std::cout << "Error opening video stream or file" << std::endl;
            return false;
        }
    }else {
        return false;
    }
    double fps = videoInFPS();
    n_start = round(PicOn_f
                        + PicOn_s * fps
                        + PicOn_m * fps * 60);

    n_end = round(PicOut_f
                        + PicOut_s * fps
                        + PicOut_m * fps * 60);

    return true;
}

bool CvVideoIO::initVideoOut(const std::string& out_path) {
    double fps = videoInFPS();
    int frame_width = videoInFrameWith();
    int frame_height =videoInFrameHeight();
    cv::Size size(frame_width*2+4, frame_height);
    int codec = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');

    _video_out = std::make_shared<cv::VideoWriter>(out_path , codec, fps, size, true);
    if(!_video_out || !_video_out->isOpened()) {
        //std::cout << "Error opening video stream for output[" <<out_path << "] !" <<std::endl;
        return false;
    } {
        return true;
    }
}


void CvVideoIO::deinitVideoIn() {
    if(_video_in) {
        _video_in->release();
        _video_in = nullptr;
    }
}

void CvVideoIO::deinitVideoOut() {
    if(_video_out) {
        _video_out->release();
        _video_out = nullptr;
    }
}



bool CvVideoIO::videoInReady() {
    return _video_in && _video_in->isOpened();
}

bool CvVideoIO::videoOutReady() {
    return _video_out && _video_out->isOpened();
}

double CvVideoIO::videoInFPS() {
    if(!videoInReady()) {
        return -1;
    }
    return _video_in->get(cv::CAP_PROP_FPS);
}

int CvVideoIO::videoInFrameWith() {
    if(!videoInReady()) {
        return -1;
    }
    return _video_in->get(cv::CAP_PROP_FRAME_WIDTH);
}


int CvVideoIO::videoInFrameHeight() {
    if(!videoInReady()) {
        return -1;
    }
    return _video_in->get(cv::CAP_PROP_FRAME_HEIGHT);
}

int CvVideoIO::videoFrameCount() {
    if(!videoInReady()) {
        return -1;
    }
    return _video_in->get(cv::CAP_PROP_FRAME_COUNT);
}

bool CvVideoIO::readFrameFromeVideo(cv::Mat& frame_in) {
    if(!videoInReady()) {
        return false;
    }
    return _video_in->read(frame_in);
}


cv::Mat CvVideoIO::toMergedFrame(int y_method, const cv::Mat&frame_in, const cv::Mat& frame_out) {
    int rows = frame_in.rows;
    int cols  = frame_in.cols + frame_out.cols +4;
    cv::Vec3b style;
    switch (y_method) {
    case 1:
        //RED
        style=cv::Vec3b(0, 0, 255);
        break;
    case 2:
        //GREEN
        style=cv::Vec3b(0, 255, 0);
        break;
    case 3:
        //BLUE
        style=cv::Vec3b(255, 0, 0);
        break;
    default:
        assert(y_method==1);
        break;
    }


    cv::Mat_<cv::Vec3b> merged(rows, cols);
    frame_in.copyTo(merged.colRange(0, frame_in.cols));

    merged.colRange( frame_in.cols,  frame_in.cols + 4) = style;
    frame_out.copyTo(merged.colRange(frame_in.cols + 4, cols));
    return merged;
}

void CvVideoIO::saveFrame(const cv::Mat& frame) {
    if(!this->videoOutReady()) {
        return;
    }
    _video_out->write(frame);
}


void CvVideoIO::showFrame(const cv::Mat& frame)  {
    cv::imshow("Frame", frame);
    int key = cv::waitKey(1);
    if (key == 'q')
    {
        std::cout << "q key is pressed by the user. Stopping the video" << std::endl;
    }
}



void CudaDeviceinfo(std::ostream& out) {
    int cudaDevviceCount = cv::cuda::getCudaEnabledDeviceCount();
    out << "============================= CUDA Device Info =============================" << std::endl;
    std::cout << "cudaDevviceCount = " << cudaDevviceCount << std::endl;
#define devinfo(method) do{std::cout<<"\t."#method"()=|" << info.method() << "|"<<std::endl;}while(0)
    cv::cuda::DeviceInfo info;
    std::cout << "cv::cuda::DeviceInfo"<<std::endl;
    devinfo(name);
    devinfo(majorVersion);
    devinfo(minorVersion);
    devinfo(multiProcessorCount);
    devinfo(canMapHostMemory);
    devinfo(ECCEnabled);
    devinfo(unifiedAddressing);
    devinfo(memoryClockRate);
    devinfo(memoryBusWidth);
    devinfo(l2CacheSize);
    devinfo(totalMemory);
    devinfo(freeMemory);
    devinfo(totalGlobalMem);
    devinfo(totalConstMem);
    devinfo(regsPerBlock);
    devinfo(sharedMemPerBlock);
    devinfo(warpSize);
    devinfo(maxThreadsPerBlock);
    devinfo(maxThreadsDim);
    devinfo(maxGridSize);
    devinfo(maxThreadsPerMultiProcessor);
    devinfo(textureAlignment);
    devinfo(texturePitchAlignment);
    devinfo(maxTexture1D);
    devinfo(maxTexture1DMipmap);
    devinfo(maxTexture1DLinear);
    devinfo(maxTexture2D);
    devinfo(maxTexture2DMipmap);
    devinfo(maxTexture2DLinear);
    devinfo(maxTexture2DGather);
    devinfo(maxTexture3D);
    devinfo(maxTextureCubemap);
    devinfo(maxTexture1DLayered);
    devinfo(maxTexture2DLayered);
    devinfo(maxTextureCubemapLayered);
    devinfo(maxSurface1D);
    devinfo(maxSurface2D);
    devinfo(maxSurface3D);
    devinfo(maxSurface1DLayered);
    devinfo(maxSurface2DLayered);
    devinfo(maxSurfaceCubemap);
    devinfo(maxSurfaceCubemapLayered);
    devinfo(surfaceAlignment);

    devinfo(concurrentKernels);
    devinfo(kernelExecTimeoutEnabled);
    devinfo(asyncEngineCount);


    devinfo(integrated);
    devinfo(clockRate);
    devinfo(isCompatible);
    devinfo(deviceID);
    devinfo(pciBusID);
    devinfo(pciDeviceID);
    devinfo(pciDomainID);
    devinfo(tccDriver);

#undef devinfo

}

}


