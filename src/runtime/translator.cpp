#include <insituc/runtime/jit_compiler/translator.hpp>
#include <insituc/utility/numeric/safe_convert.hpp>

#include <limits>

#include <cstdint>

namespace insituc
{
namespace runtime
{

using meta::st;

auto
translator::translate(mnemocode const _mnemocode)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fninit : {
        // FINIT – Initialize Floating-Point Unit
        return append(0xDB_o, 0xE3_o);
    }
    case mnemocode::ud2 : {
        // UD2 - Undefined instruction 0000 FFFF : 0000 1011
        return append(0x0F_o, 0b00001011_o);
    }
    case mnemocode::fnop : {
        // FNOP - No Operation 11011 001 : 1101 0000
        return append(0b11011001_o, 0b11010000_o);
    }
    case mnemocode::fwait : {
        // FWAIT - Wait until FPU Ready 1001 1011
        return append(0b10011011_o);
    }
    case mnemocode::fnstsw : {
        // FSTSW AX - Store Status Word into AX
        // FSTSW - Store Status Word into Memory 11011 101 : mod 111 r/m
        return append(0b11011111_o, 0b11100000_o);
    }
    case mnemocode::fdecstp : {
        // FDECSTP - Decrement Stack-Top Pointer 11011 001 : 1111 0110
        return append(0b11011001_o, 0b11110110_o);
    }
    case mnemocode::fincstp : {
        // FINCSTP - Increment Stack-Top Pointer 11011 001 : 1111 0111
        return append(0b11011001_o, 0b11110111_o);
    }
    case mnemocode::fxam : {
        // FXAM - Examine 11011 001 : 1110 0101
        return append(0b11011001_o, 0b11100101_o);
    }
    case mnemocode::ftst : {
        // FTST - Test 11011 001 : 1110 0100
        return append(0b11011001_o, 0b11100100_o);
    }
    case mnemocode::fabs : {
        // FABS - Absolute Value 11011 001 : 1110 0001
        return append(0b11011001_o, 0b11100001_o);
    }
    case mnemocode::fchs : {
        // FCHS - Change Sign 11011 001 : 1110 0000
        return append(0b11011001_o, 0b11100000_o);
    }
    case mnemocode::trunc : {
        constexpr register_name base_ = register_name::sp; // return addresses is placed here
        constexpr size_type int64_size_ = sizeof(std::int64_t);
#if defined(__i386__)
        // sub sizeof(int64), %%sp
        if (!sub(base_, int64_size_)) {
            return false;
        }
        // fisttpl -0(%%sp) : FISTTP - Store ST in int64 (chop) and pop (m64int) 11011 101 : modA 001 r/m
        if (!append(0b11011101_o, 0b00001000_o)) {
            return false;
        }
        if (!inderect_address(base_, false, 0)) {
            return false;
        }
        // fildl -0(%%sp) : FILD - Load Integer (64-bit memory) 11011 111 : mod 101 r/m
        if (!append(0b11011111_o, 0b00101000_o)) {
            return false;
        }
        if (!inderect_address(base_, false, 0)) {
            return false;
        }
        // add sizeof(int64), %%sp
        if (!add(base_, int64_size_)) {
            return false;
        }
#elif defined(__x86_64__)
        // Let's use 128-byte "red zone" (http://eli.thegreenplace.net/2011/09/06/stack-frame-layout-on-x86-64/).
        // fisttpl -0(%%sp) : FISTTP - Store ST in int64 (chop) and pop (m64int) 11011 101 : modA 001 r/m
        if (!append(0b11011101_o, 0b00001000_o)) {
            return false;
        }
        if (!inderect_address(base_, false, int64_size_)) {
            return false;
        }
        // fildl -0(%%sp) : FILD - Load Integer (64-bit memory) 11011 111 : mod 101 r/m
        if (!append(0b11011111_o, 0b00101000_o)) {
            return false;
        }
        if (!inderect_address(base_, false, int64_size_)) {
            return false;
        }
#else
#error "Unsupported CPU architecture"
#endif
        return true;
    }
    case mnemocode::frndint : {
        // FRNDINT - Round to Integer 11011 001 : 1111 1100
        return append(0b11011001_o, 0b11111100_o);
    }
    case mnemocode::fsqrt : {
        // FSQRT - Square Root 11011 001 : 1111 1010
        return append(0b11011001_o, 0b11111010_o);
    }
    case mnemocode::fcos : {
        // FCOS - Cosine of ST(0) 11011 001 : 1111 1111
        return append(0b11011001_o, 0b11111111_o);
    }
    case mnemocode::fsin : {
        // FSIN - Sine 11011 001 : 1111 1110
        return append(0b11011001_o, 0b11111110_o);
    }
    case mnemocode::f2xm1 : {
        // F2XM1 - Compute 2ST(0) - 1 11011 001 : 1111 0000
        return append(0b11011001_o, 0b11110000_o);
    }
    case mnemocode::fldz : {
        // FLDZ - Load +0.0 into ST(0) 11011 001 : 1110 1110
        return append(0b11011001_o, 0b11101110_o);
    }
    case mnemocode::fld1 : {
        // FLD1 - Load +1.0 into ST(0) 11011 001 : 1110 1000
        return append(0b11011001_o, 0b11101000_o);
    }
    case mnemocode::fldpi : {
        // FLDPI - Load ? into ST(0) 11011 001 : 1110 1011
        return append(0b11011001_o, 0b11101011_o);
    }
    case mnemocode::fldl2e : {
        // FLDL2E - Load log2(?) into ST(0) 11011 001 : 1110 1010
        return append(0b11011001_o, 0b11101010_o);
    }
    case mnemocode::fldl2t : {
        // FLDL2T - Load log2(10) into ST(0) 11011 001 : 1110 1001
        return append(0b11011001_o, 0b11101001_o);
    }
    case mnemocode::fldlg2 : {
        // FLDLG2 - Load log10(2) into ST(0) 11011 001 : 1110 1100
        return append(0b11011001_o, 0b11101100_o);
    }
    case mnemocode::fldln2 : {
        // FLDLN2 - Load log?(2) into ST(0) 11011 001 : 1110 1101
        return append(0b11011001_o, 0b11101101_o);
    }
    case mnemocode::fxch : { // FXCH = FXCH ST(1)
        // FXCH - Exchange ST(0) and ST(1) 11011 001 : 1100 1 ST(1)
        return append(0b11011001_o, 0b11001001_o);
    }
    case mnemocode::fscale : {
        // FSCALE - Scale 11011 001 : 1111 1101
        return append(0b11011001_o, 0b11111101_o);
    }
    case mnemocode::fprem : {
        // FPREM - Partial Remainder 11011 001 : 1111 1000
        return append(0b11011001_o, 0b11111000_o);
    }
    case mnemocode::fprem1 : {
        // FPREM1 - Partial Remainder (IEEE) 11011 001 : 1111 0101
        return append(0b11011001_o, 0b11110101_o);
    }
    case mnemocode::fcom : { // FCOM = FCOM ST(1)
        // FCOM - Compare Real ST(1) 11011 000 : 11 010 ST(1)
        return append(0b11011000_o, 0b11010001_o);
    }
    case mnemocode::fucom : { // FUCOM = FUCOM ST(1)
        // FUCOM – Unordered Compare Real 11011 101 : 1110 0 ST(1)
        return append(0b11011101_o, 0b11100001_o);
    }
    case mnemocode::fcomp : { // FCOMP = FCOMP ST(1)
        // FCOMP - Compare Real and Pop ST(1) 11011 000 : 11 011 ST(1)
        return append(0b11011000_o, 0b11011001_o);
    }
    case mnemocode::fucomp : { // FUCOMP = FUCOMP ST(1)
        // FUCOMP – Unordered Compare Real and Pop 11011 101 : 1110 1 ST(1)
        return append(0b11011101_o, 0b11101001_o);
    }
    case mnemocode::fcompp : {
        // FCOMPP - Compare Real and Pop Twice 11011 110 : 11 011 001
        return append(0b11011110_o, 0b11011001_o);
    }
    case mnemocode::fucompp : {
        // FUCOMPP – Unordered Compare Real and Pop Twice 11011 010 : 1110 1001
        return append(0b11011010_o, 0b11101001_o);
    }
    case mnemocode::fadd : { // FADD = FADDP ST(1), ST
        // FADDP - Add and Pop ST(1) = ST(0) + ST(1) 11011 110 : 11 000 ST(1)
        return append(0b11011110_o, 0b11000001_o);
    }
    case mnemocode::fsub : { // FSUB = FSUBP ST(1), ST
        // FSUBP - Subtract and Pop ST(1) = ST(0) - ST(1) 11011 110 : 1110 1 ST(1)
        return append(0b11011110_o, 0b11101001_o);
    }
    case mnemocode::fsubr : { // FUSBR = FSUBRP ST(1), ST
        // FSUBRP - Reverse Subtract and Pop ST(1) = ST(1) - ST(0) 11011 110 : 1110 0 ST(1)
        return append(0b11011110_o, 0b11100001_o);
    }
    case mnemocode::fmul : { // FMUL = FMULP ST(1), ST
        // FMULP - Multiply ST(1) = ST(0) - ST(1) 11011 110 : 1100 1 ST(1)
        return append(0b11011110_o, 0b11001001_o);
    }
    case mnemocode::fdiv : { // FDIV = FDIVP ST(1), ST
        // FDIVP - Divide and Pop ST(1) = ST(0) / ST(1) 11011 110 : 1111 1 ST(1)
        return append(0b11011110_o, 0b11111001_o);
    }
    case mnemocode::fdivr : { // FDIVR = FDIVRP ST(1), ST
        // FDIVRP - Reverse Divide and Pop ST(1) = ST(1) / ST(0) 11011 110 : 1111 0 ST(1)
        return append(0b11011110_o, 0b11110001_o);
    }
    case mnemocode::fpatan : {
        // FPATAN - Partial Arctangent 11011 001 : 1111 0011
        return append(0b11011001_o, 0b11110011_o);
    }
    case mnemocode::fyl2x : {
        // FYL2X - ST(1) ? log2(ST(0)) 11011 001 : 1111 0001
        return append(0b11011001_o, 0b11110001_o);
    }
    case mnemocode::fyl2xp1 : {
        // FYL2XP1 - ST(1) ? log2(ST(0) + 1.0) 11011 001 : 1111 1001
        return append(0b11011001_o, 0b11111001_o);
    }
    case mnemocode::fptan : {
        // FPTAN - Partial Tangent 11011 001 : 1111 0010
        return append(0b11011001_o, 0b11110010_o);
    }
    case mnemocode::fsincos : {
        // FSINCOS - Sine and Cosine 11011 001 : 1111 1011
        return append(0b11011001_o, 0b11111011_o);
    }
    case mnemocode::fxtract : {
        // FXTRACT - Extract Exponent and Significand 11011 001 : 1111 0100
        return append(0b11011001_o, 0b11110100_o);
    }
    case mnemocode::sahf : {
        // SAHF - Store AH into Flags 1001 1110
        return append(0b10011110_o);
    }
    case mnemocode::endl : {
        return true;
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::add(register_name const _base, ufar_type const _delta)
-> result_type
{
    // ADD - Integer Addition : immediate to register 1000 00sw : 11 000 reg : immediate data
    // Sign-Extend (s) Bit is set to 0
    // Operand Size (w) Bit is set to 1
    if (!append(0b10000001_o)) {
        return false;
    }
    if (!append(rm(_base) | 0b11000000_o)) { // Add
        return false;
    }
    return add_displacement(_delta);
}

auto
translator::sub(register_name const _base, ufar_type const _delta)
-> result_type
{
    // SUB - Integer Subtraction : immediate to register 1000 00sw : 11 101 reg : immediate data
    // Sign-Extend (s) Bit is set to 0
    // Operand Size (w) Bit is set to 1
    if (!append(0b10000001_o)) {
        return false;
    }
    if (!append(rm(_base) | 0b11101000_o)) { // Sub
        return false;
    }
    return add_displacement(_delta);
}

auto
translator::affect_stack_pointer(mnemocode const _mnemocode,
                                 size_type const _offset)
-> result_type
{
    constexpr register_name stack_base_ = register_name::c;
    if (less(std::numeric_limits< ufar_type >::max() / sizeof(F), _offset)) {
        return false;
    }
    auto const delta_ = static_cast< ufar_type >(sizeof(F) * _offset);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::sp_dec : {
        if (!sub(stack_base_, delta_)) {
            return false;
        }
        assert(!(stack_pointer_ < _offset));
        stack_pointer_ -= _offset;
        break;
    }
    case mnemocode::sp_inc : {
        if (!add(stack_base_, delta_)) {
            return false;
        }
        assert(!(std::numeric_limits< size_type >::max() - _offset < stack_pointer_));
        stack_pointer_ += _offset;
        break;
    }
    default : {
        return false;
    }
    }
    return true;
}

auto
translator::unary(mnemocode const _mnemocode,
                  size_type const _operand)
-> result_type
{
    if (!(_operand < st.depth)) {
        return false;
    }
    auto const o = byte_type(_operand);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fst : {
        // FST - Store Real ST(i) 11011 101 : 11 010 ST(i)
        return append(0b11011101_o, byte_type(0b11010000_o | o));
    }
    case mnemocode::fstp : {
        // FSTP - Store Real and Pop ST(i) 11011 101 : 11 011 ST(i)
        return append(0b11011101_o, byte_type(0b11011000_o | o));
    }
    case mnemocode::fld : {
        // FLD - Load Real ST(i) 11011 001 : 11 000 ST(i)
        return append(0b11011001_o, byte_type(0b11000000_o | o));
    }
    case mnemocode::fcom : {
        // FCOM - Compare Real ST(i) 11011 000 : 11 010 ST(i)
        return append(0b11011000_o, byte_type(0b11010000_o | o));
    }
    case mnemocode::fucom : {
        // FUCOM – Unordered Compare Real 11011 101 : 1110 0 ST(i)
        return append(0b11011101_o, byte_type(0b11100000_o | o));
    }
    case mnemocode::fcomp : {
        // FCOMP - Compare Real and Pop ST(i) 11011 000 : 11 011 ST(i)
        return append(0b11011000_o, byte_type(0b11011000_o | o));
    }
    case mnemocode::fucomp : {
        // FUCOMP – Unordered Compare Real and Pop 11011 101 : 1110 1 ST(i)
        return append(0b11011101_o, byte_type(0b11101000_o | o));
    }
    case mnemocode::ffree : {
        // FFREE - Free ST(i) Register 11011 101 : 1100 0 ST(i)
        return append(0b11011101_o, byte_type(0b11000000_o | o));
    }
    case mnemocode::ffreep : {
        // FFREEP - Free ST(i) Register and Pop 11011 111 : 1100 0 ST(i)
        return append(0b11011111_o, byte_type(0b11000000_o | o));
    }
    case mnemocode::fxch : {
        // FXCH - Exchange ST(0) and ST(i) 11011 001 : 1100 1 ST(i)
        return append(0b11011001_o, byte_type(0b11001000_o | o));
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::translate(mnemocode const _mnemocode,
                      size_type const _operand)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fst :
    case mnemocode::fstp :
    case mnemocode::fld :
    case mnemocode::fcom :
    case mnemocode::fucom :
    case mnemocode::fcomp :
    case mnemocode::fucomp :
    case mnemocode::ffree :
    case mnemocode::ffreep :
    case mnemocode::fxch : {
        return unary(_mnemocode, _operand);
    }
    case mnemocode::sp_inc :
    case mnemocode::sp_dec : {
        return affect_stack_pointer(_mnemocode, _operand);
    }
    case mnemocode::bra :
    case mnemocode::ket :
    case mnemocode::endl : {
        return true;
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::fsource(mnemocode const _mnemocode,
                    size_type const _source)
-> result_type
{
    if (!(_source < st.depth)) {
        return false;
    }
    auto const o = byte_type(_source);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fcmovb : {
        // move if below (B) 11011 010 : 11 000 ST(i)
        return append(0b11011010_o, byte_type(0b11000000_o | o));
    }
    case mnemocode::fcmove : {
        // move if equal (E) 11011 010 : 11 001 ST(i)
        return append(0b11011010_o, byte_type(0b11001000_o | o));
    }
    case mnemocode::fcmovbe : {
        // move if below or equal (BE) 11011 010 : 11 010 ST(i)
        return append(0b11011010_o, byte_type(0b11010000_o | o));
    }
    case mnemocode::fcmovu : {
        // move if unordered (U) 11011 010 : 11 011 ST(i)
        return append(0b11011010_o, byte_type(0b11011000_o | o));
    }
    case mnemocode::fcmovnb : {
        // move if not below (NB) 11011 011 : 11 000 ST(i)
        return append(0b11011011_o, byte_type(0b11000000_o | o));
    }
    case mnemocode::fcmovne : {
        // move if not equal (NE) 11011 011 : 11 001 ST(i)
        return append(0b11011011_o, byte_type(0b11001000_o | o));
    }
    case mnemocode::fcmovnbe : {
        // move if not below or equal (NBE) 11011 011 : 11 010 ST(i)
        return append(0b11011011_o, byte_type(0b11010000_o | o));
    }
    case mnemocode::fcmovnu : {
        // move if not unordered (NU) 11011 011 : 11 011 ST(i)
        return append(0b11011011_o, byte_type(0b11011000_o | o));
    }
    case mnemocode::fcomi : {
        // FCOMI - Compare Real and Set EFLAGS 11011 011 : 11 110 ST(i)
        return append(0b11011011_o, byte_type(0b11110000_o | o));
    }
    case mnemocode::fucomi : {
        // FUCOMI – Unorderd Compare Real and Set EFLAGS 11011 011 : 11 101 ST(i)
        return append(0b11011011_o, byte_type(0b11101000_o | o));
    }
    case mnemocode::fcomip : {
        // FCOMIP - Compare Real, Set EFLAGS, and Pop 11011 111 : 11 110 ST(i)
        return append(0b11011111_o, byte_type(0b11110000_o | o));
    }
    case mnemocode::fucomip : {
        // FUCOMIP – Unorderd Compare Real, Set EFLAGS, and Pop 11011 111 : 11 101 ST(i)
        return append(0b11011111_o, byte_type(0b11101000_o | o));
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::fdestination(mnemocode const _mnemocode,
                         size_type const _destination)
-> result_type
{
    if (!(_destination < st.depth)) {
        return false;
    }
    auto const o = byte_type(_destination);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::faddp : {
        // FADDP - Add and Pop ST(0) = ST(0) + ST(i) 11011 110 : 11 000 ST(i)
        return append(0b11011110_o, byte_type(0b11000000_o | o));
    }
    case mnemocode::fsubp : {
        // FSUBP - Subtract and Pop ST(0) = ST(0) - ST(i) 11011 110 : 1110 1 ST(i)
        return append(0b11011110_o, byte_type(0b11101000_o | o));
    }
    case mnemocode::fsubrp : {
        // FSUBRP - Reverse Subtract and Pop ST(i) = ST(i) - ST(0) 11011 110 : 1110 0 ST(i)
        return append(0b11011110_o, byte_type(0b11100000_o | o));
    }
    case mnemocode::fmulp : {
        // FMULP - Multiply ST(i) = ST(0) / ST(i) 11011 110 : 1100 1 ST(i)
        return append(0b11011110_o, byte_type(0b11001000_o | o));
    }
    case mnemocode::fdivp : {
        // FDIVP - Divide and Pop ST(0) = ST(0) / ST(i) 11011 110 : 1111 1 ST(i)
        return append(0b11011110_o, byte_type(0b11111000_o | o));
    }
    case mnemocode::fdivrp : {
        // FDIVRP - Reverse Divide and Pop ST(0) = ST(i) / ST(0) 11011 110 : 1111 0 ST(i)
        return append(0b11011110_o, byte_type(0b11110000_o | o));
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::fbinary(mnemocode const _mnemocode,
                    size_type const _destination,
                    size_type const _source)
-> result_type
{
    if (!(_destination < st.depth)) {
        return false;
    }
    if (!(_source < st.depth)) {
        return false;
    }
    bool const d = (0 != _destination);
    auto const o = byte_type(d ? _destination : _source);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fadd : {
        // FADD - Add ST(d) = ST(0) + ST(i) 11011 d00 : 11 000 ST(i)
        return append((d ? 0b11011100_o : 0b11011000_o), byte_type(0b11000000_o | o));
    }
    case mnemocode::fsub : {
        // FSUB - Subtract ST(d) = ST(0) - ST(i) 11011 d00 : 1110 R ST(i)
        return append((d ? 0b11011100_o : 0b11011000_o),
                      byte_type(0b11100000_o | (d ? 0b00001000_o : 0b00000000_o) | o));
    }
    case mnemocode::fsubr : {
        // FSUBR - Reverse Subtract ST(d) = ST(i) - ST(0) 11011 d00 : 1110 R ST(i)
        return append((d ? 0b11011100_o : 0b11011000_o),
                      byte_type(0b11100000_o | (d ? 0b00000000_o : 0b00001000_o) | o));
    }
    case mnemocode::fmul : {
        // FMUL - Multiply ST(d) = ST(0) / ST(i) 11011 d00 : 1100 1 ST(i)
        return append((d ? 0b11011100_o : 0b11011000_o),
                      byte_type(0b11001000_o | o));
    }
    case mnemocode::fdiv : {
        // FDIV - Divide ST(d) = ST(0) / ST(i) 11011 d00 : 1111 R ST(i)
        return append((d ? 0b11011100_o : 0b11011000_o),
                      byte_type(0b11110000_o | (d ? 0b00001000_o : 0b00000000_o) | o));
    }
    case mnemocode::fdivr : {
        // FDIVR - Reverse Divide ST(d) = ST(i) / ST(0) 11011 d00 : 1111 R ST(i)
        return append((d ? 0b11011100_o : 0b11011000_o),
                      byte_type(0b11110000_o | (d ? 0b00000000_o : 0b00001000_o) | o));
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::translate(mnemocode const _mnemocode,
                      size_type const _offset,
                      memory_layout const _memory_layout)
-> result_type
{
    switch (_memory_layout) {
    case memory_layout::heap : {
        return heap_access(_mnemocode, _offset);
    }
    case memory_layout::stack : {
        return stack_access(_mnemocode, _offset);
    }
    }
    return false;
}

auto
translator::translate(mnemocode const _mnemocode,
                      size_type const _destination,
                      size_type const _source)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::call : {
        stack_pointer_ += _source;
        // E8 cd CALL rel32 M Valid Valid Call near, relative, displacement relative to
        // next instruction. 32-bit displacement sign
        // extended to 64-bits in 64-bit mode.
        assert(_destination < instance_.entry_points_.size());
        size_type const callee_ = instance_.entry_points_.at(_destination);
        assert(callee_ < instance_.code_.size());
        if (!append(0b11101000_o)) { // CALL - Call Procedure (in same segment) direct 1110 1000 : displacement32
            return false;
        }
        size_type const displacement_ = (instance_.code_.size() + sizeof(far_type)) - callee_;
        if (!is_includes< far_type >(displacement_)) {
            return false;
        }
        return add_displacement(-static_cast< far_type >(displacement_)); // negation of positive number is safe
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
    case mnemocode::fucomi :
    case mnemocode::fcomip :
    case mnemocode::fucomip : {
        if (0 != _destination) {
            return false;
        }
        return fsource(_mnemocode, _source);
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
        return fbinary(_mnemocode, _destination, _source);
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
        return fdestination(_mnemocode, _destination);
    }
    case mnemocode::fld : {
        assert(!(st.depth < _source));
        assert(!(std::numeric_limits< size_type >::max() - _destination < _source));
        size_type position_ = _destination;
        for (size_type i = 0; i < _source; ++i) {
            if (!translate(mnemocode::fld, position_++, memory_layout::stack)) {
                return false;
            }
        }
        return true;
    }
    case mnemocode::fstp : {
        assert(!(st.depth < _source));
        assert(!(_destination < _source));
        size_type position_ = _destination;
        for (size_type i = 0; i < _source; ++i) {
            if (!translate(mnemocode::fstp, --position_, memory_layout::stack)) {
                return false;
            }
        }
        return true;
    }
    case mnemocode::ret : {
        assert(0 != _destination);
        assert(!(st.depth < _destination)); // 1..8
        return append(0b11000011_o); // RET - Return from Procedure (same segment) no argument 1100 0011
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::memory_access(mnemocode const _mnemocode)
-> result_type
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
    switch (_mnemocode) {
#pragma clang diagnostic pop
    case mnemocode::fst : {
        if (use_float) {
            // FST - Store Real 32-bit memory 11011 001 : mod 010 r/m
            return append(0b11011001_o, 0b00010000_o);
        } else if (use_double) {
            // FST - Store Real 64-bit memory 11011 101 : mod 010 r/m
            return append(0b11011101_o, 0b00010000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fstp : {
        if (use_float) {
            // FSTP - Store Real and Pop 32-bit memory 11011 001 : mod 011 r/m
            return append(0b11011001_o, 0b00011000_o);
        } else if (use_double) {
            // FSTP - Store Real and Pop 64-bit memory 11011 101 : mod 011 r/m
            return append(0b11011101_o, 0b00011000_o);
        } else if (use_long_double) {
            // FSTP - Store Real and Pop 80-bit memory 11011 011 : mod 111 r/m
            return append(0b11011011_o, 0b00111000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fld : {
        if (use_float) {
            // FLD - Load Real 32-bit memory 11011 001 : mod 000 r/m
            return append(0b11011001_o, 0b00000000_o);
        } else if (use_double) {
            // FLD - Load Real 64-bit memory 11011 101 : mod 000 r/m
            return append(0b11011101_o, 0b00000000_o);
        } else if (use_long_double) {
            // FLD - Load Real 80-bit memory 11011 011 : mod 101 r/m
            return append(0b11011011_o, 0b00101000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fadd : {
        if (use_float) {
            // FADD - Add ST(0) < ST(0) + 32-bit memory 11011 000 : mod 000 r/m
            return append(0b11011000_o, 0b00000000_o);
        } else if (use_double) {
            // FADD - Add ST(0) < ST(0) + 64-bit memory 11011 100 : mod 000 r/m
            return append(0b11011100_o, 0b00000000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fsub : {
        if (use_float) {
            // FSUB - Subtract ST(0) = ST(0) - 32-bit memory 11011 000 : mod 100 r/m
            return append(0b11011000_o, 0b00100000_o);
        } else if (use_double) {
            // FSUB - Subtract ST(0) = ST(0) - 64-bit memory 11011 100 : mod 100 r/m
            return append(0b11011100_o, 0b00100000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fsubr : {
        if (use_float) {
            // FSUBR - Reverse Subtract ST(0) = 32-bit memory - ST(0) 11011 000 : mod 101 r/m
            return append(0b11011000_o, 0b00101000_o);
        } else if (use_double) {
            // FSUBR - Reverse Subtract ST(0) = 64-bit memory - ST(0) 11011 100 : mod 101 r/m
            return append(0b11011100_o, 0b00101000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fmul : {
        if (use_float) {
            // FMUL - Multiply ST(0) = ST(0) - 32-bit memory 11011 000 : mod 001 r/m
            return append(0b11011000_o, 0b00001000_o);
        } else if (use_double) {
            // FMUL - Multiply ST(0) = ST(0) - 64-bit memory 11011 100 : mod 001 r/m
            return append(0b11011100_o, 0b00001000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fdiv : {
        if (use_float) {
            // FDIV - Divide ST(0) = ST(0) / 32-bit memory 11011 000 : mod 110 r/m
            return append(0b11011000_o, 0b00110000_o);
        } else if (use_double) {
            // FDIV - Divide ST(0) = ST(0) / 64-bit memory 11011 100 : mod 110 r/m
            return append(0b11011100_o, 0b00110000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fdivr : {
        if (use_float) {
            // FDIVR - Reverse Divide ST(0) = 32-bit memory / ST(0) 11011 000 : mod 111 r/m
            return append(0b11011000_o, 0b00111000_o);
        } else if (use_double) {
            // FDIVR - Reverse Divide ST(0) = 64-bit memory / ST(0) 11011 100 : mod 111 r/m
            return append(0b11011100_o, 0b00111000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fcom : {
        if (use_float) {
            // FCOM - Compare Real 32-bit memory 11011 000 : mod 010 r/m
            return append(0b11011000_o, 0b00010000_o);
        } else if (use_double) {
            // FCOM - Compare Real 64-bit memory 11011 100 : mod 010 r/m
            return append(0b11011100_o, 0b00010000_o);
        } else {
            return false;
        }
    }
    case mnemocode::fcomp : {
        if (use_float) {
            // FCOMP - Compare Real and Pop 32-bit memory 11011 000 : mod 011 r/m
            return append(0b11011000_o, 0b00011000_o);
        } else if (use_double) {
            // FCOMP - Compare Real and Pop 64-bit memory 11011 100 : mod 011 r/m
            return append(0b11011100_o, 0b00011000_o);
        } else {
            return false;
        }
    }
    default : {
        break;
    }
    }
    return false;
}

auto
translator::heap_access(mnemocode const _mnemocode, size_type const _offset)
-> result_type
{
    if (std::numeric_limits< size_type >::max() / sizeof(F) < _offset) {
        return false;
    }
    if (!memory_access(_mnemocode)) {
        return false;
    }
    constexpr register_name heap_base_ = register_name::d;
    return inderect_address(heap_base_, true, sizeof(F) * _offset);
}

constexpr
size_type
delta(size_type const _x, size_type const _y) noexcept
{
    return (_x < _y) ? (_y - _x) : (_x - _y);
}

auto
translator::stack_access(mnemocode const _mnemocode, size_type const _offset)
-> result_type
{
    if (!memory_access((mnemocode::alloca_ == _mnemocode) ? mnemocode::fstp : _mnemocode)) {
        return false;
    }
    constexpr register_name stack_base_ = register_name::c;
    size_type const delta_ = delta(_offset, stack_pointer_);
    if (std::numeric_limits< size_type >::max() / sizeof(F) < delta_) {
        return false;
    }
    return inderect_address(stack_base_, (stack_pointer_ < _offset), sizeof(F) * delta_);
}

constexpr
byte_type
make_mod_rm(byte_type const _mod, byte_type const _rm) noexcept
{
    // 2 bits Mod, 3 bits TTT (opcode extension field) or 3 bits Reg Field, 3 bits R/M => shift left Mod field by 6 positions
    auto mod_rm = _mod;
    mod_rm <<= 6;
    mod_rm |= _rm;
    return mod_rm;
}

auto
translator::inderect_address(register_name const _base, bool const _increase, size_type const _displacement)
-> result_type
{
    byte_type & op_b_ = instance_.code_.back(); // modify 2-nd byte
    // The base-plus-index and scale-plus-index forms of 32-bit addressing requires the SIB byte.
    using sib_byte = byte_type; // Intel terms
    using disp8 = near_type; // Intel terms
    byte_type const rm_ = rm(_base);
    if (_displacement == 0) {
        // no displacement <=> zero offset/displacement in case of BP register
        if (_base == register_name::bp) {
            // (Mod = 01) + disp8 == 0 + R/M = 101
            constexpr byte_type mod_ = 0b01_o; // Mod = 01
            op_b_ |= make_mod_rm(mod_, rm_);
            return append(byte_type(disp8(0x00))); // explicit disp8 = 0
        } else {
            constexpr byte_type mod_ = 0b00_o; // Mod = 00
            op_b_ |= make_mod_rm(mod_, rm_);
            if (_base == register_name::sp) {
                // R/M = 100 => SIB byte (SS = 00, Index = 100 (none), REG = 100) == 0x24 without disp(Mod = 00)
                return append(sib_byte(0b00100100_o)); // SIB byte == 0x24
            } else {
                return true;
            }
        }
    } else if (is_includes< near_type >(_displacement)) {
        // near offset/displacement
        constexpr byte_type mod_ = 0b01_o; // Mod = 01
        op_b_ |= make_mod_rm(mod_, rm_);
        if (_base == register_name::sp) {
            // R/M = 100 => SIB byte (SS = 00, Index = 100 (none), REG = 100) == 0x24 + disp8(Mod = 01)
            if (!append(sib_byte(0b00100100_o))) { // SIB byte == 0x24
                return false;
            }
        }
        auto const displacement_ = static_cast< near_type >(_displacement);
        return add_displacement(static_cast< near_type >(_increase ? +displacement_ : -displacement_));
    } else if (is_includes< far_type >(_displacement)) {
        // far offset/displacement
        constexpr byte_type mod_ = 0b10_o; // Mod = 10
        op_b_ |= make_mod_rm(mod_, rm_);
        if (_base == register_name::sp) {
            // R/M = 100 => SIB byte (SS = 00, Index = 100 (none), REG = 100) == 0x24 + disp32(Mod = 10)
            if (!append(sib_byte(0b00100100_o))) { // SIB byte == 0x24
                return false;
            }
        }
        auto const displacement_ = static_cast< far_type >(_displacement);
        return add_displacement(static_cast< far_type >(_increase ? +displacement_ : -displacement_));
    } else {
        return false;
    }
    // TODO: (possible)
    // absolute addressing for x32 and RIP-relative addressing for x32-64
    // append(make_mod_rm(0b00_o, 0b101_o) | _op_b); // Mod = 00, R/M = 101
    // add_displacement(&_operand - (code_.data() + code_.size()));
}

}
}
