#pragma once

namespace insituc
{

template< typename type >
constexpr type static_const = {}; // to avoid ODR violations // http://ericniebler.github.io/std/wg21/D4381.html

}
