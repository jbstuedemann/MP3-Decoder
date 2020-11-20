#ifndef INCLUDE_KERNEL_UTIL_MATH_H_
#define INCLUDE_KERNEL_UTIL_MATH_H_

#include "stdint.h"
#include <math.h>
#undef M_PI
#undef M_SQRT2

namespace util {

    namespace math {

        constexpr double M_PI = 3.141592653589793;
        constexpr double M_SQRT2 = 1.41421356237309505;

        inline double power(double a, double b){ return std::pow(a, b); }

        double sin(double n);
        inline double cos(double n) { return sin(M_PI / 2 - n); }

    }  // namespace math

}  // namespace util

#endif  // INCLUDE_KERNEL_UTIL_MATH_H_