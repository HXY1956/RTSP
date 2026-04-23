#include <iomanip>
#include <sstream>
#include <algorithm>
#include "SET_RTSP.h"
using namespace pugi;

namespace SET
{
    SET_RTSP::SET_RTSP()
        : SET::SET_BASE() 
    {
        _set.insert(XMLKEY_RTSP);
    }

    SET_RTSP::~SET_RTSP()
    {
    }

    void SET_RTSP::check()
    {
        _gmutex.lock();
        xml_node parent = _doc.child(XMLKEY_ROOT);
        xml_node node = _default_node(parent, XMLKEY_RTSP);
        _gmutex.unlock();
        return;
    }

    void SET_RTSP::help()
    {
        _gmutex.lock();
        _gmutex.unlock();
        return;
    }

    std::string SET_RTSP::filepath()
    {
        _gmutex.lock();
        std::string str = _doc.child(XMLKEY_ROOT).child(XMLKEY_RTSP).attribute("filepath").value();
        _gmutex.unlock();
        return str;
    }

} // namespace
