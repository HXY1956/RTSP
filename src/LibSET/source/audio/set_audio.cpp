#include <iomanip>
#include <sstream>
#include <algorithm>
#include "set_audio.h"
using namespace pugi;

namespace SET
{
    SET_AUDIO::SET_AUDIO()
        : SET::SET_BASE() 
    {
        _set.insert(XMLKEY_AUDIO);
    }

    SET_AUDIO::~SET_AUDIO()
    {
    }

    void SET_AUDIO::check()
    {
        _gmutex.lock();
        xml_node parent = _doc.child(XMLKEY_ROOT);
        xml_node node = _default_node(parent, XMLKEY_AUDIO);
        _gmutex.unlock();
        return;
    }

    void SET_AUDIO::help()
    {
        _gmutex.lock();
        _gmutex.unlock();
        return;
    }

    const std::string SET_AUDIO::filepath()
    {
        _gmutex.lock();
        std::string str = _doc.child(XMLKEY_ROOT).child(XMLKEY_AUDIO).attribute("filepath").value();
        _gmutex.unlock();
        return str;
    }

    const std::string SET_AUDIO::device_name()
    {
        _gmutex.lock();
        std::string str = _doc.child(XMLKEY_ROOT).child(XMLKEY_AUDIO).attribute("device_name").value();
        _gmutex.unlock();
        return str;
    }

    const std::string SET_AUDIO::device_path()
    {
        _gmutex.lock();
        std::string str = _doc.child(XMLKEY_ROOT).child(XMLKEY_AUDIO).attribute("device_path").value();
        _gmutex.unlock();
        return str;
    }

    const bool SET_AUDIO::know_name()
    {
        _gmutex.lock();
        bool str = _doc.child(XMLKEY_ROOT).child(XMLKEY_AUDIO).attribute("know_name").as_bool();
        _gmutex.unlock();
        return str;
    }

    const int SET_AUDIO::channels()
    {
        _gmutex.lock();
        int str = _doc.child(XMLKEY_ROOT).child(XMLKEY_AUDIO).attribute("channels").as_int();
        _gmutex.unlock();
        return str;
    }

    const int SET_AUDIO::sample_rate()
    {
        _gmutex.lock();
        int str = _doc.child(XMLKEY_ROOT).child(XMLKEY_AUDIO).attribute("sample_rate").as_int();
        _gmutex.unlock();
        return str;
    }

    const int SET_AUDIO::sample_size()
    {
        _gmutex.lock();
        int str = _doc.child(XMLKEY_ROOT).child(XMLKEY_AUDIO).attribute("sample_size").as_int();
        _gmutex.unlock();
        return str;
    }

} // namespace
