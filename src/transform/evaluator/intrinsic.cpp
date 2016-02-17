#include <insituc/transform/evaluator/intrinsic.hpp>

#include <insituc/transform/evaluator/subexpression.hpp>

#include <insituc/floating_point_type.hpp>
#include <insituc/utility/append.hpp>

#include <utility>
#include <experimental/optional>

namespace insituc
{
namespace transform
{

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
namespace intrinsic
{

using result_type = ast::rvalues;

namespace
{

using U = ast::unary_expression;
using B = ast::binary_expression;
using I = ast::intrinsic_invocation;
using R = ast::rvalue_list;
using O = ast::operand;

result_type
intrinsic_chs(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< O, G > : { // chs(r) -> -r
            argument_ = -get< G && >(argument_);
            return std::move(_arguments);
        }
        case ast::index_at< O, U > : { // chs(-x) -> x
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            argument_ = std::move(unary_expression_.operand_);
            return std::move(_arguments);
        }
        case ast::index_at< O, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::chs : { // chs(chs(x)) -> x
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return std::move(intrinsic_invocation_.argument_list_.rvalues_);
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::chs, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_abs(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< O, G > : { // abs(r) = |r|
            argument_ = abs(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< O, U > : { // abs(-x) -> abs(x)
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            return intrinsic_abs(append< ast::rvalues >(std::move(unary_expression_.operand_)));
        }
        case ast::index_at< O, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::chs : { // abs(chs(x)) -> abs(x)
                return intrinsic_abs(std::move(intrinsic_invocation_.argument_list_.rvalues_));
            }
            case ast::intrinsic::abs : // abs(abs(x)) -> abs(x)
            case ast::intrinsic::sqrt : // abs(sqrt(x)) -> sqrt(x)
            case ast::intrinsic::exp : // abs(exp(x)) -> exp(x)
            case ast::intrinsic::arccos : // nonnegative definite
            case ast::intrinsic::pow2 : // abs(pow2(x)) -> pow2(x)
            case ast::intrinsic::sqr : // abs(sqr(x)) -> sqr(x)
            case ast::intrinsic::sumsqr : { // abs(sumsqr(x)) -> sumsqr(x)
                return std::move(_arguments);
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::abs, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_twice(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< O, G > : { // twice(r) -> r + r
            auto & value_ = get< G & >(argument_);
            value_ += value_;
            return std::move(_arguments);
        }
        case ast::index_at< O, U > : { // twice(-x) -> -twice(x)
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            argument_ = transform::evaluate(unary_expression_.operator_,
                                            R{intrinsic_twice(append< ast::rvalues >(std::move(unary_expression_.operand_)))});
            return std::move(_arguments);
        }
        case ast::index_at< O, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::abs : { // twice(abs(x)) -> abs(twice(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_abs(intrinsic_twice(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            case ast::intrinsic::chs : { // twice(chs(x)) -> chs(twice(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_chs(intrinsic_twice(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::twice, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_sumsqr(ast::rvalues && _arguments)
{
    if (0 < _arguments.size()) {
        G value_ = zero;
        ast::rvalues arguments_;
        for (ast::rvalue & argument_ : _arguments) {
            switch (argument_.which()) {
            case ast::index_at< ast::rvalue, G > : {
                auto & sqr_ = get< G & >(argument_);
                sqr_ *= sqr_;
                value_ += std::move(sqr_);
                continue;
            }
            case ast::index_at< ast::rvalue, U > : {
                auto & unary_expression_ = get< U & >(argument_);
                assert(unary_expression_.operator_ == ast::unary::minus);
                arguments_.push_back(std::move(unary_expression_.operand_));
                continue;
            }
            case ast::index_at< ast::rvalue, I > : {
                auto & intrinsic_invocation_ = get< I & >(argument_);
                ast::rvalues & rvalues_ = intrinsic_invocation_.argument_list_.rvalues_;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
                switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
                case ast::intrinsic::abs :
                case ast::intrinsic::chs : {
                    assert(rvalues_.size() == 1);
                    arguments_.push_back(std::move(rvalues_.back()));
                    continue;
                }
                default : {
                    break;
                }
                } // ast::intrinsic::sqrt case leads to much more entangled code
                break;
            }
            default : {
                break;
            }
            }
            arguments_.push_back(std::move(argument_));
        }
        _arguments.resize(1);
        ast::rvalue & argument_ = _arguments.back();
        if (arguments_.empty()) {
            argument_ = std::move(value_);
        } else if (value_ == zero) {
            argument_ = I{ast::intrinsic::sumsqr, R{std::move(arguments_)}};
        } else if (value_ < zero) {
            argument_ = B{I{ast::intrinsic::sumsqr, R{std::move(arguments_)}}, ast::binary::sub, -std::move(value_)};
        } else {
            argument_ = B{I{ast::intrinsic::sumsqr, R{std::move(arguments_)}}, ast::binary::add,  std::move(value_)};
        }
    } else {
        _arguments.emplace_back(I{ast::intrinsic::sumsqr, {std::move(_arguments)}});
    }
    return std::move(_arguments);
}

result_type
intrinsic_sqrt(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // sqrt(r) = square root of r
            argument_ = sqrt(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::sqr : { // sqrt(sqr(x)) -> abs(x)
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_abs(std::move(intrinsic_invocation_.argument_list_.rvalues_));
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::sqrt, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_round(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // round(r) = nearest to r integer
            argument_ = nearbyint(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::chs : { // round(chs(x)) -> chs(round(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_chs(intrinsic_round(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            case ast::intrinsic::abs : { // round(abs(x)) -> abs(round(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_abs(intrinsic_round(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            case ast::intrinsic::round : // round(round(x)) -> round(x)
            case ast::intrinsic::trunc : { // round(trunc(x)) -> trunc(x)
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return std::move(_arguments);
            }
            case ast::intrinsic::frac : { // round(frac(x)) -> 0
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                argument_ = zero;
                return std::move(_arguments);
            }
            default : {
                break;
            }
            }
            break;
        }
        case ast::index_at< ast::rvalue, ast::constant > : {
            switch (get< ast::constant const >(argument_)) {
            case ast::constant::zero : {
                argument_ = zero;
                break;
            }
            case ast::constant::one : {
                argument_ = one;
                break;
            }
            case ast::constant::pi : { // 3,1415926535897932384626433832795...
                argument_ = G(3);
                break;
            }
            case ast::constant::l2e : { // 1,4426950408889634073599246810019...
                argument_ = one;
                break;
            }
            case ast::constant::l2t : { // 3,3219280948873623478703194294894...
                argument_ = G(3);
                break;
            }
            case ast::constant::lg2 : { // 0,30102999566398119521373889472449...
                argument_ = zero;
                break;
            }
            case ast::constant::ln2 : { // 0,69314718055994530941723212145818...
                argument_ = one;
                break;
            }
            }
            return std::move(_arguments);
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::round, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_trunc(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // trunc(r) = integral part of r
            argument_ = trunc(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::chs : { // trunc(chs(x)) -> chs(trunc(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_chs(intrinsic_trunc(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            case ast::intrinsic::abs : { // trunc(abs(x)) -> abs(trunc(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_abs(intrinsic_trunc(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            case ast::intrinsic::trunc : // trunc(trunc(x)) -> trunc(x)
            case ast::intrinsic::round : { // trunc(round(x)) -> round(x)
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return std::move(_arguments);
            }
            case ast::intrinsic::frac : { // trunc(frac(x)) -> 0
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                argument_ = zero;
                return std::move(_arguments);
            }
            default : {
                break;
            }
            }
            break;
        }
        case ast::index_at< ast::rvalue, ast::constant > : {
            switch (get< ast::constant const >(argument_)) {
            case ast::constant::zero : {
                argument_ = zero; // unrecheable
                break;
            }
            case ast::constant::one : {
                argument_ = one; // unrecheable
                break;
            }
            case ast::constant::pi : { // 3,1415926535897932384626433832795...
                argument_ = G(3);
                break;
            }
            case ast::constant::l2e : { // 1,4426950408889634073599246810019...
                argument_ = one;
                break;
            }
            case ast::constant::l2t : { // 3,3219280948873623478703194294894...
                argument_ = G(3);
                break;
            }
            case ast::constant::lg2 : { // 0,30102999566398119521373889472449...
                argument_ = zero;
                break;
            }
            case ast::constant::ln2 : { // 0,69314718055994530941723212145818...
                argument_ = zero;
                break;
            }
            }
            return std::move(_arguments);
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::trunc, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_remainder(ast::rvalues && _arguments)
{
    if (_arguments.size() == 2) {
        O & dividend_ = _arguments.front();
        O & divisor_ = _arguments.back();
        if (dividend_.active< G >() && divisor_.active< G >()) {
            dividend_ = remainder(get< G && >(dividend_), get< G && >(divisor_));
            _arguments.pop_back();
            return std::move(_arguments);
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::remainder, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_cos(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // cos(r) = cosine of r
            argument_ = cos(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, U > : { // cos(-x) = cos(x)
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            return intrinsic_cos(append< ast::rvalues >(std::move(unary_expression_.operand_)));
        }
        case ast::index_at< ast::rvalue, ast::constant > : { // cos(-x) = cos(x)
            auto const constant_ = get< ast::constant const >(argument_);
            if (constant_ == ast::constant::zero) {
                argument_ = one;
                return std::move(_arguments);
            } else if (constant_ == ast::constant::pi) {
                argument_ = zero;
                return std::move(_arguments);
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::cos, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_sin(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // sin(r) = sine of r
            argument_ = sin(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, U > : { // sin(-x) = sin(x)
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            argument_ = transform::evaluate(unary_expression_.operator_,
                                            R{intrinsic_sin(append< ast::rvalues >(std::move(unary_expression_.operand_)))});
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, ast::constant > : { // sin(-x) = sin(x)
            auto const constant_ = get< ast::constant const >(argument_);
            if (constant_ == ast::constant::zero) {
                argument_ = zero;
                return std::move(_arguments);
            } else if (constant_ == ast::constant::pi) {
                argument_ = zero;
                return std::move(_arguments);
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::sin, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_sincos(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // sincos(r) = sine of r, cosine of r
            _arguments.emplace_back(cos(get< G & >(argument_)));
            argument_ = sin(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, ast::constant > : { // sin(-x) = sin(x)
            auto const constant_ = get< ast::constant const >(argument_);
            if (constant_ == ast::constant::zero) {
                argument_ = zero;
                _arguments.emplace_back(one);
                return std::move(_arguments);
            } else if (constant_ == ast::constant::pi) {
                argument_ = zero;
                _arguments.emplace_back(-one);
                return std::move(_arguments);
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::sincos, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_tg(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // tg(r) = tangent of r
            argument_ = tan(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, U > : { // sin(-x) = sin(x)
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            argument_ = transform::evaluate(unary_expression_.operator_,
                                            R{intrinsic_tg(append< ast::rvalues >(std::move(unary_expression_.operand_)))});
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, ast::constant > : {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (get< ast::constant const >(argument_)) {
#pragma clang diagnostic pop
            case ast::constant::zero :
            case ast::constant::pi : {
                argument_ = zero;
                return std::move(_arguments);
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::tg, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_ctg(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // ctg(r) = cotangent of r
            argument_ = one / tan(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, U > : { // sin(-x) = sin(x)
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            argument_ = transform::evaluate(unary_expression_.operator_,
                                            R{intrinsic_ctg(append< ast::rvalues >(std::move(unary_expression_.operand_)))});
            return std::move(_arguments);
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::ctg, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_arctg(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.front();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // arctg(r) = arcrangent of r
            argument_ = atan2(get< G && >(argument_), one);
            return std::move(_arguments);
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::arctg, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_atan2(ast::rvalues && _arguments)
{
    if (_arguments.size() == 2) {
        O & x_ = _arguments.front();
        O & y_ = _arguments.back();
        if (x_.active< G >() && y_.active< G >()) {
            x_ = atan2(get< G && >(x_), get< G && >(y_));
            _arguments.pop_back();
            return std::move(_arguments);
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::atan2, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_poly(ast::rvalues && _arguments)
{
    result_type results_;
    if (_arguments.empty()) {
        // poly is at least unary function
    } else {
        O head_ = std::move(_arguments.front());
        _arguments.pop_front();
        if (_arguments.empty()) {
            results_.emplace_back(zero);
        } else {
            size_type const size_ = _arguments.size();
            if (size_ == 1) {
                results_.push_back(std::move(_arguments.front()));
            } else if (size_ == 2) {
                results_.push_back(transform::evaluate(std::move(_arguments.front()),
                                                       ast::binary::add,
                                                       transform::evaluate(std::move(_arguments.back()),
                                                                           ast::binary::mul,
                                                                           std::move(head_))));
            } else if (head_.active< G >()) {
                G x_ = get< G && >(head_);
                if (x_ == zero) {
                    results_.push_back(std::move(_arguments.front())); // car
                } else if (x_ == one) {
                    G value_ = zero;
                    std::experimental::optional< O > result_;
                    for (ast::rvalue & argument_ : _arguments) {
                        if (argument_.active< G >()) {
                            value_ += get< G && >(argument_);
                        } else {
                            if (!result_) {
                                result_ = std::move(argument_);
                            } else {
                                *result_ = transform::evaluate(std::move(*result_),
                                                               ast::binary::add,
                                                               std::move(argument_));
                            }
                        }
                    }
                    if (!result_) {
                        results_.emplace_back(std::move(value_));
                    } else {
                        if (value_ < zero) {
                            results_.push_back(transform::evaluate(std::move(*result_),
                                                                   ast::binary::sub,
                                                                   O(-std::move(value_))));
                        } else if (zero < value_) {
                            results_.push_back(transform::evaluate(std::move(*result_),
                                                                   ast::binary::add,
                                                                   O(std::move(value_))));
                        } else {
                            results_.push_back(std::move(*result_));
                        }
                    }
                } else if (x_ == -one) {
                    G value_ = zero;
                    std::experimental::optional< O > lhs_;
                    std::experimental::optional< O > rhs_;
                    bool sign_ = false;
                    for (ast::rvalue & argument_ : _arguments) {
                        bool const is_value_ = argument_.active< G >();
                        if (sign_) {
                            if (is_value_) {
                                value_ -= get< G && >(argument_);
                            } else {
                                if (!rhs_) {
                                    rhs_ = std::move(argument_);
                                } else {
                                    *rhs_ = transform::evaluate(std::move(*rhs_),
                                                                ast::binary::add,
                                                                std::move(argument_));
                                }
                            }
                        } else {
                            if (is_value_) {
                                value_ += get< G && >(argument_);
                            } else {
                                if (!lhs_) {
                                    lhs_ = std::move(argument_);
                                } else {
                                    *lhs_ = transform::evaluate(std::move(*lhs_),
                                                                ast::binary::add,
                                                                std::move(argument_));
                                }
                            }
                        }
                        sign_ = !sign_;
                    }
                    if (!lhs_) {
                        if (!rhs_) {
                            results_.emplace_back(std::move(value_));
                        } else {
                            if (value_ == zero) {
                                results_.push_back(transform::evaluate(ast::unary::minus, std::move(*rhs_)));
                            } else {
                                results_.push_back(transform::evaluate(O(std::move(value_)),
                                                                       ast::binary::sub,
                                                                       std::move(*rhs_)));
                            }
                        }
                    } else {
                        if (!rhs_) {
                            if (value_ < zero) {
                                results_.push_back(transform::evaluate(std::move(*lhs_),
                                                                       ast::binary::sub,
                                                                       O(-std::move(value_))));
                            } else if (zero < value_) {
                                results_.push_back(transform::evaluate(std::move(*lhs_),
                                                                       ast::binary::add,
                                                                       O(std::move(value_))));
                            } else {
                                results_.push_back(std::move(*lhs_));
                            }
                        } else {
                            if (value_ < zero) {
                                *rhs_ = transform::evaluate(std::move(*rhs_),
                                                            ast::binary::add,
                                                            O(-std::move(value_)));
                            } else if (zero < value_) {
                                *lhs_ = transform::evaluate(std::move(*lhs_),
                                                            ast::binary::add,
                                                            O(std::move(value_)));
                            }
                            results_.push_back(transform::evaluate(std::move(*lhs_),
                                                                   ast::binary::sub,
                                                                   std::move(*rhs_)));
                        }
                    }
                } else {
                    G value_ = zero;
                    G monomial_ = one;
                    size_type zeros_ = 0;
                    for (ast::rvalue & argument_ : _arguments) {
                        if (argument_.active< G >()) {
                            auto & a_ = get< G & >(argument_);
                            a_ *= monomial_;
                            value_ += std::move(a_);
                            a_ = zero;
                            ++zeros_;
                        }
                        monomial_ *= x_;
                    }
                    assert(2 < size_);
                    if (zeros_ == size_) {
                        results_.emplace_back(std::move(value_));
                    } else {
                        _arguments.front() = transform::evaluate(std::move(_arguments.front()),
                                                                 ast::binary::add,
                                                                 O(std::move(value_)));
                        _arguments.emplace_front(std::move(x_)); // prepend x back to the front
                        results_.emplace_back(I{ast::intrinsic::poly, {std::move(_arguments)}});
                    }
                }
            } else {
                _arguments.push_front(std::move(head_));
                results_.emplace_back(I{ast::intrinsic::poly, {std::move(_arguments)}});
            }
        }
    }
    return results_;
}

result_type
intrinsic_frac(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.front();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // frac(r) = fractional part of r
            auto & frac_ = get< G & >(argument_);
            frac_ -= nearbyint(frac_);
            return std::move(_arguments);
        } // ast::constant::zero and ast::constant::one are exact values, but unrecheable
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::frac, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_intrem(ast::rvalues && _arguments)
{
    if (_arguments.size() == 2) {
        O & dividend_ = _arguments.front();
        O & divisor_ = _arguments.back();
        if (dividend_.active< G >() && divisor_.active< G >()) {
            auto & integral_ = get< G & >(dividend_);
            auto & remainder_ = get< G & >(divisor_);
            remainder_ = fmod(integral_, std::move(remainder_));
            integral_ -= remainder_;
            return std::move(_arguments);
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::intrem, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_fracint(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.front();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // fracint(r) = fractional part of r, integral part of r
            auto & fractional_ = get< G & >(argument_);
            _arguments.emplace_back(trunc(fractional_));
            if (isinf(fractional_)) {
                fractional_ = zero;
            } else {
                fractional_ -= get< G & >(_arguments.back());
            }
            return std::move(_arguments);
        } // ast::constant::zero and ast::constant::one are exact, but unrecheable
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::fracint, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_pow(ast::rvalues && _arguments)
{
    if (_arguments.size() == 2) {
        O & base_ = _arguments.front();
        O & power_ = _arguments.back();
        if (base_.active< G >() && power_.active< G >()) {
            base_ = pow(get< G && >(base_), get< G && >(power_));
            _arguments.pop_back();
            return std::move(_arguments);
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::pow, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_exp(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // exp(r) = exponent of r
            argument_ = exp(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, ast::constant > : {
            auto const constant_ = get< ast::constant const >(argument_);
            if (constant_ == ast::constant::zero) {
                argument_ = one;
                return std::move(_arguments);
            } else if (constant_ == ast::constant::ln2) {
                argument_ = G(2);
                return std::move(_arguments);
            }
            break;
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::ln : {
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return std::move(intrinsic_invocation_.argument_list_.rvalues_);
            }
            case ast::intrinsic::log2 : {
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_pow(append< ast::rvalues >(std::move(intrinsic_invocation_.argument_list_.rvalues_.back()), ast::constant::l2e));
            }
            case ast::intrinsic::lg : {
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_pow(append< ast::rvalues >(std::move(intrinsic_invocation_.argument_list_.rvalues_.back()), B{ast::constant::l2e, ast::binary::mul, ast::constant::lg2}));
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::exp, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_pow2(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // pow2(r) = 2 raised to r power
            argument_ = exp2(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, ast::constant > : {
            auto const constant_ = get< ast::constant const >(argument_);
            if (constant_ == ast::constant::zero) {
                argument_ = one;
                return std::move(_arguments);
            } else if (constant_ == ast::constant::l2t) {
                argument_ = G(10);
                return std::move(_arguments);
            }
            break;
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::ln : {
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_pow(append< ast::rvalues >(std::move(intrinsic_invocation_.argument_list_.rvalues_.back()), ast::constant::ln2));
            }
            case ast::intrinsic::log2 : {
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return std::move(intrinsic_invocation_.argument_list_.rvalues_);
            }
            case ast::intrinsic::lg : {
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_pow(append< ast::rvalues >(std::move(intrinsic_invocation_.argument_list_.rvalues_.back()), ast::constant::lg2));
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::pow2, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_sqr(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // sqr(r) = r * r
            auto & sqr_ = get< G & >(argument_);
            sqr_ *= sqr_;
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, U > : { // sqr(-x) -> sqr(x)
            auto & unary_expression_ = get< U & >(argument_);
            assert(unary_expression_.operator_ == ast::unary::minus);
            return intrinsic_sqr(append< ast::rvalues >(std::move(unary_expression_.operand_)));
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::abs : // sqr(abs(x)) -> sqr(x)
            case ast::intrinsic::chs : { // sqr(chs(x)) -> sqr(x)
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_sqr(std::move(intrinsic_invocation_.argument_list_.rvalues_));
            }
            case ast::intrinsic::exp : { // sqr(exp(x)) -> exp(twice(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_exp(intrinsic_twice(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            case ast::intrinsic::pow2 : { // sqr(pow2(x)) -> pow2(tiwce(x))
                assert(intrinsic_invocation_.argument_list_.rvalues_.size() == 1);
                return intrinsic_pow2(intrinsic_twice(std::move(intrinsic_invocation_.argument_list_.rvalues_)));
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::sqr, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_log(ast::rvalues && _arguments)
{
    if (_arguments.size() == 2) {
        O & base_ = _arguments.front();
        O & power_ = _arguments.back();
        if (base_.active< G >() && power_.active< G >()) {
            base_ = log(get< G && >(power_)) / log(get< G && >(base_)); // ln(argument) / ln(base)
            _arguments.pop_back();
            return std::move(_arguments);
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::log, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_ln(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // ln(r) = log base e of r
            argument_ = log(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
            size_type const arity_ = intrinsic_invocation_.argument_list_.rvalues_.size();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::pow : {
                if (arity_ != 2) {
                    break;
                }
                result_type ln_ = intrinsic_ln(append< ast::rvalues >(std::move(intrinsic_invocation_.argument_list_.rvalues_.front())));
                if (ln_.size() != 1) {
                    break;
                }
                argument_ = transform::evaluate(std::move(intrinsic_invocation_.argument_list_.rvalues_.back()),
                                                ast::binary::mul,
                                                std::move(ln_.back()));
                return std::move(_arguments);
            }
            case ast::intrinsic::exp : {
                if (arity_ != 1) {
                    break;
                }
                return std::move(intrinsic_invocation_.argument_list_.rvalues_);
            }
            case ast::intrinsic::pow2 : {
                if (arity_ != 1) {
                    break;
                }
                argument_ = transform::evaluate(ast::constant::ln2,
                                                ast::binary::mul,
                                                std::move(intrinsic_invocation_.argument_list_.rvalues_.back()));
                if (argument_.empty()) {
                    break;
                }
                return std::move(_arguments);
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::ln, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_log2(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // log2(r) = log base 2 of r
            argument_ = log2(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
            size_type const arity_ = intrinsic_invocation_.argument_list_.rvalues_.size();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::pow : {
                if (arity_ != 1) {
                    break;
                }
                result_type log2_ = intrinsic_log2(append< ast::rvalues >(std::move(intrinsic_invocation_.argument_list_.rvalues_.front())));
                if (log2_.empty()) {
                    break;
                }
                argument_ = transform::evaluate(std::move(intrinsic_invocation_.argument_list_.rvalues_.back()),
                                                ast::binary::mul,
                                                std::move(log2_.back()));
                if (argument_.empty()) {
                    break;
                }
                return std::move(_arguments);
            }
            case ast::intrinsic::exp : {
                if (arity_ != 1) {
                    break;
                }
                argument_ = transform::evaluate(ast::constant::l2e,
                                                ast::binary::mul,
                                                std::move(intrinsic_invocation_.argument_list_.rvalues_.back()));
                if (argument_.empty()) {
                    break;
                }
                return std::move(_arguments);
            }
            case ast::intrinsic::pow2 : {
                if (arity_ != 1) {
                    break;
                }
                return std::move(intrinsic_invocation_.argument_list_.rvalues_);
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::log2, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_lg(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // lg(r) = log base 10 of r
            argument_ = log10(get< G && >(argument_));
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, I > : {
            auto & intrinsic_invocation_ = get< I & >(argument_);
            size_type const arity_ = intrinsic_invocation_.argument_list_.rvalues_.size();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (intrinsic_invocation_.intrinsic_) {
#pragma clang diagnostic pop
            case ast::intrinsic::pow : {
                if (arity_ != 1) {
                    break;
                }
                result_type lg_ = intrinsic_lg(append< ast::rvalues >(std::move(intrinsic_invocation_.argument_list_.rvalues_.front())));
                if (lg_.empty()) {
                    break;
                }
                ast::rvalue result_ = transform::evaluate(std::move(intrinsic_invocation_.argument_list_.rvalues_.back()),
                                                          ast::binary::mul,
                                                          std::move(lg_.back()));
                if (result_.empty()) {
                    break;
                }
                _arguments.push_back(std::move(result_));
                return std::move(_arguments);
            }
            case ast::intrinsic::exp : {
                if (arity_ != 1) {
                    break;
                }
                ast::rvalue result_ = transform::evaluate(B{ast::constant::l2e, ast::binary::mul, ast::constant::lg2},
                                                          ast::binary::mul,
                                                          std::move(intrinsic_invocation_.argument_list_.rvalues_.back()));
                if (result_.empty()) {
                    break;
                }
                _arguments.push_back(std::move(result_));
                return std::move(_arguments);
            }
            case ast::intrinsic::pow2 : {
                if (arity_ != 1) {
                    break;
                }
                ast::rvalue result_ = transform::evaluate(ast::constant::lg2,
                                                          ast::binary::mul,
                                                          std::move(intrinsic_invocation_.argument_list_.rvalues_.back()));
                if (result_.empty()) {
                    break;
                }
                _arguments.push_back(std::move(result_));
                return std::move(_arguments);
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::lg, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_yl2xp1(ast::rvalues && _arguments)
{
    if (_arguments.size() == 2) {
        O & x_ = _arguments.front();
        O & y_ = _arguments.back();
        if (x_.active< G >() && y_.active< G >()) {
            x_ = get< G && >(y_) * log1p(get< G && >(x_)) / log(one + one); // y * log1p(x) / ln2
            _arguments.pop_back();
            return std::move(_arguments);
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::yl2xp1, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_scale2(ast::rvalues && _arguments)
{
    if (_arguments.size() == 2) {
        O & power_ = _arguments.front();
        O & scaler_ = _arguments.back();
        if (power_.active< G >() && scaler_.active< G >()) {
            power_ = get< G && >(scaler_) * exp2(trunc(get< G && >(power_))); // scaler * pow2(trunc(power))
            _arguments.pop_back();
            return std::move(_arguments);
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::scale2, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_extract(ast::rvalues && _arguments)
{
    result_type results_;
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // extract(r) = base 2 exponent and significand
            auto & value_ = get< G & >(argument_);
            _arguments.emplace_front(logb(abs(value_))); // exponent
            value_ = std::move(value_) / exp2(get< G & >(_arguments.front()));
            return std::move(_arguments);
        } // ast::constant::zero and ast::constant::one are exact values, but unrecheable
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::extract, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_pow2m1(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // pow2m1(r) = 2^r - 1
            argument_ = exp2(get< G && >(argument_)) - one;
            return std::move(_arguments);
        }
        case ast::index_at< ast::rvalue, ast::constant > : {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (get< ast::constant const >(argument_)) {
#pragma clang diagnostic pop
            case ast::constant::l2t : {
                argument_ = G(9);
                return std::move(_arguments);
            }
            default : {
                break;
            }
            }
            break;
        }
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::pow2m1, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_arcsin(ast::rvalues && _arguments)
{
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // arcsin(r) = arcsine of r
            argument_ = asin(get< G && >(argument_));
            return std::move(_arguments);
        } // ast::constant::zero and ast::constant::one are exact values, but unrecheable here
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::arcsin, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_arccos(ast::rvalues && _arguments)
{
    result_type results_;
    if (_arguments.size() == 1) {
        O & argument_ = _arguments.back();
        switch (argument_.which()) {
        case ast::index_at< ast::rvalue, G > : { // arccos(r) = arccosine of r
            argument_ = acos(get< G && >(argument_));
            return std::move(_arguments);
        } // ast::constant::zero and ast::constant::one are exact values, but unrecheable here
        default : {
            break;
        }
        }
    }
    _arguments.emplace_back(I{ast::intrinsic::arccos, {std::move(_arguments)}});
    return std::move(_arguments);
}

result_type
intrinsic_max(ast::rvalues && _arguments)
{
    result_type results_;
    if (0 < _arguments.size()) {
        ast::rvalues arguments_;
        O max_value_;
        for (ast::rvalue & argument_ : _arguments) {
            if (argument_.active< G >()) {
                auto & value_ = get< G & >(argument_);
                if (max_value_.empty() || (get< G const & >(max_value_) < value_)) {
                    max_value_ = std::move(argument_);
                }
            } else {
                arguments_.push_back(std::move(argument_));
            }
        }
        if (max_value_.empty()) {
            results_.emplace_back(I{ast::intrinsic::max, R{std::move(arguments_)}});
        } else {
            if (arguments_.empty()) {
                results_.push_back(std::move(max_value_));
            } else {
                arguments_.push_back(std::move(max_value_));
                results_.emplace_back(I{ast::intrinsic::max, R{std::move(arguments_)}});
            }
        }
    }
    return results_;
}

result_type
intrinsic_min(ast::rvalues && _arguments)
{
    result_type results_;
    if (0 < _arguments.size()) {
        ast::rvalues arguments_;
        O min_value_;
        for (ast::rvalue & argument_ : _arguments) {
            if (argument_.active< G >()) {
                auto & value_ = get< G & >(argument_);
                if (min_value_.empty() || (value_ < get< G const & >(min_value_))) {
                    min_value_ = std::move(argument_);
                }
            } else {
                arguments_.push_back(std::move(argument_));
            }
        }
        if (min_value_.empty()) {
            results_.emplace_back(I{ast::intrinsic::min, R{std::move(arguments_)}});
        } else {
            if (arguments_.empty()) {
                results_.push_back(std::move(min_value_));
            } else {
                arguments_.push_back(std::move(min_value_));
                results_.emplace_back(I{ast::intrinsic::min, R{std::move(arguments_)}});
            }
        }
    }
    return results_;
}

result_type
evaluate(ast::intrinsic const _intrinsic, ast::rvalues && _arguments)
{
    switch (_intrinsic) {
    case ast::intrinsic::chs       : return intrinsic_chs      (std::move(_arguments));
    case ast::intrinsic::abs       : return intrinsic_abs      (std::move(_arguments));
    case ast::intrinsic::twice     : return intrinsic_twice    (std::move(_arguments));
    case ast::intrinsic::sumsqr    : return intrinsic_sumsqr   (std::move(_arguments));
    case ast::intrinsic::sqrt      : return intrinsic_sqrt     (std::move(_arguments));
    case ast::intrinsic::round     : return intrinsic_round    (std::move(_arguments));
    case ast::intrinsic::trunc     : return intrinsic_trunc    (std::move(_arguments));
    case ast::intrinsic::remainder : return intrinsic_remainder(std::move(_arguments));
    case ast::intrinsic::cos       : return intrinsic_cos      (std::move(_arguments));
    case ast::intrinsic::sin       : return intrinsic_sin      (std::move(_arguments));
    case ast::intrinsic::sincos    : return intrinsic_sincos   (std::move(_arguments));
    case ast::intrinsic::tg        : return intrinsic_tg       (std::move(_arguments));
    case ast::intrinsic::ctg       : return intrinsic_ctg      (std::move(_arguments));
    case ast::intrinsic::arctg     : return intrinsic_arctg    (std::move(_arguments));
    case ast::intrinsic::atan2     : return intrinsic_atan2    (std::move(_arguments));
    case ast::intrinsic::poly      : return intrinsic_poly     (std::move(_arguments));
    case ast::intrinsic::frac      : return intrinsic_frac     (std::move(_arguments));
    case ast::intrinsic::intrem    : return intrinsic_intrem   (std::move(_arguments));
    case ast::intrinsic::fracint   : return intrinsic_fracint  (std::move(_arguments));
    case ast::intrinsic::pow       : return intrinsic_pow      (std::move(_arguments));
    case ast::intrinsic::exp       : return intrinsic_exp      (std::move(_arguments));
    case ast::intrinsic::pow2      : return intrinsic_pow2     (std::move(_arguments));
    case ast::intrinsic::sqr       : return intrinsic_sqr      (std::move(_arguments));
    case ast::intrinsic::log       : return intrinsic_log      (std::move(_arguments));
    case ast::intrinsic::ln        : return intrinsic_ln       (std::move(_arguments));
    case ast::intrinsic::log2      : return intrinsic_log2     (std::move(_arguments));
    case ast::intrinsic::lg        : return intrinsic_lg       (std::move(_arguments));
    case ast::intrinsic::yl2xp1    : return intrinsic_yl2xp1   (std::move(_arguments));
    case ast::intrinsic::scale2    : return intrinsic_scale2   (std::move(_arguments));
    case ast::intrinsic::extract   : return intrinsic_extract  (std::move(_arguments));
    case ast::intrinsic::pow2m1    : return intrinsic_pow2m1   (std::move(_arguments));
    case ast::intrinsic::arcsin    : return intrinsic_arcsin   (std::move(_arguments));
    case ast::intrinsic::arccos    : return intrinsic_arccos   (std::move(_arguments));
    case ast::intrinsic::max       : return intrinsic_max      (std::move(_arguments));
    case ast::intrinsic::min       : return intrinsic_min      (std::move(_arguments));
    }
}

} // static namespace

}
#pragma clang diagnostic pop

ast::rvalues
evaluate(ast::intrinsic const _intrinsic,
         ast::rvalues && _arguments)
{
    for (ast::rvalue & argument_ : _arguments) {
        argument_ = unref(std::move(argument_)); // a necessary evil
    }
    assert((_arguments.size() != 1) || !_arguments.back().active< ast::rvalue_list >());
    typename intrinsic::result_type results_ = intrinsic::evaluate(_intrinsic, std::move(_arguments));
    if (results_.empty()) {
        // TODO: process error here
        results_.emplace_back(ast::intrinsic_invocation{_intrinsic, ast::rvalue_list{std::move(_arguments)}});
    }
    return results_;
}

}
}
