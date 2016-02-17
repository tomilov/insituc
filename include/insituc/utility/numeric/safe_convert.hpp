#pragma once

#include <insituc/utility/numeric/safe_compare.hpp>

#include <type_traits>
#include <stdexcept>
#include <limits>

namespace insituc
{

template< typename to, typename from >
constexpr
bool
is_includes(from const & _value) noexcept
{
    return !(less(_value, std::numeric_limits< to >::min()) || less(std::numeric_limits< to >::max(), _value));
}

template< typename to, typename from >
constexpr
to
convert_if_includes(from && _value)
{
    return is_includes< to >(_value) ? static_cast< to >(std::forward< from >(_value))
                                     : throw std::out_of_range("can't convert: out of range");
}

}
