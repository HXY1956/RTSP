#include "set_vis.h"

SET::SET_VIS::SET_VIS() : SET::SET_BASE()
{
    _set.insert(XMLKEY_VIS);
    checkdefault();
}

SET::SET_VIS::~SET_VIS()
{
}

void SET::SET_VIS::checkdefault()
{
    _gmutex.lock();
    xml_node parent = _doc.child(XMLKEY_ROOT);
    xml_node node = _default_node(parent, XMLKEY_VIS);
    _default_attr(node, "timecost", false);
    _gmutex.unlock();
}

const int SET::SET_VIS::width()
{
    _gmutex.lock();
    int str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("width").as_int();
    _gmutex.unlock();
    return str;
}

const int SET::SET_VIS::height()
{
    _gmutex.lock();
    int str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("height").as_int();
    _gmutex.unlock();
    return str;
}

const int SET::SET_VIS::rate()
{
    _gmutex.lock();
    int str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("framerate").as_int();
    _gmutex.unlock();
    return str;
}

const int SET::SET_VIS::deviceID()
{
    _gmutex.lock();
    int str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("deviceID").as_int();
    _gmutex.unlock();
    return str;
}

const bool SET::SET_VIS::knowpath()
{
    _gmutex.lock();
    bool str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("knowpath").as_bool();
    _gmutex.unlock();
    return str;
}

const bool SET::SET_VIS::isvideo()
{
    _gmutex.lock();
    bool str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("isvideo").as_bool();
    _gmutex.unlock();
    return str;
}

const std::string SET::SET_VIS::videopath()
{
    _gmutex.lock();
    std::string str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("videopath").value();
    _gmutex.unlock();
    return str;
}

const std::string SET::SET_VIS::format()
{
    _gmutex.lock();
    std::string str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("format").value();
    _gmutex.unlock();
    return str;
}

const std::string SET::SET_VIS::devicepath()
{
    _gmutex.lock();
    std::string str = _doc.child(XMLKEY_ROOT).child(XMLKEY_VIS).attribute("devicepath").value();
    _gmutex.unlock();
    return str;
}
