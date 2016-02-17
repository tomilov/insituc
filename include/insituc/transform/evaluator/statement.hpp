#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::statement
evaluate(ast::statement const & _statement);

ast::statement
evaluate(ast::statement && _statement);


ast::statements
evaluate(ast::statements const & _statements);

ast::statements
evaluate(ast::statements && _statements);

}
}
