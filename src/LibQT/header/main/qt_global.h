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

	enum class ProcMode {
		OFFLINE = 0,
	    ONLINE = 1
	};
}

#endif