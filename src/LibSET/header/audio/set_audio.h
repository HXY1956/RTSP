#ifndef SET_AUDIO_H
#define SET_AUDIO_H

#define XMLKEY_AUDIO "audio"

#include <string>
#include <iostream>
#include "set_base.h"

using namespace pugi;

namespace SET
{
    class SET_AUDIO : public virtual SET_BASE
    {
    public:
        SET_AUDIO();
        ~SET_AUDIO();

        void check();
        void help();

        const std::string filepath();
        const std::string device_name();
        const std::string device_path();
        const bool know_name();
        const int channels();
        const int sample_size();
        const int sample_rate();

    private:
    };

} // namespace

#endif
