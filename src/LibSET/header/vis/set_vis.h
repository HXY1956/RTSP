#ifndef SET_VIS_H
#define SET_VIS_H

#define XMLKEY_VIS "vis"
#include "set_base.h"

namespace SET
{
    class SET_VIS : public virtual SET_BASE
    {
    public:
        SET_VIS();
        ~SET_VIS();

        void check() {};
        void checkdefault();
        void help() {};

        const int width();
        const int height();
        const int rate();
        const int deviceID();
        const bool knowpath();
        const bool isvideo();
        const std::string videopath();
        const std::string format();
        const std::string devicepath();


    protected:
    };
};

#endif
