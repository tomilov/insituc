#pragma once

#include <insituc/ast/ast.hpp>

#include <algorithm>
#include <iterator>

namespace insituc
{
namespace ast
{

constexpr
bool
operator == (empty const & /*_lhs*/, empty const & /*_rhs*/) noexcept
{
    return true;
}

inline
bool
operator == (lvalue_list const & _lhs, lvalue_list const & _rhs)
{
    return (_lhs.lvalues_ == _rhs.lvalues_);
}

inline
bool
operator == (operand const & _lhs, operand const & _rhs);

inline
bool
operator == (rvalue_list const & _lhs, rvalue_list const & _rhs)
{
    return (_lhs.rvalues_ == _rhs.rvalues_);
}

inline
bool
operator == (unary_expression const & _lhs, unary_expression const & _rhs)
{
    if (!(_lhs.operator_ == _rhs.operator_)) {
        return false;
    }
    if (!(_lhs.operand_ == _rhs.operand_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (operation const & _lhs, operation const & _rhs)
{
    if (!(_lhs.operator_ == _rhs.operator_)) {
        return false;
    }
    if (!(_lhs.operand_ == _rhs.operand_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (entry_substitution const & _lhs, entry_substitution const & _rhs)
{
    if (!(_lhs.entry_name_ == _rhs.entry_name_)) {
        return false;
    }
    if (!(_lhs.argument_list_ == _rhs.argument_list_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (intrinsic_invocation const & _lhs, intrinsic_invocation const & _rhs)
{
    if (!(_lhs.intrinsic_ == _rhs.intrinsic_)) {
        return false;
    }
    if (!(_lhs.argument_list_ == _rhs.argument_list_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (binary_expression const & _lhs, binary_expression const & _rhs)
{
    if (!(_lhs.lhs_ == _rhs.lhs_)) {
        return false;
    }
    if (!(_lhs.operator_ == _rhs.operator_)) {
        return false;
    }
    if (!(_lhs.rhs_ == _rhs.rhs_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (expression const & _lhs, expression const & _rhs)
{
    if (!(_lhs.first_ == _rhs.first_)) {
        return false;
    }
    if (!(_lhs.rest_ == _rhs.rest_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (operand const & _lhs, operand const & _rhs)
{
    operand const & lhs_ = unref(_lhs);
    operand const & rhs_ = unref(_rhs);
    if (&lhs_ == &rhs_) {
        return true;
    }
    size_type const l = lhs_.which();
    size_type const r = rhs_.which();
    constexpr size_type rvalue_list_index_ = index_at< operand, rvalue_list >;
    if (l == rvalue_list_index_) {
        auto const & l_ = get< rvalue_list const & >(lhs_);
        if (l_.rvalues_.size() == 1) {
            return (l_.rvalues_.back() == rhs_);
        } else if (r == rvalue_list_index_) {
            return (l_ == get< rvalue_list const & >(rhs_));
        } else {
            return false;
        }
    }
    if (r == rvalue_list_index_) {
        auto const & r_ = get< rvalue_list const & >(rhs_);
        if (r_.rvalues_.size() == 1) {
            return (lhs_ == r_.rvalues_.back());
        } else if (l == rvalue_list_index_) {
            return (get< rvalue_list const & >(lhs_) == r_); // unreachable
        } else {
            return false;
        }
    }
    if (l != r) {
        return false;
    }
    return (*lhs_ == *rhs_);
}

inline
bool
operator == (assignment const & _lhs, assignment const & _rhs)
{
    if (!(_lhs.lhs_ == _rhs.lhs_)) {
        return false;
    }
    if (!(_lhs.operator_ == _rhs.operator_)) {
        return false;
    }
    if (!(_lhs.rhs_ == _rhs.rhs_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (variable_declaration const & _lhs, variable_declaration const & _rhs)
{
    if (!(_lhs.lhs_ == _rhs.lhs_)) {
        return false;
    }
    if (!(_lhs.rhs_ == _rhs.rhs_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (statement_block const & _lhs, statement_block const & _rhs)
{
    if (!(_lhs.statements_ == _rhs.statements_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (statement const & _lhs, statement const & _rhs)
{
    if (_lhs.which() != _rhs.which()) {
        return false;
    }
    if (!(*_lhs == *_rhs)) {
        return false;
    }
    return true;
}

inline
bool
operator == (entry_definition const & _lhs, entry_definition const & _rhs)
{
    if (!(_lhs.entry_name_ == _rhs.entry_name_)) {
        return false;
    }
    if (!(_lhs.argument_list_ == _rhs.argument_list_)) {
        return false;
    }
    if (!(_lhs.body_ == _rhs.body_)) {
        return false;
    }
    if (!(_lhs.return_statement_ == _rhs.return_statement_)) {
        return false;
    }
    return true;
}

inline
bool
operator < (entry_definition const & _lhs, entry_definition const & _rhs)
{
    if (!(_lhs.entry_name_ < _rhs.entry_name_)) {
        return false;
    }
    return true;
}

inline
bool
operator == (program const & _lhs, program const & _rhs)
{
    return (_lhs.entries_ == _rhs.entries_);
}

}
}
