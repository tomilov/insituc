#pragma once

#include <insituc/floating_point_type.hpp>

#include <type_traits>
#include <string>

#include <cstdlib>
#include <cstddef>

#if defined(_DEBUG) || defined(DEBUG)
#include <iostream>
#include <iomanip>
#endif

namespace insituc
{

static_assert(std::is_floating_point_v< F >, "F must be fundamental floating point type");

constexpr bool use_float       = std::is_same_v< F, float       >;
constexpr bool use_double      = std::is_same_v< F, double      >;
constexpr bool use_long_double = std::is_same_v< F, long double >;

static_assert((use_float || use_double || use_long_double), "F must be fundamental floating point type");

using size_type = std::size_t;
using difference_type = std::ptrdiff_t;

using namespace std::string_literals;
using string_type = std::string;
using char_type = typename string_type::value_type;

using base_iterator_type = typename string_type::const_iterator;

}
