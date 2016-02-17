#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::entry_definition
evaluate(ast::entry_definition const & _entry);

ast::entry_definition
evaluate(ast::entry_definition && _entry);


ast::program
evaluate(ast::program const & _program);

ast::program
evaluate(ast::program && _program);

}
}
