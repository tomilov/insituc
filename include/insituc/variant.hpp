#pragma once

#if !__has_include(<versatile.hpp>)
#error "Add path to 'versatile' library headers to CPLUS_INCLUDE_PATH or PATH or by using -I compiler derective"
#endif
#include <versatile.hpp>

namespace insituc
{

using ::versatile::unwrap_type_t;
using ::versatile::in_place;
using ::versatile::visit;
using ::versatile::multivisit;
using ::versatile::aggregate_wrapper;
using ::versatile::recursive_wrapper;
using ::versatile::versatile;
using ::versatile::variant;

}
