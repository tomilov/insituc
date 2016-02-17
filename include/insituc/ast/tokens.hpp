#pragma once

#include <insituc/base_types.hpp>

#include <utility>

namespace insituc
{
namespace ast
{

enum class assign
{
    assign,
    plus_assign,
    minus_assign,
    times_assign,
    divide_assign,
    mod_assign,
    raise_assign
};

enum class unary
{
    plus,
    minus
};

enum class binary
{
    add,
    sub,
    mul,
    div,
    mod, // fmod
    pow
};

constexpr
size_type
precedence(ast::binary const _operator) noexcept
{
    switch (_operator) {
    case ast::binary::add :
    case ast::binary::sub : {
        return 1;
    }
    case ast::binary::mul :
    case ast::binary::div :
    case ast::binary::mod : {
        return 2;
    }
    case ast::binary::pow : {
        return 3;
    }
    }
}

constexpr
size_type
is_commutative(ast::binary const _operator) noexcept
{
    switch (_operator) {
    case ast::binary::add :
    case ast::binary::mul : {
        return true;
    }
    case ast::binary::sub :
    case ast::binary::div :
    case ast::binary::mod :
    case ast::binary::pow : {
        return false;
    }
    }
}

enum class constant
{
    zero,
    one,
    pi,
    l2e,
    l2t,
    lg2,
    ln2
};

enum class intrinsic
{
    // TODO: implement car and cdr for first and last elements respectively in case of variadic function or first and second for binary variant
    // TODO: decide about fma necessety
    chs,
    abs,
    twice,
    sumsqr,
    sqrt,
    round,
    trunc,
    remainder, // FPREM1
    cos,
    sin,
    sincos,
    tg,
    ctg,
    arctg,
    atan2,
    poly,
    frac,
    intrem,
    fracint, // modf
    pow, // TODO: integer literal power (usefull __gnu_cxx::power() from #include <ext/numeric>)
    exp,
    pow2,
    sqr,
    log,
    ln,
    log2,
    lg,
    yl2xp1,  // y * log2(x + 1) // The content of x must be within the range of -(1-½√2) to +(1-½√2), i.e. -0.29289... to +0.29289
    scale2,  // x * 2^y // ldexp <=> scale2
    extract, // extracts the significand and the exponent <=> (x / scale2(1.0, logb(x))), logb(x) // frexp
    pow2m1,  // 2^x - 1 // The value of x must be within the range of -1.0 to +1.0
    arcsin,
    arccos,
    max,
    min
};

enum class keyword
{
    local_,
    begin_,
    end_,
    function_,
    return_
};

constexpr
char_type const *
c_str(assign const _assign) noexcept
{
    switch (_assign) {
    case assign::assign        : return "=";
    case assign::plus_assign   : return "+=";
    case assign::minus_assign  : return "-=";
    case assign::times_assign  : return "*=";
    case assign::divide_assign : return "/=";
    case assign::mod_assign    : return "%=";
    case assign::raise_assign  : return "^=";
    }
}

constexpr
char_type const *
c_str(unary const _unary) noexcept
{
    switch (_unary) {
    case unary::plus : return "+";
    case unary::minus : return "-";
    }
}

constexpr
char_type const *
c_str(binary const _binary) noexcept
{
    switch (_binary) {
    case binary::add   : return "+";
    case binary::sub   : return "-";
    case binary::mul   : return "*";
    case binary::div   : return "/";
    case binary::mod   : return "%";
    case binary::pow   : return "^";
    }
}

constexpr
char_type const *
c_str(constant const _constant) noexcept
{
    switch (_constant) {
    case constant::zero : return "zero";
    case constant::one  : return "one";
    case constant::pi   : return "pi";
    case constant::l2e  : return "l2e";
    case constant::l2t  : return "l2t";
    case constant::lg2  : return "lg2";
    case constant::ln2  : return "ln2";
    }
}

constexpr
char_type const *
c_str(intrinsic const _intrinsic) noexcept
{
    switch (_intrinsic) {
    case intrinsic::chs       : return "chs";
    case intrinsic::abs       : return "abs";
    case intrinsic::twice     : return "twice";
    case intrinsic::sumsqr    : return "sumsqr";
    case intrinsic::sqrt      : return "sqrt";
    case intrinsic::round     : return "round";
    case intrinsic::trunc     : return "trunc";
    case intrinsic::remainder : return "remainder";
    case intrinsic::cos       : return "cos";
    case intrinsic::sin       : return "sin";
    case intrinsic::sincos    : return "sincos";
    case intrinsic::tg        : return "tg";
    case intrinsic::ctg       : return "ctg";
    case intrinsic::arctg     : return "arctg";
    case intrinsic::atan2     : return "atan2";
    case intrinsic::poly      : return "poly";
    case intrinsic::frac      : return "frac";
    case intrinsic::intrem    : return "intrem";
    case intrinsic::fracint   : return "fracint";
    case intrinsic::pow       : return "pow";
    case intrinsic::exp       : return "exp";
    case intrinsic::pow2      : return "pow2";
    case intrinsic::sqr       : return "sqr";
    case intrinsic::log       : return "log";
    case intrinsic::ln        : return "ln";
    case intrinsic::log2      : return "log2";
    case intrinsic::lg        : return "lg";
    case intrinsic::yl2xp1    : return "yl2xp1";
    case intrinsic::scale2    : return "scale2";
    case intrinsic::extract   : return "extract";
    case intrinsic::pow2m1    : return "pow2m1";
    case intrinsic::arcsin    : return "arcsin";
    case intrinsic::arccos    : return "arccos";
    case intrinsic::max       : return "max";
    case intrinsic::min       : return "min";
    }
}

constexpr
char_type const *
c_str(keyword const _keyword) noexcept
{
    switch (_keyword) {
    case keyword::local_    : return "local";
    case keyword::begin_    : return "begin";
    case keyword::end_      : return "end";
    case keyword::function_ : return "function";
    case keyword::return_   : return "return";
    }
}

template< typename token >
constexpr
std::pair< char_type const *, token >
to_pair(token const _token) noexcept
{
    static_assert(std::is_enum_v< token >);
    return {c_str(_token), _token};
}

}
}
