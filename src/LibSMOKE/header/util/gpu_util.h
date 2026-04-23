#ifndef GPU_UTIL_H
#define GPU_UTIL_H

#include <cmath>

namespace SMOKE {
	inline unsigned int ceil_div(int a, int b) {
        return static_cast<unsigned int>(
            std::ceil(static_cast<double>(a) / b)
        );
	}
}

#endif // GPU_UTIL_H
