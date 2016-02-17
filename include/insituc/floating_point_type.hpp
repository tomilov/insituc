// the header should declare (custom) floating point type G
// and corresponding fundamental floating point type alias F
#pragma once

#if 0
#include <cfenv>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragma"
#pragma STDC FENV_ACCESS ON
#pragma clang diagnostic pop
#endif

namespace insituc
{

#if 0
using F = float;
#elif 1
using F = double;
#else
using F = long double;
#endif

}

#if 1

#include "floating_point_type_wrapper.hpp"

#else

#include <cmath>

namespace floating_point_type // extend the definition for <random>
{

template< typename F >
#if 0
using G = float;
#elif 1
using G = double;
#else
using G = long double;
#endif

}

#endif

namespace insituc
{

/*
// following functions must be defined for (custom) floating-point type G in its enclosing namespace
signbit;
fpclassify;
isnan;
isinf;
isunordered;
isgreater;
isless;
islessgreater;
abs;
pow;
exp;
exp2;
log;
log2;
log10;
log1p;
nearbyint;
trunc;
sqrt;
cos;
sin;
tan;
atan;
atan2;
asin;
acos;
logb;
scalbn;
fmod;
remainder;
*/

using G = ::floating_point_type::G< F >;

extern G const zero;
extern G const one;

}
