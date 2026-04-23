#ifndef CVTOOL_H
#define CVTOOL_H
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <ostream>

namespace CVTool {

bool startWithRTSP(const std::string& url);

class CvVideoIO {
private:
    int PicOn_m = 0;    // 处理片断开始[分]
    int PicOn_s = 0;    // 处理片断开始[秒]
    int PicOn_f = 1;    // 处理片断开始[帧]
    int PicOut_m = 1;   // 处理片段结束[分]
    int PicOut_s = 0;  // 处理片段结束[秒]
    int PicOut_f = 1;   // 处理片段结束[帧]
    int n_start;
    int n_end;
    std::shared_ptr<cv::VideoCapture> _video_in;
    std::shared_ptr<cv::VideoWriter> _video_out;

public:
    CvVideoIO();
    ~CvVideoIO();


    bool initVideoIn(const std::string& url);
    bool initVideoOut(const std::string& out_path);

    double videoInFPS();
    int videoInFrameWith();
    int videoInFrameHeight();
    int videoFrameCount();


    void deinitVideoIn();
    void deinitVideoOut();

    bool videoInReady();
    bool videoOutReady();

    bool readFrameFromeVideo(cv::Mat& frame_in);


    cv::Mat toMergedFrame(int y_method, const cv::Mat&frame_in, const cv::Mat& frame_out);

    void saveFrame(const cv::Mat& frame);
    void showFrame(const cv::Mat& frame);

};

void CudaDeviceinfo(std::ostream& out);


}

#endif // CVTOOL_H
