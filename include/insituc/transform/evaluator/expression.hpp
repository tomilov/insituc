#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::operand
evaluate(ast::operand const & _operand);

ast::operand
evaluate(ast::operand && _operand);


ast::rvalues
evaluate(ast::rvalues const & _rvalues);

ast::rvalues
evaluate(ast::rvalues && _rvalues);

}
}
