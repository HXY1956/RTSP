#include <iostream>
#include <iomanip>
#include "base_mutex.h"

namespace BASE
{
    BASE_MUTEX::BASE_MUTEX()
    {
#ifdef USE_OPENMP
        omp_init_lock(&_mutex);
        isLock = false;
#endif // USE_OPENMP
        isLock = false;
    }

    BASE_MUTEX::BASE_MUTEX(const BASE_MUTEX& Other)
    {
#ifdef USE_OPENMP
        omp_init_lock(&this->_mutex);
        isLock = Other.isLock;
#endif
        isLock = Other.isLock;
    }

    // destructor
    // ----------
    BASE_MUTEX::~BASE_MUTEX()
    {
    }

    BASE_MUTEX BASE_MUTEX::operator=(const BASE_MUTEX& Other)
    {
        return BASE_MUTEX();
    }

    // ----------
    void BASE_MUTEX::lock()
    {
#ifdef USE_OPENMP
        if (!isLock)
        {
            omp_set_lock(&_mutex);
            isLock = true;
        }
#else
        if (!isLock)
        {
            _mutex.lock();
            isLock = true;
        }
#endif // USE_OPENMP
    }

    // ----------
    void BASE_MUTEX::unlock()
    {
#ifdef USE_OPENMP
        if (isLock)
        {
            omp_unset_lock(&_mutex);
            isLock = false;
        }
#else
        if (isLock)
        {
            _mutex.unlock();
            isLock = false;
        }
#endif
    }

} // namespace
