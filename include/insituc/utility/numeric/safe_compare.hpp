#pragma once

#include <insituc/type_traits.hpp>

#include <type_traits>

namespace insituc
{

// http://stackoverflow.com/questions/16752954/ just ::operator <, but safe
template< typename lhs, typename rhs >
constexpr
std::enable_if_t< (std::is_signed_v< lhs > && !std::is_signed_v< rhs >), bool >
less(lhs const & _lhs, rhs const & _rhs) noexcept
{
    using type = typename std::common_type< lhs, rhs >::type;
    return (_lhs < static_cast< lhs >(0)) || (static_cast< type >(_lhs) < static_cast< type >(_rhs));
}

template< typename lhs, typename rhs >
constexpr
std::enable_if_t< (!std::is_signed_v< lhs > && std::is_signed_v< rhs >), bool >
less(lhs const & _lhs, rhs const & _rhs) noexcept
{
    using type = typename std::common_type< lhs, rhs >::type;
    return !(_rhs < static_cast< rhs >(0)) && (static_cast< type >(_lhs) < static_cast< type >(_rhs));
}

template< typename lhs, typename rhs >
constexpr
std::enable_if_t< (std::is_signed_v< lhs > == std::is_signed_v< rhs >), bool >
less(lhs const & _lhs, rhs const & _rhs) noexcept
{
    return (_lhs < _rhs);
}

}
