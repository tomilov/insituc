#include <insituc/meta/assembler.hpp>

#include <insituc/utility/reverse.hpp>
#include <insituc/utility/numeric/safe_convert.hpp>
#include <insituc/parser/parser.hpp>

#include <cassert>

namespace insituc
{
namespace meta
{

assembler::assembler()
    : monitor_(*this)
{
    dummy_placeholder_.symbol_.name_ = "_";
    for (string_type & reserved_sybmol_ : parser::get_constants()) {
        if (!add_reserved_symbol(std::move(reserved_sybmol_))) {
            assert(false);
        }
    }
    for (string_type & reserved_sybmol_ : parser::get_intrinsics()) {
        if (!add_reserved_symbol(std::move(reserved_sybmol_))) {
            assert(false);
        }
    }
    for (string_type & reserved_sybmol_ : parser::get_keywords()) {
        if (!add_reserved_symbol(std::move(reserved_sybmol_))) {
            assert(false);
        }
    }
}

bool
assembler::is_local_variable(symbol_type const & _symbol) const
{
    auto const end = std::cend(local_variables_);
    return (std::find(std::cbegin(local_variables_), end, _symbol) != end);
}

bool
assembler::is_top_level_local_variable(symbol_type const & _symbol) const
{
    assert(!brackets_.empty());
    size_type const outer_scope_size_ = brackets_.back();
    assert(is_includes< difference_type >(outer_scope_size_));
    auto const beg = std::next(std::cbegin(local_variables_),
                               static_cast< difference_type >(outer_scope_size_));
    auto const end = std::cend(local_variables_);
    return (std::find(beg, end, _symbol) != end);
}

auto
assembler::local_variable_offset(symbol_type const & _symbol) const
-> size_type
{
    size_type offset_ = local_variables_.size();
    for (symbol_type const & local_variable_ : reverse(local_variables_)) {
        --offset_;
        if (local_variable_ == _symbol) {
            return offset_;
        }
    }
    throw std::runtime_error("cannot find local variable");
}

auto
assembler::call(symbol_type const & _symbol)
-> result_type
{
    assert(monitor_.local_variables_count() == local_variables_.size());
    if (is_current_function(_symbol)) { // recursion denied
        return false;
    }
    if (!is_function(_symbol)) { // callee must be defined
        return false;
    }
    size_type const callee_ = export_table_.at(_symbol);
    if (!monitor_(mnemocode::call, callee_, get_function(callee_).climbing_)) {
        return false;
    }
    return true;
}

auto
assembler::memory_rw_access(mnemocode const _mnemocode, symbol_type const & _symbol)
-> result_type
{
    if (is_dummy_placeholder(_symbol)) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
        switch (_mnemocode) {
#pragma clang diagnostic pop
        case mnemocode::fst :
        case mnemocode::fstp : {
            if (!monitor_(_mnemocode, st)) {
                return false;
            }
            return true;
        }
        case mnemocode::fld : {
            if (!monitor_(mnemocode::fldz)) {
                return false;
            }
            return true;
        }
        case mnemocode::fadd :
        case mnemocode::fsub :
        case mnemocode::fsubr : {
            if (!monitor_(mnemocode::fldz)) {
                return false;
            }
            if (!monitor_(_mnemocode)) {
                return false;
            }
            return true;
        }
        case mnemocode::fmul :
        case mnemocode::fdiv :
        case mnemocode::fdivr : {
            if (!monitor_(mnemocode::fld1)) {
                return false;
            }
            if (!monitor_(_mnemocode)) {
                return false;
            }
            return true;
        }
        case mnemocode::fcom : {
            if (!monitor_(mnemocode::ftst)) {
                return false;
            }
            return true;
        }
        case mnemocode::fcomp : {
            if (!monitor_(mnemocode::ftst)) {
                return false;
            }
            if (!monitor_(mnemocode::fstp, st)) {
                return false;
            }
            return true;
        }
        default : {
            break;
        }
        }
        return false;
    } else if (is_local_variable(_symbol)) {
        size_type const offset_ = local_variable_offset(_symbol);
        if (!monitor_(_mnemocode, offset_, memory_layout::stack)) {
            return false;
        }
        return true;
    } else if (is_global_variable(_symbol)) {
        size_type const offset_ = global_varibles_.at(_symbol);
        if (!monitor_(_mnemocode, offset_, memory_layout::heap)) {
            return false;
        }
        return true;
    } else {
        return false;
    }
}

auto
assembler::filter_brackets(mnemocode const _mnemocode)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::bra : {
        size_type const count_ = local_variables_.size();
        assert(monitor_.local_variables_count() == count_);
        brackets_.push_back(count_);
        if (!monitor_(mnemocode::bra, count_)) {
            return false;
        }
        return true;
    }
    case mnemocode::ket : {
        size_type const count_ = local_variables_.size();
        assert(monitor_.local_variables_count() == count_);
        assert(!brackets_.empty());
        size_type const previous_frame_used_ = brackets_.back();
        brackets_.pop_back();
        assert(!(count_ < previous_frame_used_));
        size_type const delta_ = count_ - previous_frame_used_;
        local_variables_.resize(previous_frame_used_);
        assert(!(local_variables_.size() < monitor_.arity()));
        if (!monitor_(mnemocode::ket, delta_)) {
            return false;
        }
        return true;
    }
    case mnemocode::endl : {
        assert(monitor_.local_variables_count() == local_variables_.size());
        break;
    }
    default : {
        break;
    }
    }
    if (!monitor_(_mnemocode)) {
        return false;
    }
    return true;
}

void
assembler::monitor::enter(size_type const _arity, size_type const _input)
{
    assert(!(_arity < _input));
    assert(!(st.depth < _input));
    assert(function_.empty());
    function_.enter(_arity, _input);
    stack_pointer_ = 0;
    arity_ = _arity;
    frame_used_ = function_.frame_clobbered_ = local_variables_count_ = arity_ - _input;
    used_ = function_.clobbered_ = _input;
}

size_type
assembler::monitor::excess() const
{
    assert(!(frame_used_ < local_variables_count_));
    size_type const stack_excess_ = frame_used_ - local_variables_count_;
    assert(!(std::numeric_limits< size_type >::max() - used_ < stack_excess_));
    return stack_excess_ + used_;
}

auto
assembler::monitor::check()
-> result_type
{
    assert(!function_.empty());
    assert(function_.compiled());
    function const reference_ = std::move(function_);
    enter(reference_.arity(), reference_.input_);
    if (!reference_.for_each_instruction(std::ref(*this))) {
        return false;
    }
    leave(reference_.symbol_, reference_.arguments_);
    if (!(function_ == reference_)) {
        return false;
    }
    return true;
}

auto
assembler::monitor::access_top(mnemocode const _mnemocode, size_type const _offset)
-> result_type
{
    if (!is_valid_offset(_offset)) {
        return false;
    }
    if (is_near_offset(_offset)) {
        return operator () (_mnemocode, _offset);
    } else {
        size_type const offset_ = get_far_offset(_offset);
        return operator () (_mnemocode, offset_, memory_layout::stack);
    }
}

auto
assembler::monitor::verify(mnemocode const _mnemocode)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fninit : {
        return adjust(used_, 0);
    }
    case mnemocode::ud2 : { // legal
        return true;
    }
    case mnemocode::fnop :
    case mnemocode::fwait : {
        return true;
    }
    case mnemocode::fnstsw : {
        return true;
    }
    case mnemocode::fdecstp :
    case mnemocode::fincstp : {
        return true;
    }
    case mnemocode::fxam : { // FXAM can operate on empty ST(0)
        return true;
    }
    case mnemocode::ftst :
    case mnemocode::fabs :
    case mnemocode::fchs :
    case mnemocode::frndint :
    case mnemocode::trunc :
    case mnemocode::fsqrt :
    case mnemocode::fcos :
    case mnemocode::fsin :
    case mnemocode::f2xm1 : {
        return adjust(1);
    }
    case mnemocode::fldz :
    case mnemocode::fld1 :
    case mnemocode::fldpi :
    case mnemocode::fldl2e :
    case mnemocode::fldl2t :
    case mnemocode::fldlg2 :
    case mnemocode::fldln2 : {
        return adjust(0, 1);
    }
    case mnemocode::fxch :
    case mnemocode::fcom :
    case mnemocode::fucom :
    case mnemocode::fscale :
    case mnemocode::fprem :
    case mnemocode::fprem1 : {
        return adjust(2);
    }
    case mnemocode::fcompp :
    case mnemocode::fucompp : {
        return adjust(2, 0);
    }
    case mnemocode::fcomp :
    case mnemocode::fucomp :
    case mnemocode::fadd :
    case mnemocode::fsub :
    case mnemocode::fsubr :
    case mnemocode::fmul :
    case mnemocode::fdiv :
    case mnemocode::fdivr :
    case mnemocode::fpatan :
    case mnemocode::fyl2x :
    case mnemocode::fyl2xp1 : {
        return adjust(2, 1);
    }
    case mnemocode::fptan :
    case mnemocode::fsincos :
    case mnemocode::fxtract : {
        return adjust(1, 2);
    }
    case mnemocode::sahf : {
        return true;
    }
    case mnemocode::endl : {
        assert(frame_used_ == local_variables_count_);
        assert(used_ == 0);
        return true;
    }
    default : {
        break;
    }
    }
    return false;
}

auto
assembler::monitor::verify(mnemocode const _mnemocode,
                           size_type const _operand)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fst : {
        if (_operand == 0) {
            return adjust(1);
        } else if (_operand == used_) {
            return adjust(used_, _operand + 1);
        } else if (_operand < used_) {
            return adjust(_operand + 1);
        } else {
            return false;
        }
    }
    case mnemocode::fstp : {
        if (_operand == 0) {
            return adjust(1, 0);
        } else if (_operand == used_) {
            return adjust(used_);
        } else if (_operand < used_) {
            return adjust(used_, used_ - 1);
        } else {
            return false;
        }
    }
    case mnemocode::fld : {
        if (_operand + 1 == st.depth) {
            return false; // attemption to overflow the floating-point stack or to read the value of an empty floating-point register
        }
        return adjust(_operand + 1, _operand + 2);
    }
    case mnemocode::fcom :
    case mnemocode::fucom : {
        return adjust(_operand + 1);
    }
    case mnemocode::fcomp :
    case mnemocode::fucomp : {
        return adjust(_operand + 1, _operand);
    }
    case mnemocode::ffree : {
        if (_operand + 1 != used_) {
            return false;
        }
        return adjust(_operand + 1, _operand);
    }
    case mnemocode::ffreep : {
        if (_operand + 1 != used_) {
            return false;
        }
        return adjust(_operand + 1, _operand - 1);
    }
    case mnemocode::fxch : {
        return adjust(_operand + 1);
    }
    case mnemocode::sp_inc : {
        stack_pointer_ += _operand;
        assert(!(frame_used_ < stack_pointer_));
        return true;
    }
    case mnemocode::sp_dec : {
        assert(!(stack_pointer_ < _operand));
        stack_pointer_ -= _operand;
        return true;
    }
    case mnemocode::bra : {
        assert(used_ == 0);
        assert(frame_used_ == local_variables_count_);
        assert(_operand == local_variables_count_);
        return true;
    }
    case mnemocode::ket : {
        assert(used_ == 0);
        assert(frame_used_ == local_variables_count_);
        assert(!(frame_used_ < _operand));
        assert(!(local_variables_count_ < _operand));
        frame_used_ -= _operand;
        local_variables_count_ = frame_used_;
        return true;
    }
    case mnemocode::endl : {
        assert(used_ == _operand);
        assert(frame_used_ == local_variables_count_);
        return true;
    }
    default : {
        break;
    }
    }
    return false;
}

auto
assembler::monitor::verify(mnemocode const _mnemocode,
                           size_type const _destination,
                           size_type const _source)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::call : {
        assert(_destination < assembler_.functions_.size());
        function_.callies_.insert(_destination);
        function const & callee_ = assembler_.get_function(_destination);
        return adjust(callee_);
    }
    case mnemocode::fcmovb :
    case mnemocode::fcmove :
    case mnemocode::fcmovbe :
    case mnemocode::fcmovu :
    case mnemocode::fcmovnb :
    case mnemocode::fcmovne :
    case mnemocode::fcmovnbe :
    case mnemocode::fcmovnu :
    case mnemocode::fcomi :
    case mnemocode::fucomi : {
        if (0 != _destination) {
            return false;
        }
        return adjust(_source + 1);
    }
    case mnemocode::fcomip :
    case mnemocode::fucomip : {
        if (0 != _destination) {
            return false;
        }
        return adjust(_source + 1, _source);
    }
    case mnemocode::fadd :
    case mnemocode::fsub :
    case mnemocode::fmul :
    case mnemocode::fdiv :
    case mnemocode::fsubr :
    case mnemocode::fdivr : {
        if ((0 != _destination) && (0 != _source)) {
            return false;
        }
        return adjust(((0 != _destination) ? _destination : _source) + 1);
    }
    case mnemocode::faddp :
    case mnemocode::fsubp :
    case mnemocode::fsubrp :
    case mnemocode::fmulp :
    case mnemocode::fdivp :
    case mnemocode::fdivrp : {
        if (0 != _source) {
            return false;
        }
        return adjust(_destination + 1, _destination);
    }
    case mnemocode::fld : {
        assert(!(st.depth < _source));
        assert(!(frame_used_ < _destination));
        assert(_source == frame_used_ - _destination);
        frame_used_ = _destination;
#ifndef NDEBUG
        size_type const frame_used_notch_ = frame_used_;
#endif
        if (!adjust(0, _source)) {
            return false;
        }
        assert(frame_used_notch_ == frame_used_);
        return true;
    }
    case mnemocode::fstp : {
        assert(!(st.depth < _source));
        assert(!(_destination < frame_used_));
        assert(_source == _destination - frame_used_);
        frame_used_ = _destination;
#ifndef NDEBUG
        size_type const frame_used_notch_ = frame_used_;
#endif
        if (!adjust(_source, 0)) {
            return false;
        }
        assert(frame_used_notch_ == frame_used_);
        return true;
    }
    case mnemocode::ret : {
        // TODO: use _source as ast::rvalue_list::drop_
        if (0 != function_.output_) {
            return false;
        }
        assert(0 == function_.climbing_); // only one return statement allowed in whole function body
        function_.output_ = _destination;
        function_.climbing_ = stack_pointer_;
        assert(0 != _destination);
        assert(!(st.depth < _destination));
        assert(!(_destination < used_));
        assert(excess() == _destination);
        result_type adjusted = adjust(_destination, 0);
        assert(frame_used_ == local_variables_count_);
        assert(used_ == 0);
        return adjusted;
    }
    default : {
        break;
    }
    }
    return false;
}

auto
assembler::monitor::verify(mnemocode const _mnemocode,
                           size_type const _offset,
                           memory_layout const _memory_layout)
-> result_type
{
    switch (_memory_layout) {
    case memory_layout::heap : {
        return verify_heap_access(_mnemocode, _offset);
    }
    case memory_layout::stack : {
        return verify_stack_access(_mnemocode, _offset);
    }
    }
    return false;
}

auto
assembler::monitor::verify_heap_access(mnemocode const _mnemocode, size_type const _offset)
-> result_type
{
    static_cast< void >(_offset);
    return verify_memory_access(_mnemocode);
}

auto
assembler::monitor::verify_stack_access(mnemocode const _mnemocode, size_type const _offset)
-> result_type
{
    if (mnemocode::alloca_ == _mnemocode) {
        assert(local_variables_count_ == frame_used_);
        assert(_offset == frame_used_);
        static_cast< void >(_offset);
        if (!verify_memory_access(mnemocode::fstp)) {
            return false;
        }
        ++local_variables_count_;
        ++frame_used_;
        return update_stack_depth();
    } else {
        return verify_memory_access(_mnemocode);
    }
}

auto
assembler::monitor::verify_memory_access(mnemocode const _mnemocode)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fst : {
        if (use_long_double) {
            return false;
        }
        return adjust(1);
    }
    case mnemocode::fstp : {
        return adjust(1, 0);
    }
    case mnemocode::fld : {
        return adjust(0, 1);
    }
    case mnemocode::fadd :
    case mnemocode::fsub :
    case mnemocode::fsubr :
    case mnemocode::fmul :
    case mnemocode::fdiv :
    case mnemocode::fdivr :
    case mnemocode::fcom : {
        if (use_long_double) {
            return false;
        }
        return adjust(1);
    }
    case mnemocode::fcomp : {
        if (use_long_double) {
            return false;
        }
        return adjust(1, 0);
    }
    default : {
        break;
    }
    }
    return false;
}

auto
assembler::monitor::update_stack_depth(size_type const _additional)
-> result_type
{
    assert(!(std::numeric_limits< size_type >::max() - frame_used_ < _additional));
    size_type const frame_clobbered_(frame_used_ + _additional);
    if (function_.frame_clobbered_ < frame_clobbered_) {
        function_.frame_clobbered_ = frame_clobbered_;
    }
    return true;
}

auto
assembler::monitor::adjust_stack_pointer(function const & _callee)
-> result_type
{
    size_type const frame_consumed_ = _callee.arity() - _callee.input_;
    assert(!(frame_used_ < frame_consumed_));
    assert(!(frame_used_ - frame_consumed_ < local_variables_count_));
    frame_used_ -= frame_consumed_;
    if (stack_pointer_ < frame_used_) {
        if (!operator () (mnemocode::sp_inc, frame_used_ - stack_pointer_)) {
            return false;
        }
    } else if (frame_used_ < stack_pointer_) {
        if (!operator () (mnemocode::sp_dec, stack_pointer_ - frame_used_)) {
            return false;
        }
    }
    assert(stack_pointer_ == frame_used_);
    assert(!(std::numeric_limits< size_type >::max() - stack_pointer_ < _callee.climbing_));
    stack_pointer_ += _callee.climbing_;
    return update_stack_depth(_callee.frame_clobbered_);
}

auto
assembler::monitor::adjust(size_type const _consumed,
                           size_type const _produced,
                           size_type const _clobbered)
-> result_type
{
    assert(!(st.depth < _clobbered));
    assert(!(_clobbered < _consumed));
    assert(!(_clobbered < _produced));
    size_type const additional_(_clobbered - _consumed);
    size_type const unused_(st.depth - used_);
    bool const excess_(unused_ < additional_);
    bool const deficiency_(used_ < _consumed);
    if (excess_ != deficiency_) {
        if (deficiency_) {
            if (st.depth / 2 < _consumed) {
                for (size_type i = _consumed; i < st.depth; ++i) {
                    if (!operator () (mnemocode::fdecstp)) {
                        return false;
                    }
                }
            } else {
                for (size_type i = 0; i < _consumed; ++i) {
                    if (!operator () (mnemocode::fincstp)) {
                        return false;
                    }
                }
            }
            size_type const delta_(_consumed - used_);
            assert(!(frame_used_ < delta_));
            assert(!(frame_used_ - delta_ < arity_));
            if (!operator () (mnemocode::fld, frame_used_ - delta_, delta_)) {
                return false; // from destination to destination + delta_ : destination++
            }
        } else if (excess_) {
            if (st.depth / 2 < additional_) {
                for (size_type i = additional_; i < st.depth; ++i) {
                    if (!operator () (mnemocode::fincstp)) {
                        return false;
                    }
                }
            } else {
                for (size_type i = 0; i < additional_; ++i) {
                    if (!operator () (mnemocode::fdecstp)) {
                        return false;
                    }
                }
            }
            size_type const delta_ = additional_ - unused_;
            assert(!(std::numeric_limits< size_type >::max() - frame_used_ < delta_));
            if (!operator () (mnemocode::fstp, frame_used_ + delta_, delta_)) {
                return false; // from destination to destination - delta_ : --destination
            }
        } else {
            assert(false);
            return false;
        }
        if (0 != used_) {
            if (st.depth / 2 < unused_) {
                for (size_type i = unused_; i < st.depth; ++i) {
                    if (!operator () (mnemocode::fdecstp)) {
                        return false;
                    }
                }
            } else {
                for (size_type i = 0; i < unused_; ++i) {
                    if (!operator () (mnemocode::fincstp)) {
                        return false;
                    }
                }
            }
        }
    } else if (excess_ && deficiency_) { // <=> st.depth < _clobbered // floating-point stack is too small to maintain this operation entirely (try to split into smaller subsequences)
        return false;
    }
    assert(!(used_ < _consumed));
    used_ -= _consumed;
    {
        size_type const involved_ = used_ + _clobbered;
        assert(!(st.depth < involved_));
        if (function_.clobbered_ < involved_) {
            function_.clobbered_ = involved_;
        }
    }
    assert(!(st.depth - used_ < _produced));
    used_ += _produced;
    return update_stack_depth();
}

}
}
