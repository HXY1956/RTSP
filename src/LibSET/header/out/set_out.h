#ifndef SET_OUT_H
#define SET_OUT_H

#define XMLKEY_OUT "out"

#include <string>
#include <iostream>
#include "set_base.h"

using namespace pugi;

namespace SET
{
    class SET_OUT : public virtual SET_BASE
    {
    public:
        SET_OUT();
        ~SET_OUT();

        void check();
        void help();

    private:
    };

} // namespace
#endif
