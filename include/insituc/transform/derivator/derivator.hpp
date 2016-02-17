#pragma once

#include <insituc/ast/ast.hpp>

namespace insituc
{
namespace transform
{

ast::program
derive(ast::program const & _primitives,
       ast::identifier _target);

void
derive(ast::program const && _primitives,
       ast::identifier _derivative_name) = delete; // result should contain permanent references to the _primitives' internals

ast::program
derive(ast::program const & _primitives,
       ast::symbols _targets,
       ast::symbols const & _wrts);

void
derive(ast::program const && _primitives,
       ast::symbols _targets,
       ast::symbols _wrts) = delete; // result should contain permanent references to the _primitives' internals


ast::program
derive(ast::program const & _primitives,
       ast::symbols const & _wrts);

void
derive(ast::program const && _primitives,
       ast::symbols _wrts) = delete; // result should contain permanent references to the _primitives' internals

}
}
