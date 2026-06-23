#ifndef QT_GLOBAL_H
#define QT_GLOBAL_H
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>              
#include <atomic>             
#include <chrono>     
#include <fstream>
#include <iostream>
#include <sstream>

namespace QT {
	using ImgMap = std::map<int64, cv::Mat>;
	using AudioMap = std::map<int64_t, std::vector<uint8_t>>;

	/**
	*@简介  处理模式，分为离线模式和在线模式。离线模式适用于处理预先录制的视频文件，而在线模式适用于实时处理来自摄像头或RTSP流的视频数据。根据不同的处理需求，选择合适的处理模式可以优化性能和资源使用。
	*/
	enum class ProcMode {
		OFFLINE = 0,
	    ONLINE = 1
	};
}

#endif