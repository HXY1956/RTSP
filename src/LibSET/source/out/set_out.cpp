#include <iomanip>
#include <sstream>
#include <algorithm>
#include "SET_OUT.h"
using namespace pugi;

namespace SET
{
    SET_OUT::SET_OUT()
        : SET::SET_BASE() 
    {
        _set.insert(XMLKEY_OUT);
    }

    SET_OUT::~SET_OUT()
    {
    }

    void SET_OUT::check()
    {
        _gmutex.lock();
        xml_node parent = _doc.child(XMLKEY_ROOT);
        xml_node node = _default_node(parent, XMLKEY_OUT);
        _gmutex.unlock();
        return;
    }

    void SET_OUT::help()
    {
        _gmutex.lock();
        _gmutex.unlock();
        return;
    }

} // namespace
