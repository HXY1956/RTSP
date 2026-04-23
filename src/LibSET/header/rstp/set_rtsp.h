#ifndef SET_WAV_H
#define SET_WAV_H

#define XMLKEY_RTSP "rstp"

#include <string>
#include <iostream>
#include "set_base.h"

using namespace pugi;

namespace SET
{
    class SET_RTSP : public virtual SET_BASE
    {
    public:
        SET_RTSP();
        ~SET_RTSP();

        void check();
        void help();

        std::string filepath();

    private:
    };

} // namespace
#endif
