#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::operand
evaluate(ast::unary const _operator,
         ast::operand && _operand);

ast::operand
evaluate(ast::operand && _lhs,
         ast::binary const _operator,
         ast::operand && _rhs);

}
}
