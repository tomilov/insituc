#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::entry_definition
leaf(ast::entry_definition const & _entry,
     ast::symbol const & _name);

}
}
