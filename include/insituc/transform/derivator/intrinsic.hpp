#pragma once

#include <insituc/transform/derivator/context.hpp>

namespace insituc
{
namespace transform
{

ast::rvalues
derive_intrinsic(ast::intrinsic const _intrinsic,
                 ast::rvalues && _arguments,
                 ast::rvalues && _darguments);

}
}
