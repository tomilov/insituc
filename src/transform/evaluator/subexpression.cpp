#include <insituc/transform/evaluator/subexpression.hpp>

#include <insituc/floating_point_type.hpp>

#include <versatile/visit.hpp>

#include <type_traits>
#include <utility>
#include <stdexcept>
#include <experimental/optional>

namespace insituc
{
namespace transform
{

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
namespace subexpression
{

using result_type = ast::operand;

using U = ast::unary_expression;

namespace unary
{
namespace
{

template< typename type >
std::enable_if_t< !std::is_same_v< type, ast::empty >, result_type >
evaluate(type && _ast)
{
    static_assert(!std::is_lvalue_reference_v< type >, "!");
    static_assert(!std::is_const_v< type >, "!");
    return U{ast::unary::minus, std::move(_ast)};
}

template< typename type >
[[noreturn]]
std::enable_if_t< std::is_same_v< type, ast::empty >, result_type >
evaluate(type && /*_empty*/)
{
    throw std::logic_error("empty operand in subexpression evaluator is not allowed");
}

result_type
evaluate(G && _value)
{
    return -std::move(_value);
}

result_type
evaluate(U && _unary_expression)
{
    assert(_unary_expression.operator_ != ast::unary::plus);
    return std::move(_unary_expression.operand_);
}

result_type
evaluate(ast::intrinsic_invocation && _intrinsic_invocation)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_intrinsic_invocation.intrinsic_) {
#pragma clang diagnostic pop
    case ast::intrinsic::chs : {
        assert(_intrinsic_invocation.argument_list_.rvalues_.size() == 1);
        return std::move(_intrinsic_invocation.argument_list_.rvalues_.back());
    }
    default : {
        break;
    }
    }
    return U{ast::unary::minus, std::move(_intrinsic_invocation)};
}

// TODO: intrinsic abs chs move up

} // static namespace
}

using B = ast::binary_expression;

namespace binary
{
namespace
{

template< typename lhs, typename rhs >
std::enable_if_t< !(std::is_same_v< lhs, ast::empty > || std::is_same_v< rhs, ast::empty >), result_type >
evaluate(lhs && _lhs,
         ast::binary const _operator,
         rhs && _rhs)
{
    static_assert(!std::is_lvalue_reference_v< lhs >, "!");
    static_assert(!std::is_lvalue_reference_v< rhs >, "!");
    static_assert(!std::is_const_v< lhs >, "!");
    static_assert(!std::is_const_v< rhs >, "!");
    return B{std::move(_lhs), _operator, std::move(_rhs)};
}

template< typename lhs, typename rhs >
[[noreturn]]
std::enable_if_t< (std::is_same_v< lhs, ast::empty > || std::is_same_v< rhs, ast::empty >), result_type >
evaluate(lhs && /*_lhs*/,
         ast::binary const /*_operator*/,
         rhs && /*_rhs*/)
{
    throw std::logic_error("empty operand in subexpression evaluator is not allowed");
}

template< typename lhs >
result_type
evaluate(lhs && _lhs,
         ast::binary const _operator,
         U && _rhs)
{
    assert(_rhs.operator_ == ast::unary::minus);
    switch (_operator) {
    case ast::binary::add : {
        return B{std::forward< lhs >(_lhs), ast::binary::sub, std::move(_rhs.operand_)};
    }
    case ast::binary::sub : {
        return B{std::forward< lhs >(_lhs), ast::binary::add, std::move(_rhs.operand_)};
    }
    case ast::binary::mul : {
        return U{ast::unary::minus, B{std::forward< lhs >(_lhs), ast::binary::mul, std::move(_rhs.operand_)}};
    }
    case ast::binary::div : {
        return U{ast::unary::minus, B{std::forward< lhs >(_lhs), ast::binary::div, std::move(_rhs.operand_)}};
    }
    case ast::binary::mod : {
        return B{std::forward< lhs >(_lhs), ast::binary::mod, std::move(_rhs.operand_)};
    }
    case ast::binary::pow : {
        break; // it is meaningless to do `a ^ (-b) -> 1 / (a ^ b)` due to performance reasons (division may take additional execution time)
    }
    }
    return B{std::forward< lhs >(_lhs), _operator, std::move(_rhs)};
}

template< typename rhs >
result_type
evaluate(U && _lhs,
         ast::binary const _operator,
         rhs && _rhs)
{
    assert(_lhs.operator_ == ast::unary::minus);
    switch (_operator) {
    case ast::binary::add : {
        return B{std::forward< rhs >(_rhs), ast::binary::sub, std::move(_lhs.operand_)};
    }
    case ast::binary::sub : {
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::add, std::forward< rhs >(_rhs)}};
    }
    case ast::binary::mul : {
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::mul, std::forward< rhs >(_rhs)}};
    }
    case ast::binary::div : {
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::div, std::forward< rhs >(_rhs)}};
    }
    case ast::binary::mod : {
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::mod, std::forward< rhs >(_rhs)}};
    }
    case ast::binary::pow : {
        break; // it is meaningless to make any assumptions about inconsistency of `(-a) ^ b -> nan` due to unknown nature of lhs during compile time
    }
    }
    return B{std::move(_lhs), _operator, std::forward< rhs >(_rhs)};
}

result_type
evaluate(U && _lhs,
         ast::binary const _operator,
         U && _rhs)
{
    assert(_lhs.operator_ == ast::unary::minus);
    assert(_rhs.operator_ == ast::unary::minus);
    switch (_operator) {
    case ast::binary::add : {
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::add, std::move(_rhs.operand_)}};
    }
    case ast::binary::sub : {
        return B{std::move(_rhs.operand_), ast::binary::sub, std::move(_lhs.operand_)};
    }
    case ast::binary::mul : {
        return B{std::move(_lhs.operand_), ast::binary::mul, std::move(_rhs.operand_)};
    }
    case ast::binary::div : {
        return B{std::move(_lhs.operand_), ast::binary::div, std::move(_rhs.operand_)};
    }
    case ast::binary::mod : {
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::mod, std::move(_rhs.operand_)}};
    }
    case ast::binary::pow : {
        break; // it is meaningless to make any assumptions about inconsistency of `(-a) ^ (-b) -> 1 / ((-a) ^ b) -> 1/ nan -> nan` due to unknown nature of lhs
    }
    }
    return B{std::move(_lhs), _operator, std::move(_rhs)};
}

template< typename lhs >
result_type
evaluate(lhs && _lhs,
         ast::binary const _operator,
         G && _rhs)
{
    switch (_operator) {
    case ast::binary::add : {
        if (_rhs < zero) {
            return B{std::forward< lhs >(_lhs), ast::binary::sub, -std::move(_rhs)};
        } else if (zero < _rhs) {
            return B{std::forward< lhs >(_lhs), ast::binary::add, std::move(_rhs)};
        } else {
            return std::forward< lhs >(_lhs);
        }
        break;
    }
    case ast::binary::sub : {
        if (_rhs < zero) {
            return B{std::forward< lhs >(_lhs), ast::binary::add, -std::move(_rhs)};
        } else if (zero < _rhs) {
            return B{std::forward< lhs >(_lhs), ast::binary::sub, std::move(_rhs)};
        } else {
            return std::forward< lhs >(_lhs);
        }
        break;
    }
    case ast::binary::mul : {
        if (_rhs == zero) {
            return zero;
        } else if (_rhs == one) {
            return std::forward< lhs >(_lhs);
        } else if (_rhs == -one) {
            return unary::evaluate(std::forward< lhs >(_lhs));
        }
        break;
    }
    case ast::binary::div : {
        if (_rhs == one) {
            return std::forward< lhs >(_lhs);
        } else if (_rhs == -one) {
            return unary::evaluate(std::forward< lhs >(_lhs));
        } else if (_rhs == zero) {
            throw std::runtime_error("/ division by zero");
        }
        return B{std::forward< lhs >(_lhs), ast::binary::mul, one / _rhs};
    }
    case ast::binary::mod : {
        if (_rhs == zero) {
            throw std::runtime_error("% division by zero");
        }
        break;
    }
    case ast::binary::pow : {
        if (_rhs == zero) {
            return one;
        } else if (_rhs == one) {
            return std::forward< lhs >(_lhs);
        }
        break;
    }
    }
    return B{std::forward< lhs >(_lhs), _operator, std::move(_rhs)};
}

template< typename rhs >
result_type
evaluate(G && _lhs,
         ast::binary const _operator,
         rhs && _rhs)
{
    switch (_operator) {
    case ast::binary::add : {
        if (_lhs == zero) {
            return std::forward< rhs >(_rhs);
        }
        break;
    }
    case ast::binary::sub : {
        if (_lhs == zero) {
            return unary::evaluate(std::forward< rhs >(_rhs));
        }
        break;
    }
    case ast::binary::mul : {
        if (_lhs == zero) {
            return zero;
        } else if (_lhs == one) {
            return std::forward< rhs >(_rhs);
        } else if (_lhs == -one) {
            return unary::evaluate(std::forward< rhs >(_rhs));
        }
        break;
    }
    case ast::binary::div : {
        if (_lhs == zero) {
            return zero;
        }
        break;
    }
    case ast::binary::mod : {
        if (_lhs == zero) {
            return zero;
        }
        break;
    }
    case ast::binary::pow : {
        /*if (!(zero < _lhs)) {
                throw std::runtime_error("^ domain error: base is less than or equal to 0, but exp is not less than or equal to ​0​.");
            }*/
        if (_lhs == one) {
            return one;
        }
        break;
    }
    }
    return B{std::move(_lhs), _operator, std::forward< rhs >(_rhs)};
}

result_type
evaluate(G && _lhs,
         ast::binary const _operator,
         U && _rhs);

result_type
evaluate(U && _lhs,
         ast::binary const _operator,
         G && _rhs)
{
    assert(_lhs.operator_ == ast::unary::minus);
    switch (_operator) {
    case ast::binary::add : {
        if (_rhs == zero) {
            return std::move(_lhs);
        }
        return B{std::move(_rhs), ast::binary::sub, std::move(_lhs.operand_)};
    }
    case ast::binary::sub : {
        if (_rhs == zero) {
            return std::move(_lhs);
        }
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::add, std::move(_rhs)}};
    }
    case ast::binary::mul : {
        if (_rhs == zero) {
            return zero;
        } else if (_rhs == one) {
            return std::move(_lhs);
        } else if (_rhs == -one) {
            return unary::evaluate(std::move(_lhs));
        }
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::mul, std::move(_rhs)}};
    }
    case ast::binary::div : {
        if (_rhs == one) {
            return std::move(_lhs);
        } else if (_rhs == -one) {
            return unary::evaluate(std::move(_lhs));
        } else if (_rhs == zero) {
            throw std::runtime_error("/ division by zero");
        }
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::div, std::move(_rhs)}};
    }
    case ast::binary::mod : {
        if (_rhs == zero) {
            throw std::runtime_error("% division by zero");
        }
        return U{ast::unary::minus, B{std::move(_lhs.operand_), ast::binary::mod, std::move(_rhs)}};
    }
    case ast::binary::pow : {
        if (_rhs == zero) {
            return one;
        } else if (_rhs == one) {
            return std::move(_lhs);
        } else if (_rhs == -one) {
            return evaluate(G(one), ast::binary::div, std::move(_lhs));
        }
        break;
    }
    }
    return B{std::move(_lhs), _operator, std::move(_rhs)};
}

result_type
evaluate(G && _lhs,
         ast::binary const _operator,
         U && _rhs)
{
    assert(_rhs.operator_ == ast::unary::minus);
    switch (_operator) {
    case ast::binary::add : {
        if (_lhs == zero) {
            return std::move(_rhs);
        }
        return B{std::move(_lhs), ast::binary::sub, std::move(_rhs.operand_)};
    }
    case ast::binary::sub : {
        if (_lhs == zero) {
            return unary::evaluate(std::move(_rhs));
        }
        return B{std::move(_lhs), ast::binary::add, std::move(_rhs.operand_)};
    }
    case ast::binary::mul : {
        if (_lhs == zero) {
            return zero;
        } else if (_lhs == one) {
            return std::move(_rhs);
        } else if (_lhs == -one) {
            return unary::evaluate(std::move(_rhs));
        }
        return U{ast::unary::minus, B{std::move(_lhs), ast::binary::mul, std::move(_rhs.operand_)}};
    }
    case ast::binary::div : {
        if (_lhs == zero) {
            return zero;
        }
        return U{ast::unary::minus, B{std::move(_lhs), ast::binary::div, std::move(_rhs.operand_)}};
    }
    case ast::binary::mod : {
        if (_lhs == zero) {
            return zero;
        }
        return B{std::move(_lhs), ast::binary::mod, std::move(_rhs.operand_)};
    }
    case ast::binary::pow : {
        /*if (!(zero < _lhs)) {
                throw std::runtime_error("^ domain error: base is less than or equal to 0, but exp is not less than or equal to ​0​.");
            } else */
        if (_lhs == one) {
            return one;
        }
        break;
    }
    }
    return B{std::move(_lhs), _operator, std::move(_rhs)};
}

result_type
evaluate(G && _lhs,
         ast::binary const _operator,
         G && _rhs)
{
    switch (_operator) {
    case ast::binary::add : {
        return std::move(_lhs) + std::move(_rhs);
    }
    case ast::binary::sub : {
        return std::move(_lhs) - std::move(_rhs);
    }
    case ast::binary::mul : {
        return std::move(_lhs) * std::move(_rhs);
    }
    case ast::binary::div : {
        if (_rhs == zero) {
            throw std::runtime_error("/ division by zero.");
        }
        return std::move(_lhs) / std::move(_rhs);
    }
    case ast::binary::mod : {
        if (_rhs == zero) {
            throw std::runtime_error("% division by zero.");
        }
        return fmod(std::move(_lhs), std::move(_rhs));
    }
    case ast::binary::pow : {
        if (_lhs == zero) {
            if (zero < _rhs) {
                return zero;
            } else {
                throw std::runtime_error("^ domain error: base is 0 and exp is less than or equal to ​0​.");
            }
        } else if (_lhs < zero) {
            if (!(fmod(_rhs, one) == zero)) {
                throw std::runtime_error("^ domain error: base is negative and exp is not an integer value.");
            }
        }
        if (_rhs == zero) {
            return one;
        } else if (_rhs == one) {
            return std::move(_lhs);
        } else if (_rhs == -one) {
            return evaluate(G(one), ast::binary::div, std::move(_lhs));
        }
        return pow(std::move(_lhs), std::move(_rhs));
    }
    }
}

} // static namespace
}

}
#pragma clang diagnostic pop

ast::operand
evaluate(ast::unary const _operator,
         ast::operand && _operand)
{
    switch (_operator) {
    case ast::unary::plus : {
        return std::move(_operand);
    }
    case ast::unary::minus : {
        return visit([&] (auto & _ast) -> typename subexpression::result_type
        {
            return subexpression::unary::evaluate(std::move(_ast));
        }, *unref(std::move(_operand)));
    }
    }
}

ast::operand
evaluate(ast::operand && _lhs,
         ast::binary const _operator,
         ast::operand && _rhs)
{
    return multivisit([&] (auto & l, auto & r) -> typename subexpression::result_type
    {
        return subexpression::binary::evaluate(std::move(l), _operator, std::move(r));
    }, *unref(std::move(_lhs)), *unref(std::move(_rhs)));
}

}
}
