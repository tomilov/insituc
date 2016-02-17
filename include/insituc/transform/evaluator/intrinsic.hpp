#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::rvalues
evaluate(ast::intrinsic const _intrinsic,
         ast::rvalues && _arguments);

}
}
