#pragma once

#include <insituc/base_types.hpp>
#include <insituc/ast/ast.hpp>

#include <deque>
#include <set>
#include <map>

namespace insituc
{
namespace meta
{

using symbol_type = ast::identifier;
using symbols_type = std::deque< symbol_type >;
using symbol_set_type = std::set< symbol_type >;
using symbol_mapping_type = std::map< symbol_type, size_type const >;

}
}
