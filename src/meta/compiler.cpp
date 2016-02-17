#include <insituc/meta/compiler.hpp>

#include <insituc/parser/pragma_parser.hpp>
#include <insituc/utility/reverse.hpp>
#include <insituc/utility/head.hpp>
#include <insituc/utility/tail.hpp>

#include <versatile/visit.hpp>

#include <type_traits>
#include <algorithm>
#include <iterator>
#include <functional>

namespace insituc
{
namespace meta
{

auto
compiler::compile(ast::constant const _constant) const
-> result_type
{
    switch (_constant) {
    case ast::constant::zero : return assembler_(mnemocode::fldz);
    case ast::constant::one  : return assembler_(mnemocode::fld1);
    case ast::constant::pi   : return assembler_(mnemocode::fldpi);
    case ast::constant::l2e  : return assembler_(mnemocode::fldl2e);
    case ast::constant::l2t  : return assembler_(mnemocode::fldl2t);
    case ast::constant::lg2  : return assembler_(mnemocode::fldlg2);
    case ast::constant::ln2  : return assembler_(mnemocode::fldln2);
    }
    return false;
}

auto
compiler::compile(ast::identifier const & _identifier) const
-> result_type
{
    if (!assembler_(mnemocode::fld, _identifier)) {
        return false;
    }
    return true;
}

auto
compiler::compile(ast::lvalue_list const & _lvalue_list) const
-> result_type
{
    for (ast::identifier const & lvalue_ : _lvalue_list.lvalues_) {
        if (!assembler_(mnemocode::fstp, lvalue_)) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile(ast::unary_expression const & _unary_expression) const
-> result_type
{
    if (!push(_unary_expression.operand_)) {
        return false;
    }
    switch (_unary_expression.operator_) {
    case ast::unary::plus : {
        return true;
    }
    case ast::unary::minus : {
        return assembler_(mnemocode::fchs);
    }
    }
    return false;
}

auto
compiler::operator () (ast::operand const & _lhs,
                       ast::binary const _operator,
                       ast::operand const & _rhs) const
-> result_type
{
    switch (_operator) {
    case ast::binary::add : {
        return add_(_lhs, _rhs);
    }
    case ast::binary::sub : {
        return sub_(_lhs, _rhs);
    }
    case ast::binary::mul : {
        return mul_(_lhs, _rhs);
    }
    case ast::binary::div : {
        return div_(_lhs, _rhs);
    }
    case ast::binary::mod : {
        if (!push(_rhs)) {
            return false;
        }
        if (!push(_lhs)) {
            return false;
        }
        return assembler_(mnemocode::fprem,
                          mnemocode::fstp, st(1));
    }
    case ast::binary::pow : {
        if (!assembler_(mnemocode::fldln2)) {
            return false;
        }
        if (!push(_lhs)) {
            return false;
        }
        if (!assembler_(mnemocode::fyl2x)) {
            return false;
        }
        if (!mul_(_rhs)) {
            return false;
        }
        return assembler_(mnemocode::fldl2e,
                          mnemocode::fmul,
                          mnemocode::fld, st,
                          mnemocode::frndint,
                          mnemocode::fxch,
                          mnemocode::fsub, st, st(1),
                          mnemocode::f2xm1,
                          mnemocode::fld1,
                          mnemocode::fadd,
                          mnemocode::fscale,
                          mnemocode::fstp, st(1)); // pwr(l, r)
    }
    }
    return false;
}

auto
compiler::compile(ast::binary_expression const & _binary_expression) const
-> result_type
{
    return operator () (_binary_expression.lhs_,
                        _binary_expression.operator_,
                        _binary_expression.rhs_);
}

auto
compiler::tree_switch::operator () (node_type const & _lhs,
                                    ast::binary const _operator,
                                    node_type const & _rhs) const
-> result_type
{
    switch (_operator) {
    case ast::binary::add : {
        return operation_commutator_(_lhs, add_, _rhs);
    }
    case ast::binary::sub : {
        return operation_commutator_(_lhs, sub_, _rhs);
    }
    case ast::binary::mul : {
        return operation_commutator_(_lhs, mul_, _rhs);
    }
    case ast::binary::div : {
        return operation_commutator_(_lhs, div_, _rhs);
    }
    case ast::binary::mod : {
        if (!rpn_(_rhs)) {
            return false;
        }
        if (!rpn_(_lhs)) {
            return false;
        }
        return assembler_(mnemocode::fprem,
                          mnemocode::fstp, st(1));
    }
    case ast::binary::pow : {
        if (!assembler_(mnemocode::fldln2)) {
            return false;
        }
        if (!rpn_(_lhs)) {
            return false;
        }
        if (!assembler_(mnemocode::fyl2x)) {
            return false;
        }
        if (!operation_commutator_(mul_, _rhs)) {
            return false;
        }
        return assembler_(mnemocode::fldl2e,
                          mnemocode::fmul,
                          mnemocode::fld, st,
                          mnemocode::frndint,
                          mnemocode::fxch,
                          mnemocode::fsub, st, st(1),
                          mnemocode::f2xm1,
                          mnemocode::fld1,
                          mnemocode::fadd,
                          mnemocode::fscale,
                          mnemocode::fstp, st(1)); // pwr(l, r)
    }
    }
    return false;
}

auto
compiler::call_intrinsic(ast::intrinsic const _intrinsic,
                         ast::rvalues const & _arguments) const
-> result_type
{
    switch (_intrinsic) {
    case ast::intrinsic::chs       : return compile_chs      (_arguments);
    case ast::intrinsic::abs       : return compile_abs      (_arguments);
    case ast::intrinsic::twice     : return compile_twice    (_arguments);
    case ast::intrinsic::sumsqr    : return compile_sumsqr   (_arguments);
    case ast::intrinsic::sqrt      : return compile_sqrt     (_arguments);
    case ast::intrinsic::round     : return compile_round    (_arguments);
    case ast::intrinsic::trunc     : return compile_trunc    (_arguments);
    case ast::intrinsic::remainder : return compile_remainder(_arguments);
    case ast::intrinsic::cos       : return compile_cos      (_arguments);
    case ast::intrinsic::sin       : return compile_sin      (_arguments);
    case ast::intrinsic::sincos    : return compile_sincos   (_arguments);
    case ast::intrinsic::tg        : return compile_tg       (_arguments);
    case ast::intrinsic::ctg       : return compile_ctg      (_arguments);
    case ast::intrinsic::arctg     : return compile_arctg    (_arguments);
    case ast::intrinsic::atan2     : return compile_atan2    (_arguments);
    case ast::intrinsic::poly      : return compile_poly     (_arguments);
    case ast::intrinsic::frac      : return compile_frac     (_arguments);
    case ast::intrinsic::intrem    : return compile_intrem   (_arguments);
    case ast::intrinsic::fracint   : return compile_fracint  (_arguments);
    case ast::intrinsic::pow       : return compile_pow      (_arguments);
    case ast::intrinsic::exp       : return compile_exp      (_arguments);
    case ast::intrinsic::pow2      : return compile_pow2     (_arguments);
    case ast::intrinsic::sqr       : return compile_sqr      (_arguments);
    case ast::intrinsic::log       : return compile_log      (_arguments);
    case ast::intrinsic::ln        : return compile_ln       (_arguments);
    case ast::intrinsic::log2      : return compile_log2     (_arguments);
    case ast::intrinsic::lg        : return compile_lg       (_arguments);
    case ast::intrinsic::yl2xp1    : return compile_yl2xp1   (_arguments);
    case ast::intrinsic::scale2    : return compile_scale2   (_arguments);
    case ast::intrinsic::extract   : return compile_extract  (_arguments);
    case ast::intrinsic::pow2m1    : return compile_pow2m1   (_arguments);
    case ast::intrinsic::arcsin    : return compile_arcsin   (_arguments);
    case ast::intrinsic::arccos    : return compile_arccos   (_arguments);
    case ast::intrinsic::max       : return compile_max      (_arguments);
    case ast::intrinsic::min       : return compile_min      (_arguments);
    }
    return false;
}

auto
compiler::compile(ast::intrinsic_invocation const & _ast) const
-> result_type
{
    return call_intrinsic(_ast.intrinsic_, _ast.argument_list_.rvalues_);
}

auto
compiler::compile(ast::entry_substitution const & _ast) const
-> result_type
{
    if (!assembler_.is_function(_ast.entry_name_)) {
        return false;
    }
    size_type const pre_ = assembler_.excess();
    if (!compile(_ast.argument_list_)) {
        return false;
    }
    size_type const post_ = assembler_.excess();
    if (post_ < pre_) { // nullary functions allowed
        return false;
    }
    size_type const arity_ = post_ - pre_;
    if (assembler_.get_function(_ast.entry_name_).arity() != arity_) {
        return false;
    }
    return assembler_(mnemocode::call, _ast.entry_name_);
}

auto
compiler::compile(ast::rvalues const & _rvalues) const
-> result_type
{
    for (ast::rvalue const & rvalue_ : _rvalues) {
        if (!compile(rvalue_)) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile(ast::rvalue_list const & _ast) const
-> result_type
{
    return compile(_ast.rvalues_);
}

auto
compiler::compile(ast::variable_declaration const & _ast) const
-> result_type
{
    if (!assembler_(mnemocode::endl)) {
        return false;
    }
    size_type block_ = 0;
    for (ast::rvalue const & rvalue_ : _ast.rhs_.rvalues_) {
        size_type const pre_ = assembler_.excess();
        if (!compile(rvalue_)) {
            return false;
        }
        size_type const post_ = assembler_.excess();
        if (!(pre_ < post_)) {
            return false;
        }
        size_type const width_ = post_ - pre_;
        if (_ast.lhs_.lvalues_.size() - block_ < width_) {
            return false;
        }
        for (size_type i = width_; 0 < i; --i) {
            if (!assembler_(mnemocode::alloca_, _ast.lhs_.lvalues_.at(block_ + (i - 1)))) {
                return false;
            }
        }
        if (!assembler_(mnemocode::endl)) {
            return false;
        }
        block_ += width_;
    }
    if (block_ != _ast.lhs_.lvalues_.size()) {
        return false;
    }
    return true;
}

auto
compiler::assignment(ast::assign const _operator,
                     ast::lvalue_list const & _lhs,
                     size_type const _start,
                     size_type const _count) const
-> result_type
{
    for (size_type i = _count; 0 < i; --i) {
        ast::identifier const & lvalue_ = _lhs.lvalues_.at(_start + (i - 1));
        switch (_operator) {
        case ast::assign::assign : {
            break;
        }
        case ast::assign::plus_assign : {
            if (!add_(lvalue_)) {
                return false;
            }
            break;
        }
        case ast::assign::minus_assign : {
            if (!subr_(lvalue_)) {
                return false;
            }
            break;
        }
        case ast::assign::times_assign : {
            if (!mul_(lvalue_)) {
                return false;
            }
            break;
        }
        case ast::assign::divide_assign : {
            if (!divr_(lvalue_)) {
                return false;
            }
            break;
        }
        case ast::assign::mod_assign : {
            if (!assembler_(mnemocode::fld, lvalue_,
                            mnemocode::fprem,
                            mnemocode::fstp, st(1))) {
                return false;
            }
            break;
        }
        case ast::assign::raise_assign : {
            if (!assembler_(mnemocode::fldln2,
                            mnemocode::fld, lvalue_,
                            mnemocode::fyl2x,
                            mnemocode::fmul,
                            mnemocode::fldl2e,
                            mnemocode::fmul,
                            mnemocode::fld, st,
                            mnemocode::frndint,
                            mnemocode::fxch,
                            mnemocode::fsub, st, st(1),
                            mnemocode::f2xm1,
                            mnemocode::fld1,
                            mnemocode::fadd,
                            mnemocode::fscale,
                            mnemocode::fstp, st(1))) {
                return false;
            }
            break;
        }
        }
        if (!assembler_(mnemocode::fstp, lvalue_)) {
            return false;
        }
    }
    if (!assembler_(mnemocode::endl)) {
        return false;
    }
    return true;
}

auto
compiler::compile(ast::assignment const & _ast) const
-> result_type
{
    if (!assembler_(mnemocode::endl)) {
        return false;
    }
    size_type block_ = 0;
    for (ast::rvalue const & rvalue_ : _ast.rhs_.rvalues_) {
        size_type const pre_ = assembler_.excess();
        if (!compile(rvalue_)) {
            return false;
        }
        size_type const post_ = assembler_.excess();
        if (!(pre_ < post_)) {
            return false;
        }
        size_type const width_ = post_ - pre_;
        if (_ast.lhs_.lvalues_.size() - block_ < width_) {
            return false;
        }
        if (!assignment(_ast.operator_, _ast.lhs_, block_, width_)) {
            return false;
        }
        block_ += width_;
    }
    if (block_ != _ast.lhs_.lvalues_.size()) { // check arities matching
        return false;
    }
    return true;
}

auto
compiler::compile(ast::statement const & _ast) const
-> result_type
{
    if (!visit([&] (auto const & s) -> result_type { return compile(s); }, *_ast)) {
        return false;
    }
    return true;
}

auto
compiler::compile(ast::statements const & _ast) const
-> result_type
{
    for (ast::statement const & ast_ : _ast) {
        if (!compile(ast_)) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile(ast::statement_block const & _ast) const
-> result_type
{
    if (!assembler_(mnemocode::bra)) {
        return false;
    }
    if (!compile(_ast.statements_)) {
        return false;
    }
    if (!assembler_(mnemocode::ket)) {
        return false;
    }
    return true;
}

auto
compiler::compile(ast::entry_definition const & _ast) const
-> result_type
{
    // TODO: warning message about global symbol shadowing
    symbols_type arguments_;
    for (ast::identifier const & argument_ : _ast.argument_list_.lvalues_) {
        arguments_.push_back(argument_);
    }
    if (!assembler_.enter(_ast.entry_name_, 0, std::move(arguments_))) {
        return false;
    }
    if (!assembler_(mnemocode::bra)) {
        return false;
    }
    if (!compile(_ast.body_.statements_)) {
        return false;
    }
    if (!compile(_ast.return_statement_)) {
        return false;
    }
    if (!assembler_(mnemocode::ret, assembler_.excess(), parser::parse_pragma(_ast.return_statement_.pragma_, "drop"))) {
        return false;
    }
    if (!assembler_(mnemocode::ket)) {
        return false;
    }
    assembler_.leave();
    return true;
}

auto
compiler::compile(ast::program const & _program) const
-> result_type
{
    if (_program.entries_.empty()) {
        return false;
    }
    for (ast::entry_definition const & entry_ : _program.entries_) {
        if (!compile(entry_)) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile(ast::programs const & _programs) const
-> result_type
{
    if (_programs.empty()) {
        return false;
    }
    for (ast::program const & program_ : _programs) {
        if (!compile(program_)) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile_chs(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fchs);
}

auto
compiler::compile_abs(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fabs);
}

auto
compiler::compile_twice(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fadd, st, st(0));
}

auto
compiler::compile_sumsqr(ast::rvalues const & _arguments) const
-> result_type
{
    if (_arguments.empty()) {
        return false;
    }
    if (!compile_sumsqr(_arguments.front())) {
        return false;
    }
    for (ast::rvalue const & argument_ : tail(_arguments)) {
        if (!compile_sumsqr(argument_)) {
            return false;
        }
        if (!assembler_(mnemocode::fadd)) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile_sumsqr(ast::rvalue const & _argument) const
-> result_type
{
    size_type const pre_ = assembler_.excess();
    if (!compile(_argument)) {
        return false;
    }
    size_type const post_ = assembler_.excess();
    if (!(pre_ < post_)) {
        return false;
    }
    size_type count_ = post_ - pre_;
    if (!assembler_(mnemocode::fmul, st, st(0))) {
        return false;
    }
    while (0 < --count_) {
        if (!assembler_(mnemocode::fxch,
                        mnemocode::fmul, st, st(0),
                        mnemocode::fadd)) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile_sqrt(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fsqrt);
}

auto
compiler::compile_round(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::frndint);
}

auto
compiler::compile_trunc(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::trunc);
}

auto
compiler::compile_remainder(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push_reversed_binary(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fprem1,
                      mnemocode::fstp, st(1));
}

auto
compiler::compile_cos(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fcos);
}

auto
compiler::compile_sin(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fsin);
}

auto
compiler::compile_sincos(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fsincos);
}

auto
compiler::compile_tg(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fptan,
                      mnemocode::fstp, st);
}

auto
compiler::compile_ctg(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fptan,
                      mnemocode::fdivr);
}

auto
compiler::compile_arctg(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fld1,
                      mnemocode::fpatan);
}

auto
compiler::compile_atan2(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments, 2)) {
        return false;
    }
    return assembler_(mnemocode::fpatan);
}

auto
compiler::compile_poly(ast::rvalues const & _arguments) const
-> result_type
{
    size_type const arity_ = _arguments.size();
    if (arity_ == 0) {
        return false;
    } else if (arity_ == 1) {
        if (!assembler_(mnemocode::fldz)) {
            return false;
        }
        return true;
    } else if (arity_ == 2) {
        if (!push(_arguments.back())) { // (a(0))
            return false;
        }
        return true;
    }
    // Horner's method
    if (!push(_arguments.front())) { // (x)...
        return false;
    }
    if (!push(_arguments.back())) { // (a(n))
        return false;
    }
    for (ast::rvalue const & argument_ : head(tail(reverse(_arguments)))) {
        if (!assembler_(mnemocode::fmul, st, st(1))) { // (result*x) x
            return false;
        }
        if (auto const * const constant_ = get< ast::constant >(&argument_)) {
            if (*constant_ == ast::constant::zero) {
                continue;
            }
        }
        if (!add_(argument_)) { // (result*x + a(i)) x
            return false;
        }
    }
    if (!assembler_(mnemocode::fstp, st(1))) { // (result)
        return false;
    }
    return true; // (a0 * x^0 + a1 * x^1 + a2 * x^2 + ... + ai * x^i + ... + an * x^n)
}

auto
compiler::compile_frac(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fld, st,
                      mnemocode::frndint,
                      mnemocode::fsub
                      );
}

auto
compiler::compile_intrem(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments, 2)) {
        return false;
    }
    return assembler_(mnemocode::fld, st(1),
                      mnemocode::fprem,
                      mnemocode::fstp, st(1),
                      mnemocode::fsub, st(1), st
                      );
}

auto
compiler::compile_fracint(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fld, st,
                      mnemocode::trunc,
                      mnemocode::fsub, st(1), st
                      );
}

auto
compiler::compile_pow(ast::rvalues const & _arguments) const
-> result_type
{
    size_type const arity_ = _arguments.size();
    if (arity_ == 2) {
        if (!assembler_(mnemocode::fldln2)) {
            return false;
        }
        if (!push(_arguments.front())) {
            return false;
        }
        if (!assembler_(mnemocode::fyl2x)) {
            return false;
        }
        if (!mul_(_arguments.back())) {
            return false;
        }
        return assembler_(mnemocode::fldl2e, // TODO: fld1 *
                          mnemocode::fmul,
                          mnemocode::fld, st,
                          mnemocode::frndint,
                          mnemocode::fxch,
                          mnemocode::fsub, st, st(1),
                          mnemocode::f2xm1,
                          mnemocode::fld1,
                          mnemocode::fadd,
                          mnemocode::fscale,
                          mnemocode::fstp, st(1)
                          );
    } else if (arity_ == 1) {
        if (!push(_arguments, 2)) {
            return false;
        }
        if (!assembler_(mnemocode::fxch,
                        mnemocode::fldln2,
                        mnemocode::fxch,
                        mnemocode::fyl2x,
                        mnemocode::fmul,
                        mnemocode::fldl2e,
                        mnemocode::fmul,
                        mnemocode::fld, st,
                        mnemocode::frndint,
                        mnemocode::fxch,
                        mnemocode::fsub, st, st(1),
                        mnemocode::f2xm1,
                        mnemocode::fld1,
                        mnemocode::fadd,
                        mnemocode::fscale,
                        mnemocode::fstp, st(1)
                        )) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

auto
compiler::compile_exp(ast::rvalues const & _arguments) const
-> result_type
{
    if (_arguments.size() != 1) {
        return false;
    }
    if (!assembler_(mnemocode::fldl2e)) {
        return false;
    }
    if (!mul_(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::fld, st,
                      mnemocode::frndint,
                      mnemocode::fxch,
                      mnemocode::fsub, st, st(1),
                      mnemocode::f2xm1,
                      mnemocode::fld1,
                      mnemocode::fadd,
                      mnemocode::fscale,
                      mnemocode::fstp, st(1)
                      );
}

auto
compiler::compile_pow2(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fld, st,
                      mnemocode::frndint,
                      mnemocode::fxch,
                      mnemocode::fsub, st, st(1),
                      mnemocode::f2xm1,
                      mnemocode::fld1,
                      mnemocode::fadd,
                      mnemocode::fscale,
                      mnemocode::fstp, st(1)
                      );
}

auto
compiler::compile_sqr(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fmul, st, st(0));
}

auto
compiler::compile_log(ast::rvalues const & _arguments) const
-> result_type
{
    size_type const arity_ = _arguments.size();
    if (arity_ == 1) {
        if (!push(_arguments, 2)) {
            return false;
        }
        if (!assembler_(mnemocode::fld1,
                        mnemocode::fxch,
                        mnemocode::fyl2x,
                        mnemocode::fxch,
                        mnemocode::fld1,
                        mnemocode::fxch,
                        mnemocode::fyl2x,
                        mnemocode::fdiv)) {
            return false;
        }
    } else if (arity_ == 2) {
        if (!assembler_(mnemocode::fld1)) {
            return false;
        }
        if (!push(_arguments.back())) {
            return false;
        }
        if (!assembler_(mnemocode::fyl2x,
                        mnemocode::fld1)) {
            return false;
        }
        if (!push(_arguments.front())) {
            return false;
        }
        if (!assembler_(mnemocode::fyl2x,
                        mnemocode::fdiv)) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

auto
compiler::compile_ln(ast::rvalues const & _arguments) const
-> result_type
{
    if (_arguments.size() != 1) {
        return false;
    }
    if (!assembler_(mnemocode::fldln2)) {
        return false;
    }
    if (!push(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::fyl2x);
}

auto
compiler::compile_log2(ast::rvalues const & _arguments) const
-> result_type
{
    if (_arguments.size() != 1) {
        return false;
    }
    if (!assembler_(mnemocode::fld1)) {
        return false;
    }
    if (!push(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::fyl2x);
}

auto
compiler::compile_lg(ast::rvalues const & _arguments) const
-> result_type
{
    if (_arguments.size() != 1) {
        return false;
    }
    if (!assembler_(mnemocode::fldlg2)) {
        return false;
    }
    if (!push(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::fyl2x);
}

auto
compiler::compile_yl2xp1(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push_reversed_binary(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fyl2xp1);
}

auto
compiler::compile_scale2(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push_reversed_binary(_arguments)) {
        return false;
    }
    return assembler_(mnemocode::fscale,
                      mnemocode::fstp, st(1));
}

auto
compiler::compile_extract(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::fxtract);
}

auto
compiler::compile_pow2m1(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::f2xm1);
}

auto
compiler::compile_arcsin(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::fld1,
                      mnemocode::fld, st(1),
                      mnemocode::fmul, st, st(0),
                      mnemocode::fsub,
                      mnemocode::fsqrt,
                      mnemocode::fpatan
                      );
}

auto
compiler::compile_arccos(ast::rvalues const & _arguments) const
-> result_type
{
    if (!push(_arguments.back())) {
        return false;
    }
    return assembler_(mnemocode::fld, st,
                      mnemocode::fmul, st, st(0),
                      mnemocode::fld1,
                      mnemocode::fsubr,
                      mnemocode::fsqrt,
                      mnemocode::fxch,
                      mnemocode::fpatan
                      );
}

auto
compiler::compile_max(ast::rvalues const & _arguments) const
-> result_type
{
    if (_arguments.empty()) {
        return false;
    }
    if (!compile_max(_arguments.front())) {
        return false;
    }
    for (ast::rvalue const & argument_ : tail(_arguments)) {
        if (!compile_max(argument_)) {
            return false;
        }
        if (!assembler_(mnemocode::fucomi, st, st(1),
                        mnemocode::fcmovb, st, st(1),
                        mnemocode::fstp, st(1))) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile_max(ast::rvalue const & _argument) const
-> result_type
{
    size_type const pre_ = assembler_.excess();
    if (!compile(_argument)) {
        return false;
    }
    size_type const post_ = assembler_.excess();
    if (!(pre_ < post_)) {
        return false;
    }
    size_type count_ = post_ - pre_;
    while (0 < --count_) {
        if (!assembler_(mnemocode::fucomi, st, st(1),
                        mnemocode::fcmovb, st, st(1),
                        mnemocode::fstp, st(1))) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile_min(ast::rvalues const & _arguments) const
-> result_type
{
    if (_arguments.empty()) {
        return false;
    }
    if (!compile_min(_arguments.front())) {
        return false;
    }
    for (ast::rvalue const & argument_ : tail(_arguments)) {
        if (!compile_min(argument_)) {
            return false;
        }
        if (!assembler_(mnemocode::fucomi, st, st(1),
                        mnemocode::fcmovnbe, st, st(1),
                        mnemocode::fstp, st(1))) {
            return false;
        }
    }
    return true;
}

auto
compiler::compile_min(ast::rvalue const & _argument) const
-> result_type
{
    size_type const pre_ = assembler_.excess();
    if (!compile(_argument)) {
        return false;
    }
    size_type const post_ = assembler_.excess();
    if (!(pre_ < post_)) {
        return false;
    }
    size_type count_ = post_ - pre_;
    while (0 < --count_) {
        if (!assembler_(mnemocode::fucomi, st, st(1),
                        mnemocode::fcmovnbe, st, st(1),
                        mnemocode::fstp, st(1))) {
            return false;
        }
    }
    return true;
}

#if 0
// tanh
// (exp(2 * x) - 1) / (exp(2 * x) + 1)
fldl2e
fadd st, st(0)
fmul x
fld st
frndint
fxch
fsub st, st(1)
f2xm1
fld1
fadd
fscale
fst st(1) // exp(2 * x) exp(2 * x)
fld1
fsub st(2), st // 1 exp(2 * x) exp(2 * x) - 1
fadd // exp(2 * x) + 1 exp(2 * x) - 1
fdiv
// sinh
// (exp(x) - exp(-x)) / 2 == (exp(2 * x) - 1) / (2 * exp(x))
fldl2e
fmul x
fld st
frndint
fxch
fsub st, st(1)
f2xm1
fld1
fadd
fscale
fst st(1)       // exp(x) exp(x)
fmul st(1), st  // exp(x) exp(2 * x)
fadd st, st(0)  // 2 * exp(x) exp(2 * x)
fld1            // 1 2 * exp(x) exp(2 * x)
fsubp st(2), st // 2 * exp(x) exp(2 * x) - 1
fdiv
// cosh
// (exp(x) + exp(-x)) / 2 == (exp(2 * x) + 1) / (2 * exp(x))
fldl2e
fmul x
fld st
frndint
fxch
fsub st, st(1)
f2xm1
fld1
fadd
fscale
fst st(1)       // exp(x) exp(x)
fmul st(1), st  // exp(x) exp(2 * x)
fadd st, st(0)  // 2 * exp(x) exp(2 * x)
fld1            // 1 2 * exp(x) exp(2 * x)
faddp st(2), st // 2 * exp(x) exp(2 * x) + 1
fdiv
// atanh
// ln((x + 1) / (x - 1)) / 2
fldln2
fld x
fld st
fld1 // 1 x x ln2
fadd st(2), st // 1 x x + 1 ln2
fsub // x - 1 x + 1 ln2
fdiv
fyl2x
fld1
fadd st, st(0)
fdiv
// asinh
// ln(sqrt(sqr(x) + 1) + x)
fldln2
fld x
fld st
fmul st, st
fld1
fadd
fsqrt
fadd
fyl2x
// acosh
// ln(sqrt(sqr(x) - 1) + x)
fldln2
fld x
fld st
fmul st, st
fld1
fsub
fsqrt
fadd
fyl2x
#endif

}
}
