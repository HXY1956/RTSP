#ifndef HWA_SET_ALL_H
#define HWA_SET_ALL_H

#include <signal.h>
#include "SET_VIS.h"
#include "SET_AUDIO.h"
#include "SET_OUT.h"
#include "SET_RTSP.h"

namespace SET
{
    class SET_ALL :
        public virtual SET_RTSP,
        public virtual SET_AUDIO,
        public virtual SET_VIS,
        public virtual SET_OUT
    {
    public:
        SET_ALL();
        virtual ~SET_ALL();

        void check();
        void help();

    private:

    };
}
#endif