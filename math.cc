#include "math.h"

namespace util {

    namespace math {

        double wrapAngle(double angle) {
            double twoPi = 2.0 * M_PI;
            return angle - twoPi * (long long)(angle / twoPi);
        }

        double sin(double n) {
            n = wrapAngle(n);
            double n_pow = n * n * n;
            double n_fac = 2 * 3;
            int fac = 4;
            double ret = n;
            bool minus = true;
            for (int i = 0; i < 8; i++) {
                if (minus) {
                    ret -= n_pow / n_fac;
                } else {
                    ret += n_pow / n_fac;
                }
                n_pow *= n * n;
                n_fac *= (fac) * (fac + 1);
                fac += 2;
                minus = !minus;
            }
            return ret;
        }

    }  // namespace math

}  // namespace util