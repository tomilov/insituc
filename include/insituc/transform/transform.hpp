#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::programs
Jacobian(ast::program const & _functions,
         ast::symbols _wrts); // to calculate a Hessian apply the function to a joined results of a first application of the function to an input _functions

void
Jacobian(ast::program const && _functions,
         ast::symbols _wrts) = delete; // result will contain references to the _primitives' internals

}
}
