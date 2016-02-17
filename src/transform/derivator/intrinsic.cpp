#include <insituc/transform/derivator/intrinsic.hpp>

#include <insituc/utility/append.hpp>

#include <utility>
#include <stdexcept>

#include <cassert>

namespace insituc
{
namespace transform
{

namespace
{

using I = ast::intrinsic_invocation;
using B = ast::binary_expression;
using U = ast::unary_expression;
using R = ast::rvalue_list;
using O = ast::operand;

bool
intrinsic_chs(ast::rvalues && /*_arguments*/, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(I{ast::intrinsic::chs, {std::move(_darguments)}});
    return true;
}

bool
intrinsic_abs(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_twice(ast::rvalues && /*_arguments*/, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(I{ast::intrinsic::twice, {std::move(_darguments)}});
    return true;
}

bool
intrinsic_sumsqr(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_sqrt(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{R{std::move(_darguments)}, ast::binary::div, I{ast::intrinsic::twice, {append< ast::rvalues >(I{ast::intrinsic::sqrt, {std::move(_arguments)}})}}});
    return true;
}

bool
intrinsic_round(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & _results)
{
    _results.emplace_back(ast::constant::zero);
    return true;
}

bool
intrinsic_trunc(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & _results)
{
    _results.emplace_back(ast::constant::zero);
    return true;
}

bool
intrinsic_remainder(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_cos(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(U{ast::unary::minus, B{I{ast::intrinsic::sin, {std::move(_arguments)}}, ast::binary::mul, R{std::move(_darguments)}}});
    return true;
}

bool
intrinsic_sin(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{I{ast::intrinsic::cos, {std::move(_arguments)}}, ast::binary::mul, R{std::move(_darguments)}});
    return true;
}

bool
intrinsic_sincos(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    ast::rvalues darguments_ = _darguments;
    if (!intrinsic_sin(ast::rvalues(_arguments), std::move(_darguments), _results)) {
        return false;
    }
    return intrinsic_cos(std::move(_arguments), std::move(darguments_), _results);
}

bool
intrinsic_tg(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{R{std::move(_darguments)}, ast::binary::div, I{ast::intrinsic::sqr, {append< ast::rvalues >(I{ast::intrinsic::cos, {std::move(_arguments)}})}}});
    return true;
}

bool
intrinsic_ctg(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(U{ast::unary::minus, B{R{std::move(_darguments)}, ast::binary::div, I{ast::intrinsic::sqr, {append< ast::rvalues >(I{ast::intrinsic::sin, {std::move(_arguments)}})}}}});
    return true;
}

bool
intrinsic_arctg(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{R{std::move(_darguments)}, ast::binary::div, B{ast::constant::one, ast::binary::add, I{ast::intrinsic::sqr, {std::move(_arguments)}}}});
    return true;
}

bool
intrinsic_atan2(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_poly(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_frac(ast::rvalues && /*_arguments*/, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.push_back(R{std::move(_darguments)});
    return true;
}

bool
intrinsic_intrem(ast::rvalues && /*_arguments*/, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_front(ast::constant::zero);
    _results.push_back(R{std::move(_darguments)});
    return true;
}

bool
intrinsic_fracint(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_pow(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_exp(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{I{ast::intrinsic::exp, {std::move(_arguments)}}, ast::binary::mul, R{std::move(_darguments)}});
    return true;
}

bool
intrinsic_pow2(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{B{I{ast::intrinsic::pow2, {std::move(_arguments)}}, ast::binary::mul, ast::constant::ln2}, ast::binary::mul, R{std::move(_darguments)}});
    return true;
}

bool
intrinsic_sqr(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{I{ast::intrinsic::twice, {std::move(_arguments)}}, ast::binary::mul, R{std::move(_darguments)}});
    return true;
}

bool
intrinsic_log(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_ln(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{R{std::move(_darguments)}, ast::binary::div, R{std::move(_arguments)}});
    return true;
}

bool
intrinsic_log2(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{R{std::move(_darguments)}, ast::binary::div, B{R{std::move(_arguments)}, ast::binary::mul, ast::constant::ln2}});
    return true;
}

bool
intrinsic_lg(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{B{R{std::move(_darguments)}, ast::binary::mul, ast::constant::l2e}, ast::binary::div, B{R{std::move(_arguments)}, ast::binary::mul, ast::constant::l2t}});
    return true;
}

bool
intrinsic_yl2xp1(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_scale2(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_extract(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_pow2m1(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    return intrinsic_pow2(std::move(_arguments), std::move(_darguments), _results);
}

bool
intrinsic_arcsin(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(B{R{std::move(_darguments)}, ast::binary::div, I{ast::intrinsic::sqrt, {append< ast::rvalues >(B{ast::constant::one, ast::binary::sub, I{ast::intrinsic::sqr, {std::move(_arguments)}}})}}});
    return true;
}

bool
intrinsic_arccos(ast::rvalues && _arguments, ast::rvalues && _darguments, ast::rvalues & _results)
{
    _results.emplace_back(U{ast::unary::minus, B{R{std::move(_darguments)}, ast::binary::div, I{ast::intrinsic::sqrt, {append< ast::rvalues >(B{ast::constant::one, ast::binary::sub, I{ast::intrinsic::sqr, {std::move(_arguments)}}})}}}});
    return true;
}

bool
intrinsic_max(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
intrinsic_min(ast::rvalues && /*_arguments*/, ast::rvalues && /*_darguments*/, ast::rvalues & /*_results*/)
{
    return false;
}

bool
derive(ast::intrinsic const _intrinsic,
       ast::rvalues && _arguments,
       ast::rvalues && _darguments,
       ast::rvalues & _results)
{
    switch (_intrinsic) {
    case ast::intrinsic::chs       : return intrinsic_chs      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::abs       : return intrinsic_abs      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::twice     : return intrinsic_twice    (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::sumsqr    : return intrinsic_sumsqr   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::sqrt      : return intrinsic_sqrt     (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::round     : return intrinsic_round    (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::trunc     : return intrinsic_trunc    (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::remainder : return intrinsic_remainder(std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::cos       : return intrinsic_cos      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::sin       : return intrinsic_sin      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::sincos    : return intrinsic_sincos   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::tg        : return intrinsic_tg       (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::ctg       : return intrinsic_ctg      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::arctg     : return intrinsic_arctg    (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::atan2     : return intrinsic_atan2    (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::poly      : return intrinsic_poly     (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::frac      : return intrinsic_frac     (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::intrem    : return intrinsic_intrem   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::fracint   : return intrinsic_fracint  (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::pow       : return intrinsic_pow      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::exp       : return intrinsic_exp      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::pow2      : return intrinsic_pow2     (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::sqr       : return intrinsic_sqr      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::log       : return intrinsic_log      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::ln        : return intrinsic_ln       (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::log2      : return intrinsic_log2     (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::lg        : return intrinsic_lg       (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::yl2xp1    : return intrinsic_yl2xp1   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::scale2    : return intrinsic_scale2   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::extract   : return intrinsic_extract  (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::pow2m1    : return intrinsic_pow2m1   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::arcsin    : return intrinsic_arcsin   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::arccos    : return intrinsic_arccos   (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::max       : return intrinsic_max      (std::move(_arguments), std::move(_darguments), _results);
    case ast::intrinsic::min       : return intrinsic_min      (std::move(_arguments), std::move(_darguments), _results);
    }
}

} // static namespace

ast::rvalues
derive_intrinsic(ast::intrinsic const _intrinsic,
                 ast::rvalues && _arguments,
                 ast::rvalues && _darguments)
{
    ast::rvalues results_;
    if (!derive(_intrinsic, std::move(_arguments), std::move(_darguments), results_)) {
        throw std::runtime_error("intrinsic cannot be derived");
    }
    assert(!results_.empty());
    return results_;
}

}
}
