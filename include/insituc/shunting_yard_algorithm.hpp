#pragma once

#include <insituc/base_types.hpp>
#include <insituc/ast/ast.hpp>

#include <insituc/variant.hpp>

#include <type_traits>
#include <utility>
#include <functional>
#include <deque>
#include <stack>

#include <cassert>

namespace insituc
{

// for (reassemble == false) `ast::operand const &&` binds to `ast::operand const &` everywhere
template< typename dispatcher, bool reassmble >
class shunting_yard_algorithm
{

    static_assert(!std::is_reference_v< dispatcher >, "non-reference type expected");

    dispatcher & dispatcher_;

public :

    using result_type = typename dispatcher::result_type;

    shunting_yard_algorithm(dispatcher & _dispatcher)
        : dispatcher_(_dispatcher)
    { ; }

    using operand_ptr = std::conditional_t< reassmble, ast::operand, ast::operand const > *;

    struct lhs_op_rhs
    {

        size_type   const lhs_;
        ast::binary const operator_;
        size_type   const rhs_;

    };

    using node_type = versatile< aggregate_wrapper< lhs_op_rhs >, operand_ptr >;

private :

    std::deque< node_type > output_;

    struct lhs_op
    {

        size_type   const lhs_;
        ast::binary const operator_;

    };

public :

    result_type
    operator () (operand_ptr const _top) const
    {
        return dispatcher_(std::move(*_top));
    }

    result_type
    operator () (lhs_op_rhs const & _top) const
    {
        return dispatcher_(output_.at(_top.lhs_),
                           _top.operator_,
                           output_.at(_top.rhs_));
    }

    result_type
    operator () (node_type const & _node) const
    {
        return visit(*this, _node);
    }

    result_type
    traverse(std::conditional_t< reassmble, ast::expression &&, ast::expression const & > _input)
    { // if reassmble is true, then input is taken apart, then reassembled
        assert(output_.empty());
        if (_input.rest_.empty()) {
            return dispatcher_(std::move(_input.first_));
        } else if (_input.rest_.size() == 1) {
            auto & operation_ = _input.rest_.back();
            return dispatcher_(std::move(_input.first_),
                               operation_.operator_,
                               std::move(operation_.operand_));
        } else {
            //output_.reserve(_input.rest_.size() * 2 + 1); // total number of operators and operands
            std::stack< aggregate_wrapper< lhs_op > > lhs_op_;
            for (auto & operation_ : _input.rest_) {
                size_type const precedence_ = ast::precedence(operation_.operator_);
                while (!lhs_op_.empty()) {
                    lhs_op const & top_ = lhs_op_.top();
                    if (ast::precedence(top_.operator_) < precedence_) {
                        break;
                    }
                    output_.emplace_back(top_.lhs_, top_.operator_, output_.size());
                    lhs_op_.pop();
                }
                lhs_op_.emplace(output_.size(), operation_.operator_);
                output_.emplace_back(&operation_.operand_);
            }
            while (!lhs_op_.empty()) {
                lhs_op const & top_ = lhs_op_.top();
                output_.emplace_back(top_.lhs_, top_.operator_, output_.size());
                lhs_op_.pop();
            }
            output_.emplace_front(&_input.first_);
            return operator () (output_.back());
        }
    }

};

}
