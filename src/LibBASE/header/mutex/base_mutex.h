#ifndef BASE_MUTEX_H
#define BASE_MUTEX_H
#include <thread>
#include <mutex>

namespace BASE
{
    /** @brief class for BASE_MUTEX. */
    class BASE_MUTEX
    {
    public:
        /** @brief default constructor. */
        BASE_MUTEX();

        /** @brief copy constructor. */
        BASE_MUTEX(const BASE_MUTEX& Other);

        /** @brief default destructor. */
        ~BASE_MUTEX();

        /** @brief override operator =. */
        BASE_MUTEX operator=(const BASE_MUTEX& Other);

        /** @brief lock. */
        void lock();

        /** @brief unlock. */
        void unlock();

        bool isLock = false;

    protected:
#ifdef USE_OPENMP
        omp_lock_t _mutex;
#else
        std::mutex _mutex;

#endif // USE_OPENMP
    };
} // namespace

#endif