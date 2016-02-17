#include <insituc/runtime/interpreter/virtual_machine.hpp>

#include <insituc/variant.hpp>
#include <boost/math/constants/constants.hpp>

#include <experimental/optional>
#include <type_traits>
#include <utility>

#include <cmath>
#include <cassert>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
namespace insituc
{
namespace runtime
{

auto
virtual_machine::check_head_tail(meta::function const & _function, size_type const _head_size) const
-> result_type
{
    assert(!(_function.clobbered_ < _head_size));
    auto const top = std::cbegin(fpregs_);
    assert(is_includes< difference_type >(_head_size));
    auto const bottom = std::next(top, static_cast< difference_type >(_head_size));
    if (std::any_of(top, bottom, std::logical_not< fpreg_type >())) {
        return false;
    }
    if (_function.frameless()) {
        size_type const additional_ = _function.clobbered_ - _head_size;
        auto const first = std::crbegin(fpregs_);
        assert(is_includes< difference_type >(additional_));
        auto const last = std::next(first, static_cast< difference_type >(additional_));
        if (!std::all_of(first, last, std::logical_not< fpreg_type >())) {
            return false;
        }
    } else {
        if (!std::all_of(bottom, std::cend(fpregs_), std::logical_not< fpreg_type >())) {
            return false;
        }
    }
    return true;
}

auto
virtual_machine::interpret_function(meta::function const & _function)
-> result_type
{
    assert(_function.compiled());
    assert(!(st.depth < _function.clobbered_));
    assert(check_head_tail(_function, _function.input_));
    assert(_function.frame_clobbered_ < std::numeric_limits< size_type >::max() - stack_pointer_);
    assert(!(stack_.size() < stack_pointer_ + _function.frame_clobbered_));
    frame_stack_.push(frame_pointer_);
    frame_pointer_ = stack_pointer_;
#ifndef NDEBUG
    size_type const stack_input_ = _function.arity() - _function.input_;
#endif
    assert(!(stack_used_ < frame_pointer_));
    assert(stack_used_ - frame_pointer_ == stack_input_);
    output_ = 0;
    call_stack_.push(_function);
    if (!_function.for_each_instruction(visit([&] (auto const & i) -> result_type { return interpret(i); }))) {
        return false;
    }
    call_stack_.pop();
    assert(output_ == _function.output_);
    assert(!(stack_used_ < frame_pointer_));
    assert(stack_used_ - frame_pointer_ == stack_input_);
    assert(!(stack_pointer_ < frame_pointer_));
    assert(stack_pointer_ - frame_pointer_ == _function.climbing_);
    stack_used_ = frame_pointer_;
    assert(!frame_stack_.empty());
    frame_pointer_ = frame_stack_.top();
    frame_stack_.pop();
    assert(check_head_tail(_function, _function.output_));
    return true;
}

auto
virtual_machine::fxam()
-> result_type
{
    if (!fpregs_.front()) {
        set_condition_codes(true, false, true);
    }
    G const & top_ = *fpregs_.front();
    C1_ = signbit(top_);
    switch (fpclassify(top_)) {
    case FP_INFINITE  : set_condition_codes(true,  true,  false); break;
    case FP_NAN       : set_condition_codes(true,  false, false); break;
    case FP_NORMAL    : set_condition_codes(false, true,  false); break;
    case FP_SUBNORMAL : set_condition_codes(false, true,  true ); break;
    case FP_ZERO      : set_condition_codes(false, false, true ); break;
    default : {
        set_condition_codes(false, false, false); // unsupported
        break;
    }
    }
    return true;
}

auto
virtual_machine::fcom(mnemocode const _mnemocode,
                      fpreg_type const & _destination,
                      fpreg_type const & _source)
-> result_type
{
    set_condition_codes(true, true, true);
    if (!_destination || !_source) {
        return false;
    }
    G const & source_ = *_source;
    if (isnan(source_)) {
        return false;
    }
    G const & destination_ = *_destination;
    if (isnan(destination_)) {
        return false;
    }

    if (isunordered(destination_, source_)) {
        set_condition_codes(true, true, true);
    } else if (isgreater(destination_, source_)) {
        set_condition_codes(false, false, false);
    } else if (isless(destination_, source_)) {
        set_condition_codes(true, false, false);
    } else if (!islessgreater(destination_, source_)) { // is it correct to use islessgreater?
        set_condition_codes(false, false, true);
    } else {
        return false;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fcompp :
    case mnemocode::fucompp : {
        if (!fpop()) {
            return false;
        }
        if (!fpop()) {
            return false;
        }
        break;
    }
    case mnemocode::fcomp :
    case mnemocode::fucomp :
    case mnemocode::fcomip :
    case mnemocode::fucomip :{
        if (!fpop()) {
            return false;
        }
        break;
    }
    case mnemocode::ftst :
    case mnemocode::fcom :
    case mnemocode::fucom :
    case mnemocode::fcomi :
    case mnemocode::fucomi : {
        break;
    }
    default : {
        return false;
    }
    }
    return true;
}

auto
virtual_machine::favoid(G const & _top) const // return true;
-> result_type
{
    if (isnan(_top)) {
        return false; // The values of NANs, +INFINITY and -0 remain unchanged without any exception being detected.
    } else if (isinf(_top)) {
        return false; // The values of NANs, +INFINITY and -0 remain unchanged without any exception being detected.
    } else {
        return true;
    }
}

auto
virtual_machine::fcircular(G const & _top)
-> result_type
{
    if (isinf(_top)) {
        return false;
    }
    // If the source angle value is outside the acceptable range (but not INFINITY), the C2 flag of the Status Word is set to 1 and the content of all data registers remains unchanged; the TOP register field of the Status Word is not modified and no exception is detected.
    C2_ = (std::is_floating_point_v< G > && isgreater(abs(_top), exp2(G(63))));
    return !C2_; // need to perform reduction
}

auto
virtual_machine::fconst(mnemocode const _mnemocode)
-> result_type
{
    using boost::math::constants::ln_ten;
    using boost::math::constants::ln_two;
    using boost::math::constants::log10_e;
    using boost::math::constants::pi;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fldz   : return fpush(zero);
    case mnemocode::fld1   : return fpush(one);
    case mnemocode::fldpi  : return fpush(pi< G >());
    case mnemocode::fldl2e : return fpush((one / ln_two< G >()));
    case mnemocode::fldl2t : return fpush((ln_ten< G >() / ln_two< G >()));
    case mnemocode::fldlg2 : return fpush((ln_two< G >() * log10_e< G >()));
    case mnemocode::fldln2 : return fpush(ln_two< G >());
    default : {
        break;
    }
    }
    return false;
}

auto
virtual_machine::fpremcheck(G const & _destination,
                            G const & _source)
-> result_type
{
    C2_ = true; // partial remainder
    if (isinf(_destination)) { // inf mod any
        return false; // Indicates floating-point invalid-arithmetic-operand (#IA) exception.
    } else if (_source == zero) {
        if (_destination == zero) { // zero mod zero
            return false; // Indicates floating-point invalid-arithmetic-operand (#IA) exception.
        } else { // finite mod zero
            return false; // Indicates floating-point zero-divide (#Z) exception.
        }
    }
    C2_ = false; // reduction complete
    return true;
}

auto
virtual_machine::fsubcheck(G const & _destination,
                           G const & _source) const
-> result_type
{
    if (isinf(_source)) {
        if (isinf(_destination)) {
            if (signbit(_source) == signbit(_destination)) {
                return false;
            }
        }
    }
    return true;
}

auto
virtual_machine::fdivcheck(G const & _destination,
                           G const & _source) const
-> result_type
{
    if (isinf(_destination)) {
        if (isinf(_source)) {
            return false;
        }
    } else if (_source == zero) {
        if (_destination == zero) {
            return false;
        }
    }
    return true;
}

auto
virtual_machine::fxch(fpreg_type & _destination)
-> result_type
{
    fpreg_type & top_ = fpregs_.front();
    bool const s = !!top_;
    bool const d = !!_destination;
    using std::swap;
    if (!s || !d) {
        if (!s) {
            top_ = indefinite_;
        }
        if (!d) {
            _destination = indefinite_;
        }
        swap(_destination, top_);
        return false;
    } else {
        swap(*_destination, *top_);
        return true;
    }
}

auto
virtual_machine::fpush(G const & _value)
-> result_type
{
    if (!!fpregs_.back()) {
        return false; // FPU stack overflow
    }
    fpregs_.pop_back();
    fpregs_.push_front(_value);
    return true;
}

auto
virtual_machine::fpush(G && _value)
-> result_type
{
    if (!!fpregs_.back()) {
        return false; // FPU stack overflow
    }
    fpregs_.pop_back();
    fpregs_.push_front(std::move(_value));
    return true;
}

auto
virtual_machine::fpop()
-> result_type
{
    if (!fpregs_.front()) {
        return false; // FPU stack underflow
    }
    fpregs_.pop_front();
    fpregs_.push_back(std::experimental::nullopt);
    return true;
}

auto
virtual_machine::fnullary(mnemocode const _mnemocode)
-> result_type
{
    if (!fpregs_.front()) {
        return false;
    }
    G & top_ = *fpregs_.front();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fabs : {
        top_ = abs(std::move(top_));
        break;
    }
    case mnemocode::fchs : {
        top_ = -std::move(top_);
        break;
    }
    case mnemocode::frndint : {
        if (!favoid(top_)) {
            return true;
        }
        top_ = nearbyint(std::move(top_));
        break;
    }
    case mnemocode::trunc : {
        if (!favoid(top_)) {
            return true;
        }
        top_ = trunc(std::move(top_));
        break;
    }
    case mnemocode::fsqrt : {
        if (!favoid(top_)) {
            return true;
        }
        top_ = sqrt(std::move(top_));
        break;
    }
    case mnemocode::fcos : {
        if (!fcircular(top_)) {
            return false;
        }
        top_ = cos(std::move(top_));
        break;
    }
    case mnemocode::fsin : {
        if (!fcircular(top_)) {
            return false;
        }
        top_ = sin(std::move(top_));
        break;
    }
    case mnemocode::f2xm1 : {
        if (isgreater(abs(top_), one)) {
            // The content of ST(0) must be within the range of -1.0 to +1.0; if it is outside the acceptable range, the result is undefined but no exception is reported.
        }
        top_ = (exp2(std::move(top_)) - one);
        break;
    }
    case mnemocode::fptan : {
        if (!fcircular(top_)) {
            return false;
        }
        top_ = tan(std::move(top_));
        if (!fpush(one)) {
            return false;
        }
        break;
    }
    case mnemocode::fsincos : {
        if (!fcircular(top_)) {
            return false;
        }
        G cos_ = cos(top_);
        top_ = sin(std::move(top_));
        if (!fpush(std::move(cos_))) {
            return false;
        }
        break;
    }
    case mnemocode::fxtract : {
        if (top_ == zero) {
            return false; // A Zero divide exception is detected when the content of ST(0) is 0.
        }
        G exponent_ = logb(abs(top_));
        G significand_ = std::move(top_) / exp2(exponent_);
        top_ = std::move(exponent_);
        if (!fpush(std::move(significand_))) {
            return false;
        }
        break;
    }
    default : {
        return false;
    }
    }
    return true;
}

auto
virtual_machine::fcmov(mnemocode const _mnemocode,
                       fpreg_type & _destination,
                       fpreg_type const & _source)
-> result_type
{
    if (!_source) {
        return false;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fcmovb : {
        if (C0_) { // CF
            break;
        }
        return true;
    }
    case mnemocode::fcmove : {
        if (C3_) { // ZF
            break;
        }
        return true;
    }
    case mnemocode::fcmovbe : {
        if (C0_ || C3_) { // CF or ZF
            break;
        }
        return true;
    }
    case mnemocode::fcmovu : {
        if (C2_) { // PF
            break;
        }
        return true;
    }
    case mnemocode::fcmovnb : {
        if (!C0_) { // CF = 0
            break;
        }
        return true;
    }
    case mnemocode::fcmovne : {
        if (!C3_) { // ZF = 0
            break;
        }
        return true;
    }
    case mnemocode::fcmovnbe : {
        if (!C0_ && !C3_) { // CF = 0 and ZF = 0
            break;
        }
        return true;
    }
    case mnemocode::fcmovnu : {
        if (!C2_) { // !PF
            break;
        }
        return true;
    }
    default : {
        return false;
    }
    }
    _destination = _source;
    return true;
}

auto
virtual_machine::fbinary(mnemocode const _mnemocode,
                         fpreg_type & _destination,
                         fpreg_type const & _source)
-> result_type
{
    if (!_destination || !_source) {
        return false;
    }
    G const & source_ = *_source;
    if (isnan(source_)) {
        return false;
    }
    G & destination_ = *_destination;
    if (isnan(destination_)) {
        return false;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fscale : {
        if (isinf(source_)) {
            if (signbit(source_)) {
                if (isinf(destination_)) {
                    return false; // inf * 2^(-inf)
                }
            } else {
                if (destination_ == zero) {
                    return false; // 0 * 2^(+inf)
                }
            }
        }
        destination_ *= exp2(trunc(source_));
        break;
    }
    case mnemocode::fprem : {
        if (!fpremcheck(destination_, source_)) {
            return false;
        }
        destination_ = fmod(std::move(destination_), source_); // remainder
        break;
    }
    case mnemocode::fprem1 : {
        if (!fpremcheck(destination_, source_)) {
            return false;
        }
        destination_ = remainder(std::move(destination_), source_); // remainder
        break;
    }
    case mnemocode::fadd :
    case mnemocode::faddp : {
        if (isinf(source_)) {
            if (isinf(destination_)) {
                if (signbit(source_) != signbit(destination_)) {
                    return false;
                }
            }
        }
        destination_ += source_;
        break;
    }
    case mnemocode::fsub :
    case mnemocode::fsubp : {
        if (!fsubcheck(destination_, source_)) {
            return false;
        }
        destination_ -= source_;
        break;
    }
    case mnemocode::fsubr :
    case mnemocode::fsubrp : {
        if (!fsubcheck(source_, destination_)) {
            return false;
        }
        destination_ = (source_ - std::move(destination_));
        break;
    }
    case mnemocode::fmul :
    case mnemocode::fmulp : {
        if (isinf(source_)) {
            if (destination_ == zero) {
                return false;
            }
        } else if (source_ == zero) {
            if (isinf(destination_)) {
                return false;
            }
        }
        destination_ *= source_;
        break;
    }
    case mnemocode::fdiv :
    case mnemocode::fdivp : {
        if (!fdivcheck(destination_, source_)) {
            return false;
        }
        destination_ /= source_;
        break;
    }
    case mnemocode::fdivr :
    case mnemocode::fdivrp : {
        if (!fdivcheck(source_, destination_)) {
            return false;
        }
        destination_ = (source_ / std::move(destination_));
        break;
    }
    case mnemocode::fpatan : {
        destination_ = atan2(std::move(destination_), source_);
        break;
    }
    case mnemocode::fyl2x : {
        if ((source_ == zero) && !isinf(destination_)) {
            return false;
        } else if (signbit(source_)) {
            return false;
        } else if ((source_ == one) && isinf(destination_)) {
            return false;
        } else if ((destination_ == zero) && isinf(source_)) {
            return false;
        }
        destination_ *= log2(source_);
        break;
    }
    case mnemocode::fyl2xp1 : {
        //G const bound_ = one - sqrt(G(2)) / G(2);
        if ((source_ == zero) && isinf(destination_)) {
            return false;
        }
        using boost::math::constants::ln_two;
        destination_ *= (log1p(source_) / ln_two< G >());
        break;
    }
    default : {
        return false;
    }
    }
    return true;
}

auto
virtual_machine::interpret(mnemocode const _mnemocode)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fninit : {
        set_condition_codes(false, false, false);
        std::fill_n(std::begin(fpregs_), st.depth, std::experimental::nullopt);
        return true;
    }
    case mnemocode::ud2 : {
        return false;
    }
    case mnemocode::fnop :
    case mnemocode::fwait :
    case mnemocode::fnstsw : {
        return true;
    }
    case mnemocode::fdecstp : {
        auto const end = std::end(fpregs_);
        std::rotate(std::begin(fpregs_), std::prev(end), end) ;
        break;
    }
    case mnemocode::fincstp : {
        auto const beg = std::begin(fpregs_);
        std::rotate(beg, std::next(beg), std::end(fpregs_));
        break;
    }
    case mnemocode::fxam : {
        if (!fxam()) {
            return false;
        }
        break;
    }
    case mnemocode::ftst : { // <=> comparison against zero
        if (!fcom(mnemocode::ftst, fpregs_.front(), zero)) {
            return false;
        }
        break;
    }
    case mnemocode::fabs :
    case mnemocode::fchs :
    case mnemocode::frndint :
    case mnemocode::trunc :
    case mnemocode::fsqrt :
    case mnemocode::fcos :
    case mnemocode::fsin :
    case mnemocode::f2xm1 : {
        if (!fnullary(_mnemocode)) {
            fpregs_.front() = indefinite_;
            return false;
        }
        break;
    }
    case mnemocode::fldz :
    case mnemocode::fld1 :
    case mnemocode::fldpi :
    case mnemocode::fldl2e :
    case mnemocode::fldl2t :
    case mnemocode::fldlg2 :
    case mnemocode::fldln2 : {
        if (!fconst(_mnemocode)) {
            if (!fpush(indefinite_)) {
                return false;
            }
            return false;
        }
        break;
    }
    case mnemocode::fxch : {
        if (!fxch(fpregs_.at(1))) {
            return false;
        }
        break;
    }
    case mnemocode::fscale :
    case mnemocode::fprem :
    case mnemocode::fprem1 : {
        if (!fbinary(_mnemocode, fpregs_.front(), fpregs_.at(1))) {
            fpregs_.front() = indefinite_;
            return false;
        }
        break;
    }
    case mnemocode::fcom :
    case mnemocode::fucom :
    case mnemocode::fcomp :
    case mnemocode::fucomp :
    case mnemocode::fcompp :
    case mnemocode::fucompp : {
        if (!fcom(_mnemocode, fpregs_.front(), fpregs_.at(1))) {
            return false;
        }
        break;
    }
    case mnemocode::fadd :
    case mnemocode::fsub :
    case mnemocode::fsubr :
    case mnemocode::fmul :
    case mnemocode::fdiv :
    case mnemocode::fdivr :
    case mnemocode::fpatan :
    case mnemocode::fyl2x :
    case mnemocode::fyl2xp1 : {
        if (!fbinary(_mnemocode, fpregs_.at(1), fpregs_.front())) {
            fpregs_.at(1) = indefinite_;
            return false;
        }
        if (!fpop()) {
            return false;
        }
        break;
    }
    case mnemocode::fptan :
    case mnemocode::fsincos :
    case mnemocode::fxtract : {
        if (!fnullary(_mnemocode)) {
            if (!fpush(indefinite_)) {
                return false;
            }
            return false;
        }
        break;
    }
    case mnemocode::sahf :
    case mnemocode::endl : {
        return true;
    }
    default : {
        return false;
    }
    }
    return true;
}

auto
virtual_machine::interpret(mnemocode const _mnemocode,
                           size_type const _operand)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fst : {
        if (!fpregs_.front()) {
            fpregs_.at(_operand) = indefinite_;
            return false;
        }
        fpregs_.at(_operand) = fpregs_.front();
        break;
    }
    case mnemocode::fstp : {
        if (!fpregs_.front()) {
            fpregs_.at(_operand) = indefinite_;
            return false;
        }
        fpregs_.at(_operand) = std::move(fpregs_.front());
        if (!fpop()) {
            return false;
        }
        break;
    }
    case mnemocode::fld : {
        fpreg_type const & operand_ = fpregs_.at(_operand);
        if (!operand_) {
            if (!fpush(indefinite_)) {
                return false;
            }
            return false;
        }
        if (!fpush(*operand_)) {
            return false;
        }
        break;
    }
    case mnemocode::fcom :
    case mnemocode::fucom :
    case mnemocode::fcomp :
    case mnemocode::fucomp : {
        if (!fcom(_mnemocode, fpregs_.front(), fpregs_.at(_operand))) {
            return false;
        }
        break;
    }
    case mnemocode::ffree : {
        fpregs_.at(_operand) = std::experimental::nullopt;
        break;
    }
    case mnemocode::ffreep : {
        fpregs_.at(_operand) = std::experimental::nullopt;
        if (!fpop()) {
            return false;
        }
        break;
    }
    case mnemocode::fxch : {
        if (!fxch(fpregs_.at(_operand))) {
            return false;
        }
        break;
    }
    case mnemocode::sp_inc : {
        assert(!(stack_.size() < _operand));
        assert(!(stack_.size() - _operand < stack_pointer_));
        stack_pointer_ += _operand;
        assert(!(stack_used_ < stack_pointer_));
        return true;
    }
    case mnemocode::sp_dec : {
        assert(!(stack_pointer_ < frame_pointer_));
        assert(!(stack_pointer_ - frame_pointer_ < _operand));
        stack_pointer_ -= _operand;
        assert(!(stack_used_ < stack_pointer_));
        return true;
    }
    case mnemocode::bra : {
        assert(!(stack_used_ < frame_pointer_));
        assert(stack_used_ - frame_pointer_ == _operand);
        assert(check_head_tail(call_stack_.top(), output_));
        return true;
    }
    case mnemocode::ket : {
        assert(!(stack_used_ < frame_pointer_));
        assert(!(stack_used_ - frame_pointer_ < _operand));
        stack_used_ -= _operand;
        assert(check_head_tail(call_stack_.top(), output_));
        return true;
    }
    case mnemocode::endl : {
        assert(check_head_tail(call_stack_.top(), _operand));
        return true;
    }
    default : {
        return false;
    }
    }
    return true;
}

auto
virtual_machine::interpret(mnemocode const _mnemocode,
                           size_type const _destination,
                           size_type const _source)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::call : {
        assert(_destination < assembler_.get_export_table().size());
        meta::function const & callee_ = assembler_.get_function(_destination);
        if (!interpret_function(callee_)) {
            return false;
        }
        break;
    }
    case mnemocode::fcmovb :
    case mnemocode::fcmove :
    case mnemocode::fcmovbe :
    case mnemocode::fcmovu :
    case mnemocode::fcmovnb :
    case mnemocode::fcmovne :
    case mnemocode::fcmovnbe :
    case mnemocode::fcmovnu : {
        if (0 != _destination) {
            return false;
        }
        if (!fcmov(_mnemocode, fpregs_.front(), fpregs_.at(_source))) {
            fpregs_.front() = indefinite_;
            return false;
        }
        break;
    }
    case mnemocode::fcomi :
    case mnemocode::fucomi :
    case mnemocode::fcomip :
    case mnemocode::fucomip : {
        if (0 != _destination) {
            return false;
        }
        if (!fcom(_mnemocode, fpregs_.front(), fpregs_.at(_source))) {
            return false;
        }
        break;
    }
    case mnemocode::fadd :
    case mnemocode::fsub :
    case mnemocode::fsubr :
    case mnemocode::fmul :
    case mnemocode::fdiv :
    case mnemocode::fdivr : {
        if ((0 != _destination) && (0 != _source)) {
            return false;
        }
        if (!fbinary(_mnemocode, fpregs_.at(_destination), fpregs_.at(_source))) {
            return false;
        }
        break;
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
        if (!fbinary(_mnemocode, fpregs_.at(_destination), fpregs_.at(_source))) {
            return false;
        }
        if (!fpop()) {
            return false;
        }
        break;
    }
    case mnemocode::fld : {
        assert(!(st.depth < _source));
        assert(!(stack_used_ < frame_pointer_));
        assert(!(stack_used_ - frame_pointer_ < _destination));
        assert(_source == (stack_used_ - frame_pointer_) - _destination); // frame_used_
        size_type position_ = _destination;
        for (size_type i = 0; i < _source; ++i) {
            if (!interpret(mnemocode::fld, position_++, meta::memory_layout::stack)) {
                return false;
            }
        }
        stack_used_ = frame_pointer_ + _destination;
        break;
    }
    case mnemocode::fstp : {
        assert(!(st.depth < _source));
        assert(!(stack_used_ < frame_pointer_));
        assert(!(_destination < stack_used_ - frame_pointer_));
        assert(_source == _destination - (stack_used_ - frame_pointer_)); // frame_used_
        size_type position_ = _destination;
        for (size_type i = 0; i < _source; ++i) {
            if (!interpret(mnemocode::fstp, --position_, meta::memory_layout::stack)) {
                return false;
            }
        }
        stack_used_ = frame_pointer_ + _destination;
        break;
    }
    case mnemocode::ret : {
        assert(0 != _destination); // 1..8
        assert(!(st.depth < _destination));
        assert(!(stack_used_ < frame_pointer_));
        assert(check_head_tail(call_stack_.top(), _destination));
        output_ = _destination;
        /*::std::array< G, st.depth > results_;
        std::copy_n(std::make_move_iterator(std::begin(fpregs_)), _destination, std::begin(results_));
        for (size_type i = 0; i < _destination; ++i) {
            if (!fpop()) {
                return false;
            }
        }*/
        break;
    }
    default : {
        return false;
    }
    }
    return true;
}

auto
virtual_machine::interpret(mnemocode const _mnemocode,
                           size_type const _offset,
                           meta::memory_layout const _memory_layout)
-> result_type
{
    switch (_memory_layout) {
    case meta::memory_layout::heap : {
        if (!(_offset < assembler_.get_heap_size())) {
            return false;
        }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
        switch (_mnemocode) {
#pragma clang diagnostic pop
        case mnemocode::fst :
        case mnemocode::fstp : {
            if (!assembler_.is_global_variable(_offset)) {
                return false;
            }
            return memory_rw_access(_mnemocode, assembler_.get_global_variable(_offset));
        }
        default : {
            return memory_ro_access(_mnemocode, assembler_.get_heap_element(_offset));
        }
        }
    }
    case meta::memory_layout::stack : {
        assert(!(stack_.size() < frame_pointer_));
        if (!(_offset < stack_.size() - frame_pointer_)) {
            return false;
        }
        G & destination_ = stack_.at(frame_pointer_ + _offset);
        if (_mnemocode == mnemocode::alloca_) {
            assert(stack_used_ < stack_.size());
            ++stack_used_;
            return memory_rw_access(mnemocode::fstp, destination_);
        } else {
            return memory_rw_access(_mnemocode, destination_);
        }
    }
    }
    return false;
}

auto
virtual_machine::memory_rw_access(mnemocode const _mnemocode,
                                  G & _destination)
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
        if (!fpregs_.front()) {
            _destination = indefinite_;
            return false;
        }
        _destination = *fpregs_.front();
        break;
    }
    case mnemocode::fstp : {
        if (!fpregs_.front()) {
            _destination = indefinite_;
            return false;
        }
        _destination = std::move(*fpregs_.front());
        if (!fpop()) {
            return false;
        }
        break;
    }
    default : {
        return memory_ro_access(_mnemocode, _destination);
    }
    }
    return true;
}

auto
virtual_machine::memory_ro_access(mnemocode const _mnemocode,
                                  G const & _source)
-> result_type
{
    fpreg_type const source_ = _source;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fld : {
        if (!fpush(*source_)) {
            return false;
        }
        break;
    }
    case mnemocode::fadd :
    case mnemocode::fsub :
    case mnemocode::fsubr :
    case mnemocode::fmul :
    case mnemocode::fdiv :
    case mnemocode::fdivr : {
        if (use_long_double) {
            return false;
        }
        if (!fbinary(_mnemocode, fpregs_.front(), source_)) {
            return false;
        }
        break;
    }
    case mnemocode::fcom :
    case mnemocode::fcomp : {
        if (use_long_double) {
            return false;
        }
        if (!fcom(_mnemocode, fpregs_.front(), source_)) {
            return false;
        }
        break;
    }
    default : {
        return false;
    }
    }
    return true;
}

}
}
#pragma clang diagnostic pop
