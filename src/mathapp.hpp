#pragma once

#include <type_traits>
#include <cmath>
#include <assert.h>

namespace tool
{
    using fpoint_default = double;
    using fpoint_fast = float;

    constexpr double PI = 3.14159265358979323846;
    constexpr double PI2 = PI * 2.0;
    constexpr double PI_2 = PI / 2.0;
    constexpr double _2_PI = 2.0 / PI;

    template<typename T,
        typename = typename std::enable_if_t<std::is_floating_point<T>::value>,
        typename = typename std::enable_if_t<std::is_same<T, fpoint_default>::value> >
        inline T hypot_fast(T a, T b)
    {
        a = std::fabs(a);
        b = std::fabs(b);

        const T
            min_ = a < b ? a : b,
            max_ = a < b ? b : a;

        if (max_ == static_cast<T>(0.0) || std::isinf(max_))
            return max_;

        const T q = min_ / max_;
        return max_ * std::sqrt(static_cast<T>(1.0) + q * q);
    }

    template<typename T,
        typename = typename std::enable_if_t<std::is_floating_point<T>::value>,
        typename = typename std::enable_if_t<!std::is_same<T, fpoint_default>::value>,
        typename = typename std::enable_if_t<std::numeric_limits<T>::digits10 <= std::numeric_limits<fpoint_default>::digits10> >
        inline T hypot_fast(T a, T b)
    {
        fpoint_default a_ = a, b_ = b;
        return static_cast<T>(std::sqrt(a_ * a_ + b_ * b_));
    }

    template <typename T>
    std::enable_if_t<std::is_floating_point<T>::value, T>
        inline hypot_naive(T a, T b)
    {
#if defined(MY_DEBUG)
        constexpr T minl = static_cast<T>(1e-19), maxl = static_cast<T>(1e+19);
        assert(minl <= a && a <= maxl && minl <= b && b <= maxl);
#endif
        return std::sqrt(a * a + b * b);
    }

    // Polynomial approximating arctangenet on the range -1,1.
    // Max error < 0.005 (or 0.29 degrees)
    template <typename T>
    std::enable_if_t<std::is_floating_point<T>::value, T>
        inline atan_approx(T a)
    {
        // taken from https://www.dsprelated.com/showarticle/1052.php
        constexpr T k1 = static_cast<T>(0.97239411), k2 = static_cast<T>(-0.19194795);
        return (k1 + k2 * a * a) * a;
    }

    template <typename T>
    std::enable_if_t<std::is_floating_point<T>::value, T>
        inline atan2_approx(T y, T x)
    {
        // taken from https://www.dsprelated.com/showarticle/1052.php
        T ay = fabs(y), ax = fabs(x);
        const bool invert = ay > ax;
        T z = invert ? ax / ay : ay / ax;                    // [0,1]
        T result = atan_approx(z);                           // [0,pi/4]
        if (invert) result = static_cast<T>(PI_2) - result;  // [0,pi/2]
        if (x < 0) result = static_cast<T>(PI) - result;     // [0,pi]
        result = copysign(result, y);                        // [-pi,pi]
        return result;
    }

    // Remez minimax polynomial approximation of sin(x)
    template <typename T>
    std::enable_if_t<std::is_floating_point<T>::value, T>
        sin_approx(T x)
    {
        /////////////////////////////////////////////////
        //  https://en.wikipedia.org/wiki/Remez_algorithm
        // Coefficients attained via:
        //  http://lolengine.net/wiki/doc/maths/remez

        bool negate_result;

        if (x < 0) {
            x = -x;
            negate_result = true;
        } else {
            negate_result = false;
        }

        // Get x within 0..pi/2 range:
        if (x > PI_2)
        {
            T x_div_half_pi = x * static_cast<T>(_2_PI);
            int quotient = static_cast<int>(x_div_half_pi);
            T x_past_quad = x - static_cast<T>(quotient) * static_cast<T>(PI_2);

            switch (quotient % 4) {
            default: // Default case shouldn't happen; fall through

            case 0: // x is < PI/2
                x = x_past_quad;
                break;
            case 1: // x is < PI
                x = static_cast<T>(PI_2) - x_past_quad;
                break;
            case 2: // x is < 3*PI/2
                x = x_past_quad;
                negate_result = !negate_result;
                break;
            case 3: // x < 2*PI
                x = static_cast<T>(PI_2) - x_past_quad;
                negate_result = !negate_result;
                break;
            }
        }

        // Max error: 1.140089580346679823414178063937590438393e-4
        //MAX ERROR: 1.140833e-04   AVG ERROR: 5.777799e-05

        // Coefficients generated using http://lolengine.net/wiki/oss/lolremez
        // Specifically, these were generated using the method outlined on:
        // http://lolengine.net/wiki/doc/maths/remez/tutorial-fixing-parameters
        // whereby, the first coefficient (a1) is forced to 1.0 and thus
        // eliminated.
        constexpr auto a3 = static_cast<T>(-1.660786242169313753522239789881311891420e-1);
        constexpr auto a5 = static_cast<T>(7.633773374658546665581958822838108771028e-3);
        T x2 = x * x;
        T x3 = x2 * x;
        T result = x + x3 * a3 + x2 * x3*a5;
        result = std::clamp(result, static_cast<T>(-1.0), static_cast<T>(1.0));
        return negate_result ? -result : result;
    }

    // Remez minimax polynomial approximation of cos(x)
    template <typename T>
    inline T cos_approx(T x)
    {
        return sin_approx(static_cast<T>(PI_2) - x);
    }
}

