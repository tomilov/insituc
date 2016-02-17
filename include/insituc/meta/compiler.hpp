#pragma once

#include <insituc/meta/assembler.hpp>
#include <insituc/type_traits.hpp>
#include <insituc/shunting_yard_algorithm.hpp>

#include <versatile/visit.hpp>

namespace insituc
{
namespace meta
{

struct compiler
{

    using result_type = bool;

    compiler(assembler & _assembler)
        : assembler_(_assembler)
        , add_ (*this, mnemocode::fadd)
        , sub_ (*this, mnemocode::fsub, mnemocode::fsubr)
        , subr_(*this, mnemocode::fsubr, mnemocode::fsub)
        , mul_ (*this, mnemocode::fmul)
        , div_ (*this, mnemocode::fdiv, mnemocode::fdivr)
        , divr_(*this, mnemocode::fdivr, mnemocode::fdiv)
    { ; }

    result_type
    operator () (ast::program const & _program) const
    {
        if (!compile(_program)) {
            return false;
        }
        return true;
    }

    result_type
    operator () (ast::programs const & _programs) const
    {
        if (!compile(_programs)) {
            return false;
        }
        return true;
    }

private :

    assembler & assembler_;

    template< typename type >
    result_type
    operator () (type const & _ast) const
    {
        if (!compile(_ast)) {
            return false;
        }
        return true;
    }

    result_type
    operator () (ast::operand const & _lhs,
                 ast::binary const _operator,
                 ast::operand const & _rhs) const;

    template< typename values >
    result_type
    push(values const & _values, size_type const _arity = 1) const
    { // values is either ast:rvalues or ast::rvalue
        static_assert(is_contained_v< values, ast::rvalue, ast::rvalues >);
        size_type const pre_ = assembler_.excess();
        if (!operator () (_values)) {
            return false;
        }
        size_type const post_ = assembler_.excess();
        if (!(pre_ < post_)) {
            return false;
        }
        if (post_ - pre_ != _arity) {
            return false;
        }
        return true;
    }

    result_type
    push_reversed_binary(ast::rvalues const & _arguments) const
    {
        size_type const arity_ = _arguments.size();
        if (arity_ == 1) {
            if (!push(_arguments, 2)) {
                return false;
            }
            if (!assembler_(mnemocode::fxch)) {
                return false;
            }
        } else if (arity_ == 2) {
            if (!push(_arguments.back())) {
                return false;
            }
            if (!push(_arguments.front())) {
                return false;
            }
        } else {
            return false;
        }
        return true;
    }

    result_type
    compile(ast::empty const &) const
    {
        return true;
    }

    result_type
    compile(G const & _x) const
    {
        return assembler_(mnemocode::fld, _x);
    }

    result_type compile(ast::intrinsic_invocation const & _ast) const;
    result_type compile(ast::entry_substitution   const & _ast) const;
    result_type compile(ast::constant             const   _ast) const;
    result_type compile(ast::identifier           const & _ast) const;
    result_type compile(ast::unary_expression     const & _ast) const;

    result_type
    compile(ast::binary_expression const & _ast) const;

    class arithmetic
    {

        compiler const & compiler_;
        assembler & assembler_;
        mnemocode const preorder_;
        mnemocode const postorder_;

    public :

        arithmetic(compiler const & _compiler,
                   mnemocode const _preorder,
                   mnemocode const _postorder)
            : compiler_(_compiler)
            , assembler_(compiler_.assembler_)
            , preorder_(_preorder)
            , postorder_(_postorder)
        { ; }

        arithmetic(compiler const & _compiler,
                   mnemocode const _preorder)
            : compiler_(_compiler)
            , assembler_(compiler_.assembler_)
            , preorder_(_preorder)
            , postorder_(_preorder)
        { ; }

    private :

        template< typename lhs, typename rhs >
        std::enable_if_t< (std::is_same_v< lhs, ast::empty > || std::is_same_v< rhs, ast::empty >), result_type >
        compile(lhs const &, rhs const &) const
        {
            return false;
        }

        template< typename lhs >
        std::enable_if_t< !is_contained_v< lhs, ast::empty >, result_type >
        compile(lhs const & _lhs, G const & _rhs) const
        {
            if (!compiler_(_lhs)) {
                return false;
            }
            if (use_long_double) {
                return assembler_(mnemocode::fld, _rhs,
                                  preorder_);
            } else {
                return assembler_(preorder_, _rhs);
            }
        }

        template< typename rhs >
        std::enable_if_t< !is_contained_v< rhs, ast::empty, G >, result_type >
        compile(G const & _lhs, rhs const & _rhs) const
        {
            if (!compiler_(_rhs)) {
                return false;
            }
            if (use_long_double) {
                return assembler_(mnemocode::fld, _lhs,
                                  postorder_);
            } else {
                return assembler_(postorder_, _lhs);
            }
        }

        template< typename rhs >
        std::enable_if_t< !is_contained_v< rhs, ast::empty, G >, result_type >
        compile(ast::identifier const & _lhs, rhs const & _rhs) const
        {
            if (!compiler_(_rhs)) {
                return false;
            }
            if (use_long_double) {
                return assembler_(mnemocode::fld, _lhs,
                                  postorder_);
            } else {
                return assembler_(postorder_, _lhs);
            }
        }

        template< typename lhs >
        std::enable_if_t< !is_contained_v< lhs, ast::empty, G, ast::identifier >, result_type >
        compile(lhs const & _lhs, ast::identifier const & _rhs) const
        {
            if (!compiler_(_lhs)) {
                return false;
            }
            if (use_long_double) {
                return assembler_(mnemocode::fld, _rhs,
                                  preorder_);
            } else {
                return assembler_(preorder_, _rhs);
            }
        }

        template< typename lhs, typename rhs >
        std::enable_if_t< !(std::is_same_v< lhs, ast::empty > || std::is_same_v< rhs, ast::empty >), result_type >
        compile(lhs const & _lhs, rhs const & _rhs) const
        {
            if (!compiler_(_lhs)) {
                return false;
            }
            if (!compiler_(_rhs)) {
                return false;
            }
            return assembler_(preorder_);
        }

        template< typename type >
        result_type
        compile(type const & _operand) const
        {
            if (!compiler_(_operand)) {
                return false;
            }
            return assembler_(preorder_);
        }

        result_type
        compile(G const & _literal) const
        {
            if (use_long_double) {
                return assembler_(mnemocode::fld, _literal,
                                  preorder_);
            } else {
                return assembler_(preorder_, _literal);
            }
        }

        result_type
        compile(ast::identifier const & _identifier) const
        {
            if (use_long_double) {
                return assembler_(mnemocode::fld, _identifier,
                                  preorder_);
            } else {
                return assembler_(preorder_, _identifier);
            }
        }

        result_type
        compile(ast::expression const & _expression) const
        {
            if (use_long_double) {
                if (_expression.rest_.empty()) {
                    return compile(_expression.first_);
                }
            }
            if (!compiler_(_expression)) {
                return false;
            }
            return assembler_(preorder_);
        }

        result_type
        compile(ast::empty const & /*_empty*/) const
        {
            return false; // "empty operand in arithmetic expression"
        }

    public :

        result_type
        operator () (ast::operand const & _lhs, ast::operand const & _rhs) const
        {
            auto const compile_ = [&] (auto const & l, auto const & r) -> result_type
            {
                return compile(l, r);
            };
            if (!multivisit(compile_, *unref(_lhs), *unref(_rhs))) {
                return false;
            }
            return true;
        }

        result_type
        operator () (ast::operand const & _operand) const
        {
            if (!visit([&] (auto const & o) -> result_type { return compile(o); }, *unref(_operand))) {
                return false;
            }
            return true;
        }

        result_type
        operator () () const
        {
            return assembler_(preorder_);
        }

    };

    arithmetic const add_;
    arithmetic const sub_;
    arithmetic const subr_;
    arithmetic const mul_;
    arithmetic const div_;
    arithmetic const divr_;

    struct tree_switch
    {

        using result_type = typename compiler::result_type;

        using shunting_yard_algorithm_type = shunting_yard_algorithm< tree_switch const, false >;
        using node_type = typename shunting_yard_algorithm_type::node_type;

        tree_switch(compiler const & _compiler)
            : compiler_(_compiler)
            , assembler_(compiler_.assembler_)
            , add_{compiler_.add_, compiler_.add_}
            , sub_{compiler_.sub_, compiler_.subr_}
            , mul_{compiler_.mul_, compiler_.mul_}
            , div_{compiler_.div_, compiler_.divr_}
            , rpn_(*this)
            , operation_commutator_(rpn_)
        { ; }

        result_type
        traverse(ast::expression const & _expression)
        {
            return rpn_.traverse(_expression);
        }

        result_type
        operator () (ast::operand const & _ast) const
        {
            return compiler_(_ast);
        }

        result_type
        operator () (ast::operand const & _lhs,
                     ast::binary const _operator,
                     ast::operand const & _rhs) const
        {
            return compiler_(_lhs,
                             _operator,
                             _rhs);
        }

        result_type
        operator () (node_type const & _lhs,
                     ast::binary const _operator,
                     node_type const & _rhs) const;

    private :

        compiler const & compiler_;
        assembler & assembler_;

        struct anticommutative_operator
        {

            arithmetic const & forward_;
            arithmetic const & reverse_;

        };

        anticommutative_operator const add_;
        anticommutative_operator const sub_;
        anticommutative_operator const mul_;
        anticommutative_operator const div_;

        shunting_yard_algorithm_type rpn_;

        struct operation_commutator
        {

            using lhs_op_rhs = typename shunting_yard_algorithm_type::lhs_op_rhs;
            using operand_ptr = typename shunting_yard_algorithm_type::operand_ptr;

            operation_commutator(shunting_yard_algorithm_type const & _rpn)
                : rpn_(_rpn)
            { ; }

            result_type
            operator () (node_type const & _lhs,
                         anticommutative_operator const & _operator,
                         node_type const & _rhs) const
            {
                return multivisit([&] (auto const & l, auto const & r) -> result_type
                {
                    return dispatch(l, _operator, r);
                }, _lhs, _rhs);
            }

            result_type
            operator () (anticommutative_operator const & _operator,
                         node_type const & _rhs) const
            {
                return visit([&] (auto const & r) -> result_type
                {
                    return dispatch(_operator, r);
                }, _rhs);
            }

        private :

            result_type
            dispatch(lhs_op_rhs const & _lhs,
                     anticommutative_operator const & _operator,
                     lhs_op_rhs const & _rhs) const
            {
                return rpn_(_lhs) && rpn_(_rhs) && _operator.forward_();
                // <=> return rpn_(_rhs) && rpn_(_lhs) && _operator.reverse_();
            }

            result_type
            dispatch(lhs_op_rhs const & _lhs,
                     anticommutative_operator const & _operator,
                     operand_ptr const _rhs) const
            {
                return rpn_(_lhs) && _operator.forward_(*_rhs);
            }

            result_type
            dispatch(operand_ptr const _lhs,
                     anticommutative_operator const & _operator,
                     lhs_op_rhs const & _rhs) const
            {
                return rpn_(_rhs) && _operator.reverse_(*_lhs);
            }

            result_type
            dispatch(operand_ptr const _lhs,
                     anticommutative_operator const & _operator,
                     operand_ptr const _rhs) const
            {
                return _operator.forward_(*_lhs, *_rhs);
                // <=> return _operator.reverse_(_rhs.get(), _lhs.get());
            }

            result_type
            dispatch(anticommutative_operator const & _operator,
                     operand_ptr const _rhs) const
            {
                return _operator.forward_(*_rhs);
            }

            result_type
            dispatch(anticommutative_operator const & _operator,
                     lhs_op_rhs const & _rhs) const
            {
                return rpn_(_rhs) && _operator.forward_();
            }

            shunting_yard_algorithm_type const & rpn_;

        };

        operation_commutator const operation_commutator_;

    };

    result_type
    compile(ast::expression const & _expression) const
    {
        if (_expression.rest_.empty()) {
            return operator () (_expression.first_);
        } else if (_expression.rest_.size() == 1) {
            ast::operation const & op_rhs_ = _expression.rest_.back();
            return operator () (_expression.first_,
                                op_rhs_.operator_,
                                op_rhs_.operand_);
        } else {
            tree_switch Dijkstra(*this);
            return Dijkstra.traverse(_expression); // RPN
        }
    }

    result_type
    compile(ast::operand_cptr const & _operand_cptr) const
    {
        return compile(*_operand_cptr);
    }

    result_type
    compile(ast::operand const & _operand) const
    {
        if (!visit([&] (auto const & o) -> result_type { return compile(o); }, *_operand)) {
            return false;
        }
        return true;
    }

    result_type compile(ast::lvalue_list const & _ast) const;
    result_type compile(ast::rvalues     const & _ast) const;
    result_type compile(ast::rvalue_list const & _ast) const;

    result_type compile_chs      (ast::rvalues const & _arguments) const;
    result_type compile_abs      (ast::rvalues const & _arguments) const;
    result_type compile_twice    (ast::rvalues const & _arguments) const;
    result_type compile_sumsqr   (ast::rvalues const & _arguments) const;
    result_type compile_sumsqr   (ast::rvalue  const & _argument ) const;
    result_type compile_sqrt     (ast::rvalues const & _arguments) const;
    result_type compile_round    (ast::rvalues const & _arguments) const;
    result_type compile_trunc    (ast::rvalues const & _arguments) const;
    result_type compile_remainder(ast::rvalues const & _arguments) const;
    result_type compile_cos      (ast::rvalues const & _arguments) const;
    result_type compile_sin      (ast::rvalues const & _arguments) const;
    result_type compile_sincos   (ast::rvalues const & _arguments) const;
    result_type compile_tg       (ast::rvalues const & _arguments) const;
    result_type compile_ctg      (ast::rvalues const & _arguments) const;
    result_type compile_arctg    (ast::rvalues const & _arguments) const;
    result_type compile_atan2    (ast::rvalues const & _arguments) const;
    result_type compile_poly     (ast::rvalues const & _arguments) const;
    result_type compile_frac     (ast::rvalues const & _arguments) const;
    result_type compile_intrem   (ast::rvalues const & _arguments) const;
    result_type compile_fracint  (ast::rvalues const & _arguments) const;
    result_type compile_pow      (ast::rvalues const & _arguments) const;
    result_type compile_exp      (ast::rvalues const & _arguments) const;
    result_type compile_pow2     (ast::rvalues const & _arguments) const;
    result_type compile_sqr      (ast::rvalues const & _arguments) const;
    result_type compile_log      (ast::rvalues const & _arguments) const;
    result_type compile_ln       (ast::rvalues const & _arguments) const;
    result_type compile_log2     (ast::rvalues const & _arguments) const;
    result_type compile_lg       (ast::rvalues const & _arguments) const;
    result_type compile_yl2xp1   (ast::rvalues const & _arguments) const;
    result_type compile_scale2   (ast::rvalues const & _arguments) const;
    result_type compile_extract  (ast::rvalues const & _arguments) const;
    result_type compile_pow2m1   (ast::rvalues const & _arguments) const;
    result_type compile_arcsin   (ast::rvalues const & _arguments) const;
    result_type compile_arccos   (ast::rvalues const & _arguments) const;
    result_type compile_max      (ast::rvalues const & _arguments) const;
    result_type compile_max      (ast::rvalue  const & _argument ) const;
    result_type compile_min      (ast::rvalues const & _arguments) const;
    result_type compile_min      (ast::rvalue  const & _argument ) const;

    result_type call_intrinsic(ast::intrinsic const _intrinsic,
                               ast::rvalues const & _arguments) const;

    result_type assignment(ast::assign const _op,
                           ast::lvalue_list const & _lhs,
                           size_type const _start,
                           size_type const _count) const;
    result_type compile(ast::assignment           const & _ast) const;
    result_type compile(ast::variable_declaration const & _ast) const;

    result_type compile(ast::statement            const & _ast) const;
    result_type compile(ast::statements           const & _ast) const;
    result_type compile(ast::statement_block      const & _ast) const;

    result_type compile(ast::entry_definition const & _ast) const;
    result_type compile(ast::program const & _ast) const;
    result_type compile(ast::programs const & _ast) const;

};

}
}
