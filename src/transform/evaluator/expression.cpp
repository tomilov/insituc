#include <insituc/transform/evaluator/expression.hpp>

#include <insituc/transform/evaluator/subexpression.hpp>
#include <insituc/transform/evaluator/intrinsic.hpp>

#include <insituc/floating_point_type.hpp>
#include <insituc/shunting_yard_algorithm.hpp>
#include <insituc/ast/compare.hpp>

#include <versatile/visit.hpp>

#include <utility>
#include <stdexcept>

namespace insituc
{
namespace transform
{

namespace expression
{

using result_type = ast::operand;

namespace copy
{
namespace
{

[[noreturn]]
result_type
evaluate(ast::empty const & /*_empty*/)
{
    throw std::runtime_error("empty operand in expression is not allowed");
}

result_type
evaluate(G const & _value)
{
    return _value;
}

result_type
evaluate(ast::intrinsic_invocation const & _callee)
{
    ast::rvalues results_ = transform::evaluate(_callee.intrinsic_,
                                                transform::evaluate(_callee.argument_list_.rvalues_));
    assert(!results_.empty());
    if (results_.size() == 1) {
        return std::move(results_.back());
    }
    return ast::rvalue_list{std::move(results_)};
}

result_type
evaluate(ast::entry_substitution const & _callee)
{
    return ast::entry_substitution{_callee.entry_name_, {transform::evaluate(_callee.argument_list_.rvalues_)}};
}

result_type
evaluate(ast::constant const _constant)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_constant) { // exact constant values only
#pragma clang diagnostic pop
    case ast::constant::zero : {
        return zero;
    }
    case ast::constant::one : {
        return one;
    }
    default : {
        return _constant;
    }
    }
}

result_type
evaluate(ast::identifier const & _identifier)
{
    return _identifier;
}

result_type
evaluate(ast::unary_expression const & _unary_expression)
{
    return transform::evaluate(_unary_expression.operator_,
                               transform::evaluate(_unary_expression.operand_));
}

result_type
evaluate(ast::binary_expression const & _binary_expression)
{
    return transform::evaluate(transform::evaluate(_binary_expression.lhs_),
                               _binary_expression.operator_,
                               transform::evaluate(_binary_expression.rhs_));
}

struct tree_switch
{

    using result_type = typename expression::result_type;

    using rpn = shunting_yard_algorithm< tree_switch const, false >;
    using node_type = typename rpn::node_type;

    tree_switch()
        : rpn_(*this)
    { ; }

    result_type
    traverse(ast::expression const & _expression)
    {
        return rpn_.traverse(_expression);
    }

    result_type
    operator () (ast::operand const & _operand) const
    {
        return transform::evaluate(_operand);
    }

    result_type
    operator () (ast::operand const & _lhs,
                 ast::binary const _operator,
                 ast::operand const & _rhs) const
    {
        return transform::evaluate(transform::evaluate(_lhs),
                                   _operator,
                                   transform::evaluate(_rhs));
    }

    result_type
    operator () (node_type const & _lhs,
                 ast::binary const _operator,
                 node_type const & _rhs) const
    {
        return transform::evaluate(rpn_(_lhs),
                                   _operator,
                                   rpn_(_rhs));
    }

private :

    rpn rpn_;

};

result_type
evaluate(ast::expression const & _expression)
{
    if (_expression.rest_.empty()) {
        return transform::evaluate(_expression.first_);
    } else if (_expression.rest_.size() == 1) {
        ast::operation const & rhs_ = _expression.rest_.back();
        return transform::evaluate(transform::evaluate(_expression.first_),
                                   rhs_.operator_,
                                   transform::evaluate(rhs_.operand_));
    } else {
        tree_switch Dijkstra;
        return Dijkstra.traverse(_expression); // RPN
    }
}

result_type
evaluate(ast::rvalue_list const & _rvalue_list)
{
    ast::rvalues rvalues_ = transform::evaluate(_rvalue_list.rvalues_);
    if (rvalues_.size() == 1) {
        return std::move(rvalues_.back());
    }
    return ast::rvalue_list{std::move(rvalues_)};
}

result_type
evaluate(ast::operand_cptr const _operand_cptr)
{
    ast::operand const & source_ = unref(_operand_cptr);
    ast::operand destination_ = transform::evaluate(source_);
    if (destination_ == source_) {
        return ast::clone_or_ref(source_);
    }
    return destination_;
}

} // static namespace
}

namespace move
{
namespace
{

[[noreturn]]
result_type
evaluate(ast::empty && /*_empty*/)
{
    throw std::runtime_error("empty operand in expression is not allowed");
}

result_type
evaluate(G && _value)
{
    return std::move(_value);
}

result_type
evaluate(ast::intrinsic_invocation && _callee)
{
    ast::rvalues results_ = transform::evaluate(_callee.intrinsic_,
                                                transform::evaluate(std::move(_callee.argument_list_.rvalues_)));
    assert(!results_.empty());
    if (results_.size() == 1) {
        return std::move(results_.back());
    }
    return ast::rvalue_list{std::move(results_)};
}

result_type
evaluate(ast::entry_substitution && _callee)
{
    _callee.argument_list_.rvalues_ = transform::evaluate(std::move(_callee.argument_list_.rvalues_));
    return std::move(_callee);
}

result_type
evaluate(ast::constant const _constant)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_constant) { // exact constant values only
#pragma clang diagnostic pop
    case ast::constant::zero : {
        return zero;
    }
    case ast::constant::one : {
        return one;
    }
    default : {
        return _constant;
    }
    }
}

result_type
evaluate(ast::identifier && _identifier)
{
    return std::move(_identifier);
}

result_type
evaluate(ast::unary_expression && _unary_expression)
{
    return transform::evaluate(_unary_expression.operator_,
                               transform::evaluate(std::move(_unary_expression.operand_)));
}

result_type
evaluate(ast::binary_expression && _binary_expression)
{
    return transform::evaluate(transform::evaluate(std::move(_binary_expression.lhs_)),
                               _binary_expression.operator_,
                               transform::evaluate(std::move(_binary_expression.rhs_)));
}

struct tree_switch
{

    using result_type = typename expression::result_type;

    using rpn = shunting_yard_algorithm< tree_switch const, true >;
    using node_type = typename rpn::node_type;

    tree_switch()
        : rpn_(*this)
    { ; }

    result_type
    traverse(ast::expression && _expression)
    {
        return rpn_.traverse(std::move(_expression));
    }

    result_type
    operator () (ast::operand && _operand) const
    {
        return transform::evaluate(std::move(_operand));
    }

    result_type
    operator () (ast::operand && _lhs,
                 ast::binary const _operator,
                 ast::operand && _rhs) const
    {
        return transform::evaluate(transform::evaluate(std::move(_lhs)),
                                   _operator,
                                   transform::evaluate(std::move(_rhs)));
    }

    result_type
    operator () (node_type const & _lhs,
                 ast::binary const _operator,
                 node_type const & _rhs) const
    {
        return transform::evaluate(rpn_(_lhs),
                                   _operator,
                                   rpn_(_rhs));
    }

private :

    rpn rpn_;

};

result_type
evaluate(ast::expression && _expression)
{
    if (_expression.rest_.empty()) {
        return transform::evaluate(std::move(_expression.first_));
    } else if (_expression.rest_.size() == 1) {
        ast::operation & rhs_ = _expression.rest_.back();
        return transform::evaluate(transform::evaluate(std::move(_expression.first_)),
                                   rhs_.operator_,
                                   transform::evaluate(std::move(rhs_.operand_)));
    } else {
        tree_switch Dijkstra;
        return Dijkstra.traverse(std::move(_expression)); // RPN
    }
}

result_type
evaluate(ast::rvalue_list && _rvalue_list)
{
    ast::rvalues rvalues_ = transform::evaluate(std::move(_rvalue_list.rvalues_));
    if (rvalues_.size() == 1) {
        return std::move(rvalues_.back());
    }
    return ast::rvalue_list{std::move(rvalues_)};
}

result_type
evaluate(ast::operand_cptr _operand_cptr)
{
    ast::operand const & source_ = unref(_operand_cptr);
    ast::operand destination_ = transform::evaluate(source_);
    if (destination_ == source_) {
        return ast::clone_or_ref(source_);
    }
    return destination_;
}

} // static namespace
}

}

ast::operand
evaluate(ast::operand const & _operand)
{
    return visit([&] (auto const & o) -> typename expression::result_type
    {
        return expression::copy::evaluate(o);
    }, *_operand);
}

ast::operand
evaluate(ast::operand && _operand)
{
    return visit([&] (auto & o) -> typename expression::result_type
    {
        return expression::move::evaluate(std::move(o));
    }, *_operand);
}

ast::rvalues
evaluate(ast::rvalues const & _rvalues)
{
    ast::rvalues rvalues_;
    for (ast::rvalue const & folded_ : _rvalues) {
        append_rvalues(evaluate(folded_), rvalues_);
    }
    return rvalues_;
}

ast::rvalues
evaluate(ast::rvalues && _rvalues)
{
    ast::rvalues rvalues_;
    for (ast::rvalue & folded_ : _rvalues) {
        append_rvalues(evaluate(std::move(folded_)), rvalues_);
    }
    return rvalues_;
}

}
}
