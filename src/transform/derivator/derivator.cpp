#include <insituc/transform/derivator/derivator.hpp>

#include <insituc/transform/derivator/intrinsic.hpp>

#include <insituc/transform/evaluator/evaluator.hpp>
#include <insituc/transform/derivator/context.hpp>
#include <insituc/utility/reverse.hpp>
#include <insituc/utility/append.hpp>

#if defined(_DEBUG) || defined(DEBUG)
#include <insituc/ast/io.hpp>
#endif

#include <versatile/visit.hpp>

#include <utility>
#include <functional>
#include <stdexcept>

#include <cassert>

namespace insituc
{
namespace transform
{

struct derivator
{

    derivator(descriptor & _descriptor)
        : descriptor_(_descriptor)
    { ; }

private :

    // input
    descriptor & descriptor_;

    // output
    size_type arity_ = 0;
    std::deque< ast::identifier > callies_;

    // temporary
    std::deque< ast::symbol_cref > local_variables_; // current set of lvalues available

    enum class variable_type
    {
        argument,
        dependent,
        independent,
    };

    variable_type
    get_variable_type(ast::identifier const & _identifier) const
    {
        size_type position_ = local_variables_.size();
        for (ast::symbol const & local_variable_ : reverse(local_variables_)) { // from inner sopes to outer
            --position_;
            if (local_variable_ == _identifier.symbol_) {
                if (position_ < arity_) {
                    return variable_type::argument;
                } else {
                    return variable_type::dependent;
                }
            }
        }
        return variable_type::independent;
    }

    ast::operand
    derive_binary(ast::operand const & _lhs,
                  ast::binary const _operator,
                  ast::operand const & _rhs)
    {
        ast::operand lhs_ = ast::clone_or_ref(_lhs);
        ast::operand dlhs_ = derive_rvalue(lhs_);
        ast::operand rhs_ = ast::clone_or_ref(_rhs);
        ast::operand drhs_ = derive_rvalue(rhs_);
        using B = ast::binary_expression;
        using I = ast::intrinsic_invocation;
        switch (_operator) {
        case ast::binary::add : {
            return B{std::move(dlhs_), ast::binary::add, std::move(drhs_)};
        }
        case ast::binary::sub : {
            return B{std::move(dlhs_), ast::binary::sub, std::move(drhs_)};
        }
        case ast::binary::mul : {
            return B{B{std::move(dlhs_), ast::binary::mul, rhs_}, ast::binary::add, B{std::move(lhs_), ast::binary::mul, std::move(drhs_)}};
        }
        case ast::binary::div : {
            return B{B{std::move(dlhs_), ast::binary::div, rhs_}, ast::binary::sub, B{B{std::move(lhs_), ast::binary::mul, std::move(drhs_)}, ast::binary::div, I{ast::intrinsic::sqr, {append< ast::rvalues >(std::move(rhs_))}}}};
        }
        case ast::binary::mod : { // http://math.stackexchange.com/questions/672610/
            return B{std::move(dlhs_), ast::binary::sub, B{std::move(drhs_), ast::binary::mul, I{ast::intrinsic::trunc, {append< ast::rvalues >(B{std::move(lhs_), ast::binary::div, std::move(rhs_)})}}}};
        }
        case ast::binary::pow : { // functional power rule
            return B{B{lhs_, ast::binary::pow, rhs_}, ast::binary::mul, B{B{std::move(dlhs_), ast::binary::mul, B{std::move(rhs_), ast::binary::div, lhs_}}, ast::binary::add, B{std::move(drhs_), ast::binary::mul, I{ast::intrinsic::ln, {append< ast::rvalues >(std::move(lhs_))}}}}};
        }
        }
    }

    ast::identifier
    derive_lvalue(ast::identifier _lvalue) const
    {
        _lvalue.derive(descriptor_.top());
        return _lvalue;
    }

    ast::operand
    derive_rvalue(ast::operand const & _operand)
    {
        return visit([&] (auto const & e) -> ast::operand
        {
            return derive_expression(e);
        }, *_operand);
    }

    ast::identifier
    derive_value(ast::identifier const & _lvalue)
    {
        return derive_lvalue(_lvalue);
    }

    ast::operand
    derive_value(ast::operand const & _operand)
    {
        return derive_rvalue(_operand);
    }

    template< typename expanding_list >
    void
    expand(size_type const _arity,
           expanding_list const & _source,
           expanding_list & _destination)
    {
        auto const & head_ = descriptor_.head();
        auto const & last_ = head_.back();
        for (size_type i = head_[last_.first_].offset_; i < last_.offset_; ++i) {
            size_type const offset_ = i * _arity;
            for (size_type j = 0; j < _arity; ++j) {
                assert(offset_ + j < _source.size());
                _destination.push_back(derive_value(_source[offset_ + j]));
            }
        }
    }

    ast::lvalues
    derive_lvalues(ast::lvalues const & _lvalues,
                   bool const _is_argument = false)
    {
        assert(!_lvalues.empty());
        size_type width_ = 0;
        for (ast::identifier const & lvalue_ : _lvalues) {
            if (!lvalue_.is_original()) {
                break;
            }
            local_variables_.emplace_back(lvalue_.symbol_);
            ++width_;
        }
#ifndef NDEBUG
        size_type const lvalues_size_ = _lvalues.size();
        size_type const prev_width_ = descriptor_.prev_arity();
#endif
        assert(width_ * prev_width_ == lvalues_size_);
        assert((width_ != lvalues_size_) || !descriptor_.head().empty());
        assert((width_ == lvalues_size_) || !_lvalues.at(width_).is_original());
        if (_is_argument) {
            arity_ = width_;
        }
        ast::lvalues lvalues_ = _lvalues;
        expand(width_, _lvalues, lvalues_);
        return lvalues_;
    }

    ast::rvalues
    derive_rvalues(ast::rvalues const & _rvalues)
    {
        assert(!_rvalues.empty());
        size_type const prev_arity_ = descriptor_.prev_arity();
        assert(prev_arity_ != 0);
        assert((_rvalues.size() % prev_arity_) == 0);
        ast::rvalues rvalues_;
        for (ast::rvalue const & rvalue_ : _rvalues) {
            rvalues_.push_back(ast::clone_or_ref(rvalue_));
        }
        expand(_rvalues.size() / prev_arity_, _rvalues, rvalues_);
        return rvalues_;
    }

    ast::rvalue_list
    derive_rvalue_list(ast::rvalue_list const & _rvalue_list)
    {
        ast::rvalue_list rvalue_list_;
        for (ast::rvalue const & rvalue_ : _rvalue_list.rvalues_) {
            rvalue_list_.rvalues_.push_back(derive_rvalue(rvalue_));
        }
        return rvalue_list_;
    }

    [[noreturn]]
    ast::operand
    derive_expression(ast::empty const & /*_empty*/)
    {
        throw std::runtime_error("empty expression cannot be derived");
    }

    ast::operand
    derive_expression(G const & /*_value*/)
    {
        return ast::constant::zero;
    }

    ast::operand
    derive_expression(ast::constant const & /*_constant*/)
    {
        return ast::constant::zero;
    }

    ast::operand
    derive_expression(ast::intrinsic_invocation const & _intrinsic_invocation)
    {
        ast::rvalues arguments_;
        ast::rvalues darguments_;
        for (ast::rvalue const & argument_ : _intrinsic_invocation.argument_list_.rvalues_) { // piecewise
            arguments_.emplace_back(ast::clone_or_ref(argument_));
            darguments_.push_back(derive_rvalue(argument_));
        }
        return ast::rvalue_list{derive_intrinsic(_intrinsic_invocation.intrinsic_, std::move(arguments_), std::move(darguments_))};
    }

    ast::operand
    derive_expression(ast::entry_substitution const & _entry_substitution)
    {
        ast::entry_substitution entry_substitution_;
        entry_substitution_.entry_name_ = derive_lvalue(_entry_substitution.entry_name_);
        callies_.push_back(entry_substitution_.entry_name_);
        if (!_entry_substitution.argument_list_.rvalues_.empty()) {
            auto const rollback_(std::move(descriptor_)); // until operator . became available
            descriptor_.push(entry_substitution_.entry_name_.wrts_);
            entry_substitution_.argument_list_.rvalues_ = derive_rvalues(_entry_substitution.argument_list_.rvalues_);
            (void)rollback_;
        }
        return std::move(entry_substitution_);
    }

    ast::operand
    derive_expression(ast::identifier const & _identifier)
    {
        switch (get_variable_type(_identifier)) {
        case variable_type::argument : {
            break;
        }
        case variable_type::dependent : {
            break;
        }
        case variable_type::independent : {
            if (_identifier.symbol_ == descriptor_.top()) {
                assert(_identifier.is_original()); // valid until DCE introduced
                return ast::constant::one;
            } else {
                return ast::constant::zero;
            }
        }
        }
        return derive_lvalue(_identifier);
    }

    ast::operand
    derive_expression(ast::unary_expression const & _unary_expression)
    {
        switch (_unary_expression.operator_) {
        case ast::unary::plus : {
            return derive_rvalue(_unary_expression.operand_);
        }
        case ast::unary::minus : {
            return ast::unary_expression{ast::unary::minus, derive_rvalue(_unary_expression.operand_)};
        }
        }
    }

    ast::operand
    derive_expression(ast::expression const & _expression)
    {
        if (_expression.rest_.empty()) { // scalar
            return derive_rvalue(_expression.first_);
        } else if (_expression.rest_.size() == 1) { // binary
            ast::operation const & rhs_ = _expression.rest_.back();
            return derive_binary(_expression.first_,
                                 rhs_.operator_,
                                 rhs_.operand_);
        } else {
            throw std::logic_error("transform to binary form first");
        }
    }

    ast::operand
    derive_expression(ast::binary_expression const & _binary_expression)
    {
        return derive_binary(_binary_expression.lhs_,
                             _binary_expression.operator_,
                             _binary_expression.rhs_);
    }

    ast::operand
    derive_expression(ast::rvalue_list const & _rvalue_list)
    {
        return derive_rvalue_list(_rvalue_list);
    }

    ast::operand
    derive_expression(ast::operand_cptr const & _operand_cptr)
    {
        return derive_rvalue(*_operand_cptr);
    }

    ast::statement
    derive_statement(ast::empty const & _empty)
    {
        return _empty;
    }

    ast::statement
    derive_statement(ast::variable_declaration const & _variable_declaration)
    {
        return ast::variable_declaration{{derive_lvalues(_variable_declaration.lhs_.lvalues_)}, {derive_rvalues(_variable_declaration.rhs_.rvalues_)}};
    }

    [[noreturn]]
    ast::statement
    derive_statement(ast::assignment const & /*_assignment*/)
    {
        throw std::runtime_error("can't infer a simple analitycal representation for the derivative of an assignment statement");
    }

    ast::statement
    derive_statement(ast::statement_block const & _statement_block)
    {
        size_type frame_used_ = local_variables_.size();
        ast::statement_block statement_block_{derive_statements(_statement_block.statements_)};
        assert(!(local_variables_.size() < frame_used_));
        if (frame_used_ < local_variables_.size())  {
            local_variables_.resize(frame_used_, local_variables_.back()); // http://stackoverflow.com/a/34818105/1430927
        }
        return statement_block_;
    }

    ast::statement
    derive_statement(ast::statement const & _statement)
    {
        return visit([&] (auto const & s) -> ast::statement
        {
            return derive_statement(s);
        }, *_statement);
    }

    ast::statements
    derive_statements(ast::statements const & _statements)
    {
        ast::statements statements_;
        for (ast::statement const & statement_ : _statements) {
            statements_.push_back(derive_statement(statement_));
        }
        return statements_;
    }

    ast::entry_definition
    derive(ast::entry_definition const & _entry)
    {
        ast::entry_definition entry_;
        entry_.entry_name_ = derive_lvalue(_entry.entry_name_);
        assert(local_variables_.empty());
        assert(arity_ == 0);
        ast::lvalues const & lvalues_ = _entry.argument_list_.lvalues_;
        if (!lvalues_.empty()) {
            entry_.argument_list_.lvalues_ = derive_lvalues(lvalues_, true);
        }
        size_type frame_used_ = local_variables_.size();
        entry_.body_.statements_ = derive_statements(_entry.body_.statements_);
        entry_.return_statement_ = derive_rvalue_list(_entry.return_statement_);
        assert(!(local_variables_.size() < frame_used_));
        if (frame_used_ < local_variables_.size())  {
            local_variables_.resize(frame_used_, local_variables_.back()); // http://stackoverflow.com/a/34818105/1430927
        }
        assert(local_variables_.size() == arity_);
        return entry_;
    }

    struct is_target
    {

        ast::symbol const & symbol_;
        ast::symbols const & wrts_;

        bool
        operator () (ast::entry_definition const & _entry) const
        {
            if (!(symbol_ == _entry.entry_name_.symbol_)) {
                return false;
            }
            if (!(wrts_ == _entry.entry_name_.wrts_)) {
                return false;
            }
            return true;
        }

    };

public :

    ast::entry_definition const *
    operator () (ast::entries const & _primitives, ast::entries & _derivatives,
                 ast::symbol const &_target, ast::symbols _wrts)
    {
        is_target const is_target_{_target, _wrts};
        for (ast::entry_definition const & primitive_ : reverse(_primitives)) {
            if (is_target_(primitive_)) {
                descriptor_.push(std::move(_wrts));
                return &primitive_;
            }
        }
        if (_wrts.empty()) {
            throw std::runtime_error("wrts is empty but symbol " + _target.name_ + " is not primitive");
        }
        for (ast::entry_definition const & derivative_ : reverse(_derivatives)) {
            if (is_target_(derivative_)) {
                descriptor_.push(std::move(_wrts));
                return &derivative_;
            }
        }
        ast::symbol wrt_ = std::move(_wrts.back());
        _wrts.pop_back();
        auto const * p = (derivator{descriptor_})(_primitives, _derivatives, _target, std::move(_wrts));
        if (!p) {
            throw std::runtime_error("can't derive primitive " + _target.name_);
        }
        descriptor_.push(std::move(wrt_));
        ast::entry_definition derivative_ = evaluate(derive(*p));
        { // callies first
            for (ast::identifier & callee_ : callies_) {
                descriptor callee_descriptor_;
                auto const * c = (derivator{callee_descriptor_})(_primitives, _derivatives, callee_.symbol_, std::move(callee_.wrts_));
                if (!c) {
                    throw std::runtime_error("can't derive callee " + callee_.symbol_.name_);
                }
            }
        }
        _derivatives.push_back(std::move(derivative_)); // caller last
        return &_derivatives.back();
    }

};

ast::program
derive(ast::program const & _primitives,
       ast::identifier _target)
{
    assert(!_primitives.entries_.empty());
    ast::program derivatives_;
    descriptor descriptor_;
    auto const * d = (derivator{descriptor_})(_primitives.entries_, derivatives_.entries_,
                                              std::move(_target.symbol_), std::move(_target.wrts_));
    if (!d) {
        throw std::runtime_error("can't derive  " + _target.symbol_.name_);
    }
    return derivatives_;
}

ast::program
derive(ast::program const & _primitives,
       ast::symbols _targets,
       ast::symbols const & _wrts)
{
    assert(!_primitives.entries_.empty());
    assert(!_targets.empty());
    assert(!_wrts.empty());
    ast::program derivatives_;
    for (ast::symbol & target_ : _targets) {
        descriptor descriptor_;
        auto const * d = (derivator{descriptor_})(_primitives.entries_, derivatives_.entries_,
                                                  target_, _wrts);
        if (!d) {
            throw std::runtime_error("can't derive " + target_.name_);
        }
    }
    return derivatives_;
}

ast::program
derive(ast::program const & _primitives,
       ast::symbols const & _wrts)
{
    assert(!_primitives.entries_.empty());
    assert(!_wrts.empty());
    ast::symbols targets_;
    for (ast::entry_definition const & entry_ : _primitives.entries_) {
        targets_.push_back(entry_.entry_name_.symbol_);
    }
    return derive(_primitives, std::move(targets_), _wrts);
}

}
}
