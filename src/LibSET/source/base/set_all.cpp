#include "SET_ALL.h"

using namespace SET;

SET_ALL::SET_ALL() :
    SET_RTSP(),
    SET_AUDIO(),
    SET_VIS(),
    SET_OUT()
{
}

SET_ALL::~SET_ALL()
{
}

void SET_ALL::check()
{
    SET_RTSP::check();
    SET_AUDIO::check();
    SET_VIS::check();
    SET_OUT::check();
}

void SET_ALL::help()
{
    SET_RTSP::help();
    SET_AUDIO::help();
    SET_VIS::help();
    SET_OUT::help();
}
