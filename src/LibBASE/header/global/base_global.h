#ifndef BASE_GLOB_H
#define BASE_GLOB_H

#include <iostream>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <tuple>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <algorithm>

namespace BASE {
    extern int64_t ProjectStartTime;

    struct AudioInfo {
        int channels = 2;
        int sample_rate = 48000;
        int sample_size = 2;
        std::string filepath = "";
        std::string devicepath = "";
        int noisemode = 0;
    };

    enum CamType {
        HICON_CAM,
        USB_CAM
    };

    struct CamInfo {
        CamType camtype;
        std::string devicepath = "";
        std::string format = "RGB";
        std::string zip = "RAW";
        std::string filepath = "";
        int width = 1280;
        int height = 720;
        int framerate = 30;
    };

    struct RtspInfo {
        int port;
        std::string IP;
        std::string suffix;
        std::string filepath;
        int way = 1;
        std::string vformat;
    };

    inline void callcerr(const std::string& msg){
        std::cerr<<msg<<"\n";
    }
}

#endif
