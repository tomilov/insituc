#pragma once

#include <type_traits>

namespace insituc
{

template< typename what, typename ...where >
struct is_contained;

template< typename what >
struct is_contained< what >
        : std::false_type
{

};

template< typename what, typename ...where >
struct is_contained< what, what, where... >
        : std::true_type
{

};

template< typename what, typename other, typename ...where >
struct is_contained< what, other, where... >
        : is_contained< what, where... >
{

};

template< typename what, typename ...where >
constexpr bool is_contained_v = is_contained< what, where... >::value;

template< typename ...types >
struct pack
{

};

template< typename what, typename ...types, typename ...where >
struct is_contained< what, pack< types... >, where... >
        : is_contained< what, types..., where... >
{

};

template< typename callee, typename ...arguments >
constexpr bool is_nothrow_callable_v = noexcept(std::declval< callee >(std::declval< arguments >()...));

}
