#include <insituc/transform/transform.hpp>

#include <insituc/transform/derivator/derivator.hpp>

#include <insituc/utility/append.hpp>

namespace insituc
{
namespace transform
{

ast::programs
Jacobian(ast::program const & _functions,
         ast::symbols _wrts)
{
    ast::programs derivatives_;
    for (ast::symbol & wrt_ : _wrts) {
        derivatives_.push_back(derive(_functions, append< ast::symbols >(std::move(wrt_))));
    }
    return derivatives_;
}

}
}
